/*
FxSound — Linux build (M2/M3/M4: PipeWire backend)

Linux implementation of AudioPassthru on libpipewire. Two nodes:
  - "FxSound": a real null-audio-sink (support.null-audio-sink) so WirePlumber
    treats it as a first-class routable/default playback sink that apps play
    into (a bare pw_filter sink is NOT routed by the policy). Its monitor ports
    carry whatever apps played.
  - "FxSound Processor": a hidden pw_filter whose inputs are linked from the
    sink monitor and whose outputs are linked to the chosen device. The realtime
    callback runs DfxDsp::processAudio between them (M3).
A registry listener enumerates real sinks (getSoundDevices), tracks our two
nodes' ports, and (re)creates the monitor->processor and processor->device
links; hot-plug add/remove notifies the app (M4).

Threading: all PipeWire calls happen on the pw_thread_loop thread; public
methods take the loop lock. The realtime process callback is lock-free.
*/

#include "AudioPassthru.h"
#include "DfxDsp.h"

#include <pipewire/pipewire.h>
#include <pipewire/filter.h>
#include <pipewire/extensions/metadata.h>
#include <spa/utils/json.h>

#include <atomic>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace {
constexpr int kChannels = 2;

struct NodeInfo {
    uint32_t id = 0;
    std::string name;
    std::string description;
    std::string media_class;
};
struct PortInfo {
    uint32_t id = 0;
    uint32_t node_id = 0;
    enum pw_direction direction = PW_DIRECTION_INPUT;
    bool is_monitor = false;
    std::string channel;   // e.g. "FL", "FR"
    std::string name;
};
} // namespace

struct AudioPassthruPrivate
{
    pw_thread_loop* loop = nullptr;
    pw_context*     context = nullptr;
    pw_core*        core = nullptr;
    pw_filter*      filter = nullptr;
    pw_proxy*       sink_proxy = nullptr;     // the routable null-audio-sink "FxSound"
    pw_registry*    registry = nullptr;
    spa_hook        registry_listener{};
    spa_hook        filter_listener{};

    uint32_t        filter_node_id = SPA_ID_INVALID;  // hidden processor node
    uint32_t        sink_node_id = SPA_ID_INVALID;    // FxSound null-sink node
    std::vector<pw_proxy*> monitor_links;             // sink monitor -> filter input

    // "default" metadata: used to make FxSound the default sink on startup and
    // restore the previous default on exit.
    pw_metadata*    metadata = nullptr;
    spa_hook        metadata_listener{};
    std::string     prev_default_sink;     // value to restore on exit
    bool            captured_default = false;
    bool            default_set = false;

    float* in_ports[kChannels]  = {nullptr, nullptr};
    float* out_ports[kChannels] = {nullptr, nullptr};

    DfxDsp* dsp = nullptr;
    std::atomic<bool> muted{false};
    AudioPassthruCallback* callback = nullptr;

    // Graph model (guarded by the thread-loop lock).
    std::map<uint32_t, NodeInfo> nodes;
    std::map<uint32_t, PortInfo> ports;
    std::vector<pw_proxy*> links;        // links we created (FxSound -> device)
    std::string default_sink_name;       // from the default-metadata
    std::string selected_sink_name;      // user choice; empty => default

    std::vector<float> ileave_in;     // interleaved L/R, 32-bit float
    std::vector<float> ileave_out;

    int  configured_rate = 0;   // last rate passed to dsp->setSignalFormat
    bool started = false;
};

namespace {

// ---- realtime processing ------------------------------------------------
void on_process(void* userdata, struct spa_io_position* position)
{
    auto* d = static_cast<AudioPassthruPrivate*>(userdata);
    uint32_t n = position->clock.duration;

    float* in[kChannels];
    float* out[kChannels];
    for (int c = 0; c < kChannels; ++c) {
        in[c]  = static_cast<float*>(pw_filter_get_dsp_buffer(d->in_ports[c], n));
        out[c] = static_cast<float*>(pw_filter_get_dsp_buffer(d->out_ports[c], n));
    }
    for (int c = 0; c < kChannels; ++c) if (!out[c]) return;

    if (d->muted.load(std::memory_order_relaxed)) {
        for (int c = 0; c < kChannels; ++c) std::memset(out[c], 0, n * sizeof(float));
        return;
    }

    bool have_in = in[0] && in[1];
    if (d->dsp && have_in) {
        // Process in native 32-bit float (PipeWire's format) so there is no
        // 16-bit quantization loss. The DSP must be told the format or it
        // produces no output; bps=32 routes it through the float path.
        int rate = (position->clock.rate.denom > 0) ? (int)position->clock.rate.denom : 48000;
        if (d->configured_rate != rate) {
            d->dsp->setSignalFormat(32, kChannels, rate, 32);
            d->configured_rate = rate;
        }
        if (d->ileave_in.size() < n * kChannels) {
            d->ileave_in.resize(n * kChannels);
            d->ileave_out.resize(n * kChannels);
        }
        for (uint32_t i = 0; i < n; ++i) {
            d->ileave_in[i*2]     = in[0][i];
            d->ileave_in[i*2 + 1] = in[1][i];
        }
        // processAudio takes short*; for bps=32 the engine reinterprets the
        // buffers as 32-bit float internally.
        d->dsp->processAudio(reinterpret_cast<short*>(d->ileave_in.data()),
                             reinterpret_cast<short*>(d->ileave_out.data()), (int)n, 0);
        for (uint32_t i = 0; i < n; ++i) {
            float l = d->ileave_out[i*2];
            float r = d->ileave_out[i*2 + 1];
            // Final hard-clip to prevent digital distortion reaching PipeWire/mixer.
            if (l > 1.0f) l = 1.0f; else if (l < -1.0f) l = -1.0f;
            if (r > 1.0f) r = 1.0f; else if (r < -1.0f) r = -1.0f;
            out[0][i] = l;
            out[1][i] = r;
        }
    } else if (have_in) {
        for (int c = 0; c < kChannels; ++c) std::memcpy(out[c], in[c], n * sizeof(float));
    } else {
        for (int c = 0; c < kChannels; ++c) std::memset(out[c], 0, n * sizeof(float));
    }
}

const struct pw_filter_events filter_events = {
    .version = PW_VERSION_FILTER_EVENTS,
    .process = on_process,
};

// ---- linking helpers ----------------------------------------------------
void clearLinks(AudioPassthruPrivate* d)
{
    for (auto* p : d->links) pw_proxy_destroy(p);
    d->links.clear();
}

// Link FxSound output ports to the input ports of the given sink node, paired
// by channel (FL/FR) with a positional fallback.
void linkToNode(AudioPassthruPrivate* d, uint32_t sink_node_id)
{
    if (d->filter_node_id == SPA_ID_INVALID) return;
    clearLinks(d);

    std::vector<const PortInfo*> our_out, dev_in;
    for (auto& kv : d->ports) {
        const PortInfo& p = kv.second;
        if (p.node_id == d->filter_node_id && p.direction == PW_DIRECTION_OUTPUT)
            our_out.push_back(&p);
        if (p.node_id == sink_node_id && p.direction == PW_DIRECTION_INPUT)
            dev_in.push_back(&p);
    }

    auto findByChannel = [](std::vector<const PortInfo*>& v, const std::string& ch) -> const PortInfo* {
        for (auto* p : v) if (p->channel == ch) return p;
        return nullptr;
    };

    const char* chans[kChannels] = {"FL", "FR"};
    for (int c = 0; c < kChannels; ++c) {
        const PortInfo* o = findByChannel(our_out, chans[c]);
        const PortInfo* i = findByChannel(dev_in, chans[c]);
        if (!o && (int)our_out.size() > c) o = our_out[c];   // positional fallback
        if (!i && (int)dev_in.size()  > c) i = dev_in[c];
        if (!o || !i) continue;

        char so[16], si[16];
        std::snprintf(so, sizeof(so), "%u", o->id);
        std::snprintf(si, sizeof(si), "%u", i->id);
        auto* props = pw_properties_new(
            PW_KEY_LINK_OUTPUT_PORT, so,
            PW_KEY_LINK_INPUT_PORT,  si,
            PW_KEY_OBJECT_LINGER,    "false",
            nullptr);
        auto* link = static_cast<pw_proxy*>(pw_core_create_object(
            d->core, "link-factory", PW_TYPE_INTERFACE_Link, PW_VERSION_LINK,
            &props->dict, 0));
        pw_properties_free(props);
        if (link) d->links.push_back(link);
    }
}

uint32_t resolveTargetSink(AudioPassthruPrivate* d)
{
    const std::string& want = !d->selected_sink_name.empty() ? d->selected_sink_name
                                                              : d->default_sink_name;
    // Prefer exact name match; otherwise first real Audio/Sink that isn't us.
    uint32_t fallback = SPA_ID_INVALID;
    for (auto& kv : d->nodes) {
        const NodeInfo& n = kv.second;
        if (n.media_class != "Audio/Sink" || n.name == "FxSound") continue;
        if (!want.empty() && n.name == want) return n.id;
        if (fallback == SPA_ID_INVALID) fallback = n.id;
    }
    return fallback;
}

// Link the FxSound null-sink's monitor ports to the processor's input ports,
// so audio apps play into the sink reaches the DSP.
void linkMonitorToFilter(AudioPassthruPrivate* d)
{
    if (d->filter_node_id == SPA_ID_INVALID || d->sink_node_id == SPA_ID_INVALID) return;

    for (auto* p : d->monitor_links) pw_proxy_destroy(p);
    d->monitor_links.clear();

    std::vector<const PortInfo*> mon, fin;
    for (auto& kv : d->ports) {
        const PortInfo& p = kv.second;
        if (p.node_id == d->sink_node_id && p.direction == PW_DIRECTION_OUTPUT && p.is_monitor)
            mon.push_back(&p);
        if (p.node_id == d->filter_node_id && p.direction == PW_DIRECTION_INPUT)
            fin.push_back(&p);
    }
    auto byCh = [](std::vector<const PortInfo*>& v, const std::string& ch) -> const PortInfo* {
        for (auto* p : v) if (p->channel == ch) return p;
        return nullptr;
    };
    const char* chans[kChannels] = {"FL", "FR"};
    for (int c = 0; c < kChannels; ++c) {
        const PortInfo* o = byCh(mon, chans[c]);
        const PortInfo* i = byCh(fin, chans[c]);
        if (!o && (int)mon.size() > c) o = mon[c];
        if (!i && (int)fin.size() > c) i = fin[c];
        if (!o || !i) continue;
        char so[16], si[16];
        std::snprintf(so, sizeof(so), "%u", o->id);
        std::snprintf(si, sizeof(si), "%u", i->id);
        auto* props = pw_properties_new(PW_KEY_LINK_OUTPUT_PORT, so,
                                        PW_KEY_LINK_INPUT_PORT, si,
                                        PW_KEY_OBJECT_LINGER, "false", nullptr);
        auto* link = static_cast<pw_proxy*>(pw_core_create_object(
            d->core, "link-factory", PW_TYPE_INTERFACE_Link, PW_VERSION_LINK, &props->dict, 0));
        pw_properties_free(props);
        if (link) d->monitor_links.push_back(link);
    }
}

void reroute(AudioPassthruPrivate* d)
{
    uint32_t target = resolveTargetSink(d);
    if (target != SPA_ID_INVALID) linkToNode(d, target);  // processor out -> device
    linkMonitorToFilter(d);                                // sink monitor -> processor in
}

// ---- default-sink metadata ----------------------------------------------
void setDefaultSinkToFxSound(AudioPassthruPrivate* d)
{
    if (!d->metadata || d->sink_node_id == SPA_ID_INVALID || d->default_set) return;
    pw_metadata_set_property(d->metadata, 0, "default.configured.audio.sink",
                             "Spa:String:JSON", "{ \"name\": \"FxSound\" }");
    d->default_set = true;
}

int metadata_property(void* data, uint32_t subject, const char* key,
                      const char* /*type*/, const char* value)
{
    auto* d = static_cast<AudioPassthruPrivate*>(data);
    if (subject != 0 || !key) return 0;
    // Capture the existing default sink once (before we override it) so we can
    // restore it on exit. Ignore our own value.
    if (spa_streq(key, "default.configured.audio.sink") && value && !d->captured_default)
    {
        if (!strstr(value, "FxSound"))
        {
            d->prev_default_sink = value;
            d->captured_default = true;
        }
    }
    return 0;
}

const struct pw_metadata_events metadata_events = {
    .version = PW_VERSION_METADATA_EVENTS,
    .property = metadata_property,
};

// ---- registry listener --------------------------------------------------
void registry_global(void* data, uint32_t id, uint32_t /*perm*/, const char* type,
                     uint32_t /*ver*/, const struct spa_dict* props)
{
    auto* d = static_cast<AudioPassthruPrivate*>(data);
    if (!props) return;

    if (spa_streq(type, PW_TYPE_INTERFACE_Node)) {
        const char* mc = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
        const char* nm = spa_dict_lookup(props, PW_KEY_NODE_NAME);
        const char* de = spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION);
        NodeInfo ni;
        ni.id = id;
        ni.name = nm ? nm : "";
        ni.description = de ? de : (nm ? nm : "");
        ni.media_class = mc ? mc : "";
        d->nodes[id] = ni;
        // Our own routable null-sink front-end.
        if (mc && spa_streq(mc, "Audio/Sink") && ni.name == "FxSound")
            d->sink_node_id = id;
        if (mc && spa_streq(mc, "Audio/Sink") && ni.name != "FxSound") {
            if (d->callback) d->callback->onSoundDeviceChange();
        }
        reroute(d);
        setDefaultSinkToFxSound(d);
    } else if (spa_streq(type, PW_TYPE_INTERFACE_Metadata)) {
        const char* mn = spa_dict_lookup(props, "metadata.name");
        if (mn && spa_streq(mn, "default") && !d->metadata) {
            d->metadata = static_cast<pw_metadata*>(pw_registry_bind(
                d->registry, id, PW_TYPE_INTERFACE_Metadata, PW_VERSION_METADATA, 0));
            if (d->metadata)
                pw_metadata_add_listener(d->metadata, &d->metadata_listener,
                                         &metadata_events, d);
        }
    } else if (spa_streq(type, PW_TYPE_INTERFACE_Port)) {
        const char* nid = spa_dict_lookup(props, PW_KEY_NODE_ID);
        const char* dir = spa_dict_lookup(props, PW_KEY_PORT_DIRECTION);
        const char* ch  = spa_dict_lookup(props, PW_KEY_AUDIO_CHANNEL);
        const char* nm  = spa_dict_lookup(props, PW_KEY_PORT_NAME);
        const char* mon = spa_dict_lookup(props, PW_KEY_PORT_MONITOR);
        PortInfo pi;
        pi.id = id;
        pi.node_id = nid ? (uint32_t)strtoul(nid, nullptr, 10) : 0;
        pi.direction = (dir && spa_streq(dir, "out")) ? PW_DIRECTION_OUTPUT : PW_DIRECTION_INPUT;
        pi.is_monitor = (mon && spa_streq(mon, "true"));
        pi.channel = ch ? ch : "";
        pi.name = nm ? nm : "";
        d->ports[id] = pi;
        // A new port on our nodes or the target may complete a routing.
        reroute(d);
    }
}

void registry_global_remove(void* data, uint32_t id)
{
    auto* d = static_cast<AudioPassthruPrivate*>(data);
    bool was_sink = false;
    auto it = d->nodes.find(id);
    if (it != d->nodes.end()) {
        was_sink = (it->second.media_class == "Audio/Sink");
        d->nodes.erase(it);
    }
    d->ports.erase(id);
    if (was_sink) {
        if (d->callback) d->callback->onSoundDeviceChange();
        reroute(d);
    }
}

const struct pw_registry_events registry_events = {
    .version = PW_VERSION_REGISTRY_EVENTS,
    .global = registry_global,
    .global_remove = registry_global_remove,
};

} // namespace

AudioPassthru::AudioPassthru() : data_(new AudioPassthruPrivate()) {}

AudioPassthru::~AudioPassthru()
{
    if (!data_) return;

    // Restore the previous default sink we replaced on startup. Set the
    // property under the lock, then release it so the loop thread can flush the
    // message before we tear everything down.
    if (data_->loop && data_->metadata && data_->default_set &&
        data_->captured_default && !data_->prev_default_sink.empty())
    {
        pw_thread_loop_lock(data_->loop);
        pw_metadata_set_property(data_->metadata, 0, "default.configured.audio.sink",
                                 "Spa:String:JSON", data_->prev_default_sink.c_str());
        pw_thread_loop_unlock(data_->loop);
        struct timespec ts{0, 80 * 1000 * 1000};
        nanosleep(&ts, nullptr);   // let the loop flush the metadata update
    }

    if (data_->loop) pw_thread_loop_lock(data_->loop);
    for (auto* p : data_->monitor_links) pw_proxy_destroy(p);
    clearLinks(data_);
    if (data_->metadata) pw_proxy_destroy((pw_proxy*)data_->metadata);
    if (data_->sink_proxy) pw_proxy_destroy(data_->sink_proxy);
    if (data_->registry) pw_proxy_destroy((pw_proxy*)data_->registry);
    if (data_->filter)   pw_filter_destroy(data_->filter);
    if (data_->core)     pw_core_disconnect(data_->core);
    if (data_->loop)     pw_thread_loop_unlock(data_->loop);
    if (data_->loop)     pw_thread_loop_stop(data_->loop);
    if (data_->context)  pw_context_destroy(data_->context);
    if (data_->loop)     pw_thread_loop_destroy(data_->loop);
    delete data_;
    data_ = nullptr;
}

int AudioPassthru::init()
{
    static bool pw_inited = false;
    if (!pw_inited) { pw_init(nullptr, nullptr); pw_inited = true; }

    data_->loop = pw_thread_loop_new("fxsound", nullptr);
    if (!data_->loop) return 1;
    data_->context = pw_context_new(pw_thread_loop_get_loop(data_->loop), nullptr, 0);
    if (!data_->context) return 1;

    pw_thread_loop_lock(data_->loop);
    if (pw_thread_loop_start(data_->loop) != 0) { pw_thread_loop_unlock(data_->loop); return 1; }

    data_->core = pw_context_connect(data_->context, nullptr, 0);
    if (!data_->core) { pw_thread_loop_unlock(data_->loop); return 1; }

    data_->registry = pw_core_get_registry(data_->core, PW_VERSION_REGISTRY, 0);
    pw_registry_add_listener(data_->registry, &data_->registry_listener,
                             &registry_events, data_);

    // Front-end: a real null-audio-sink named "FxSound" that WirePlumber treats
    // as a first-class routable/default-able sink (same approach as EasyEffects).
    // Apps play into it; its monitor ports carry that audio, which we capture,
    // process and forward to the device. A bare pw_filter sink is NOT routed by
    // the policy, so we must use the null-sink adapter factory here.
    auto* sink_props = pw_properties_new(
        PW_KEY_FACTORY_NAME,     "support.null-audio-sink",
        PW_KEY_NODE_NAME,        "FxSound",
        PW_KEY_NODE_DESCRIPTION, "FxSound",
        PW_KEY_MEDIA_CLASS,      "Audio/Sink",
        "audio.position",        "FL,FR",
        "monitor.channel-volumes", "false",
        "monitor.passthrough",   "true",
        "node.virtual",          "true",
        nullptr);
    data_->sink_proxy = static_cast<pw_proxy*>(pw_core_create_object(
        data_->core, "adapter", PW_TYPE_INTERFACE_Node, PW_VERSION_NODE,
        &sink_props->dict, 0));
    pw_properties_free(sink_props);

    // Back-end: hidden processor node. Its inputs are fed from the FxSound sink
    // monitor; its outputs go to the chosen device. NOT an Audio/Sink (so it is
    // not a second routable sink).
    auto* props = pw_properties_new(
        PW_KEY_MEDIA_TYPE,       "Audio",
        PW_KEY_MEDIA_CATEGORY,   "Filter",
        PW_KEY_MEDIA_ROLE,       "DSP",
        PW_KEY_NODE_NAME,        "FxSound Processor",
        PW_KEY_NODE_DESCRIPTION, "FxSound Processor",
        // We wire this node's links ourselves (monitor -> in, out -> device);
        // keep WirePlumber from auto-connecting it to other nodes.
        PW_KEY_NODE_AUTOCONNECT, "false",
        nullptr);

    data_->filter = pw_filter_new(data_->core, "FxSound Processor", props);
    if (!data_->filter) { pw_thread_loop_unlock(data_->loop); return 1; }
    pw_filter_add_listener(data_->filter, &data_->filter_listener, &filter_events, data_);

    const char* in_names[kChannels]  = {"input_FL", "input_FR"};
    const char* out_names[kChannels] = {"output_FL", "output_FR"};
    const char* chans[kChannels]     = {"FL", "FR"};
    for (int c = 0; c < kChannels; ++c) {
        data_->in_ports[c] = static_cast<float*>(pw_filter_add_port(
            data_->filter, PW_DIRECTION_INPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS, sizeof(float),
            pw_properties_new(PW_KEY_FORMAT_DSP, "32 bit float mono audio",
                              PW_KEY_PORT_NAME, in_names[c],
                              PW_KEY_AUDIO_CHANNEL, chans[c], nullptr), nullptr, 0));
        data_->out_ports[c] = static_cast<float*>(pw_filter_add_port(
            data_->filter, PW_DIRECTION_OUTPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS, sizeof(float),
            pw_properties_new(PW_KEY_FORMAT_DSP, "32 bit float mono audio",
                              PW_KEY_PORT_NAME, out_names[c],
                              PW_KEY_AUDIO_CHANNEL, chans[c], nullptr), nullptr, 0));
    }

    if (pw_filter_connect(data_->filter, PW_FILTER_FLAG_RT_PROCESS, nullptr, 0) < 0) {
        pw_thread_loop_unlock(data_->loop); return 1;
    }

    data_->started = true;
    pw_thread_loop_unlock(data_->loop);

    // Give the server a moment to assign our node id, then route to a device.
    for (int tries = 0; tries < 50 && data_->filter_node_id == SPA_ID_INVALID; ++tries) {
        pw_thread_loop_lock(data_->loop);
        data_->filter_node_id = pw_filter_get_node_id(data_->filter);
        if (data_->filter_node_id != SPA_ID_INVALID) reroute(data_);
        pw_thread_loop_unlock(data_->loop);
        struct timespec ts{0, 10 * 1000 * 1000};
        nanosleep(&ts, nullptr);
    }
    return 0;
}

void AudioPassthru::mute(bool mute)
{
    if (data_) data_->muted.store(mute, std::memory_order_relaxed);
}

std::vector<SoundDevice> AudioPassthru::getSoundDevices(bool)
{
    std::vector<SoundDevice> out;
    if (!data_) return out;
    pw_thread_loop_lock(data_->loop);

    // Our own FxSound node is the Linux equivalent of the Windows "FxSound Audio
    // Enhancer" virtual driver. Surface it so the controller marks the engine
    // enabled (dfx_enabled_); isRealDevice=false keeps it out of the output list.
    if (data_->started) {
        SoundDevice dfx;
        dfx.isDFXDevice = true;
        dfx.isRealDevice = false;
        dfx.isActive = true;
        dfx.deviceNumChannel = kChannels;
        dfx.deviceFriendlyName = L"FxSound Audio Enhancer";
        dfx.deviceDescription = L"FxSound Audio Enhancer";
        dfx.pwszID = L"FxSound";
        out.push_back(dfx);
    }

    for (auto& kv : data_->nodes) {
        const NodeInfo& n = kv.second;
        if (n.media_class != "Audio/Sink" || n.name == "FxSound") continue;
        SoundDevice sd;
        sd.isPlaybackDevice = true;
        sd.isRealDevice = true;
        sd.isActive = true;
        sd.isDefaultDevice = (n.name == data_->default_sink_name);
        sd.deviceNumChannel = 2;
        sd.pwszID = std::wstring(n.name.begin(), n.name.end());
        sd.pwszIDRealDevices = sd.pwszID;
        sd.deviceFriendlyName = std::wstring(n.description.begin(), n.description.end());
        sd.deviceDescription = sd.deviceFriendlyName;
        out.push_back(sd);
    }
    pw_thread_loop_unlock(data_->loop);
    return out;
}

int  AudioPassthru::setBufferLength(int)                    { return 0; }
int  AudioPassthru::processTimer()                          { return 0; }

void AudioPassthru::setDspProcessingModule(DfxDsp* dsp)
{
    if (data_) data_->dsp = dsp;
}

void AudioPassthru::setAsPlaybackDevice(const SoundDevice sound_device)
{
    if (!data_) return;
    pw_thread_loop_lock(data_->loop);
    data_->selected_sink_name.assign(sound_device.pwszID.begin(), sound_device.pwszID.end());
    reroute(data_);
    pw_thread_loop_unlock(data_->loop);
}

void AudioPassthru::registerCallback(AudioPassthruCallback* cb)
{
    if (data_) data_->callback = cb;
}

bool AudioPassthru::isPlaybackDeviceAvailable()
{
    return data_ && data_->started && !data_->links.empty();
}

bool AudioPassthru::checkDeviceChanges() { return false; }

void AudioPassthru::restoreDefaultPlaybackDevice()
{
    if (!data_) return;
    pw_thread_loop_lock(data_->loop);
    data_->selected_sink_name.clear();
    reroute(data_);
    pw_thread_loop_unlock(data_->loop);
}

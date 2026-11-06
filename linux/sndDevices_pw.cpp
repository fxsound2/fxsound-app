/*
FxSound Linux port — linux/sndDevices_pw.cpp (Phase 7 real implementation)
Worker mode (RT_AUDIT=NOT_RT_SAFE): capture -> ring -> worker(processAudio) -> ring -> playback.
PipeWire replacement for the WASAPI sndDevices layer + fxvad virtual driver:

  fxvad.sys            -> null sink support.null-audio-sink (adapter factory), node.name=fxsound_audio_enhancer,
                          node.description="FxSound Audio Enhancer", media.class=Audio/Sink,
                          audio.position=[FL,FR], object.linger=false, node.pause-on-idle=false,
                          session.suspend-timeout-seconds=0, monitor.channel-volumes=false.
  IMMDeviceEnumerator  -> pw_registry listener on Audio/Sink nodes (cache + change flag).
  WASAPI loopback      -> capture pw_stream stream.capture.sink=true, target=fxsound*.monitor.
  IAudioRenderClient   -> playback pw_stream target=<real node.name>, node.dont-reconnect +
                          node.dont-fallback (device gone => silence + wait, per plan §4).
  SetDefaultEndpoint   -> default metadata default.configured.audio.sink, fallback wpctl.
  Volume               -> Mode A (see DECISIONS): mirror fxsound sink volumes -> real sink
                          (event-driven via node Props subscription, debounced, one-way);
                          monitor.channel-volumes=false keeps the monitor pre-volume so no
                          capture division is needed. B-mode divide deferred pending spike.
  previous_sink        -> XDG_STATE_HOME/fxsound/previous_sink; restore only if the current
                          default is still fxsound; crash: non-lingering sink vanishes and
                          WirePlumber falls back (linger=false semantics confirmed in spike).

DSP runs outside (worker in linux/AudioPassthru_linux.cpp) over the exported SPSC ring
accessors (pwCaptureRead/pwPlaybackWrite). F32 interleaved 48k/2ch both streams.
This file is JUCE-free and never includes compat/ (see DECISIONS). Windows never compiles
the __linux__ body.
*/
#include "sndDevices_pw.h"

#ifdef __linux__
#include <pipewire/pipewire.h>
#include <pipewire/extensions/metadata.h>
#include <spa/param/audio/format-utils.h>
#include <spa/param/props.h>
#include <spa/utils/result.h>
#include <spa/utils/json.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fcntl.h>
#include <map>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>
#include <sys/wait.h>

namespace fxsound_linux {
namespace {

constexpr const char* kFxNodeName = "fxsound_audio_enhancer";
constexpr const char* kFxDescription = "FxSound Audio Enhancer";
constexpr size_t kMaxQ = 8192, kCh = 2;

bool g_verbose = false;
PwDrift g_drift;

// ---- SPSC rings: capture RT thread produces, DSP worker consumes;
//      DSP worker produces, playback RT thread consumes. ----
std::vector<float> g_capRing, g_playRing;
std::atomic<size_t> g_capW{0}, g_capR{0}, g_playW{0}, g_playR{0};
std::atomic<bool> g_muted{false};

size_t ring_push(std::vector<float>& ring, std::atomic<size_t>& w, std::atomic<size_t>& r,
                 const float* src, size_t frames) {
  const size_t cap = ring.size() / kCh;
  if (cap == 0) return 0;
  size_t wpos = w.load(std::memory_order_relaxed);
  size_t rpos = r.load(std::memory_order_acquire);
  if (wpos < rpos) {
    w.store(rpos, std::memory_order_release);
    wpos = rpos;
  }
  size_t avail = wpos - rpos;
  if (avail >= cap) return 0;
  size_t space = cap - avail;
  size_t n = std::min(frames, space);
  if (n == 0) return 0;

  size_t idx = wpos % cap;
  size_t first = std::min(n, cap - idx);
  size_t second = n - first;
  if (first > 0) {
    std::memcpy(&ring[idx * kCh], src, first * kCh * sizeof(float));
  }
  if (second > 0) {
    std::memcpy(&ring[0], src + first * kCh, second * kCh * sizeof(float));
  }
  w.store(wpos + n, std::memory_order_release);
  return n;
}

size_t ring_pop(std::vector<float>& ring, std::atomic<size_t>& w, std::atomic<size_t>& r,
                float* dst, size_t maxFrames) {
  const size_t cap = ring.size() / kCh;
  if (cap == 0) return 0;
  size_t wpos = w.load(std::memory_order_acquire);
  size_t rpos = r.load(std::memory_order_relaxed);
  if (wpos <= rpos) return 0;
  size_t avail = wpos - rpos;
  if (avail > cap) {
    r.store(wpos, std::memory_order_release);
    return 0;
  }
  size_t n = std::min(maxFrames, avail);
  if (n == 0) return 0;

  size_t idx = rpos % cap;
  size_t first = std::min(n, cap - idx);
  size_t second = n - first;
  if (first > 0) {
    std::memcpy(dst, &ring[idx * kCh], first * kCh * sizeof(float));
  }
  if (second > 0) {
    std::memcpy(dst + first * kCh, &ring[0], second * kCh * sizeof(float));
  }
  r.store(rpos + n, std::memory_order_release);
  return n;
}

std::string stateFilePath() {
  const char* xdg = std::getenv("XDG_STATE_HOME");
  std::string base = (xdg && *xdg) ? std::string(xdg)
                                   : std::string(std::getenv("HOME") ? std::getenv("HOME") : "/tmp") + "/.local/state";
  return base + "/fxsound/previous_sink";
}

std::string readPrevSinkFile() {
  FILE* f = fopen(stateFilePath().c_str(), "r");
  if (!f) return "";
  char buf[512] = {0};
  size_t n = fread(buf, 1, sizeof(buf) - 1, f);
  fclose(f);
  while (n && (buf[n - 1] == '\n' || buf[n - 1] == '\r' || buf[n - 1] == ' ')) buf[--n] = 0;
  return std::string(buf, n);
}

void writePrevSinkFile(const std::string& name) {
  std::string p = stateFilePath();
  std::error_code ec;
  std::filesystem::create_directories(std::filesystem::path(p).parent_path(), ec);
  FILE* f = fopen(p.c_str(), "w");
  if (f) { fprintf(f, "%s\n", name.c_str()); fclose(f); }
}

// Extract {"name": "..."} from a metadata JSON value (spa_json 1.6 API).
std::string jsonNameValue(const char* json) {
  if (!json) return "";
  struct spa_json it[2];
  if (spa_json_begin_object_relax(&it[0], json, strlen(json)) <= 0) return "";
  char key[64], out[512];
  while (spa_json_get_string(&it[0], key, sizeof(key)) > 0) {
    const char* value;
    int len;
    if ((len = spa_json_next(&it[0], &value)) <= 0) break;
    if (strcmp(key, "name") == 0) {
      if (spa_json_parse_stringn(value, len, out, (int)sizeof(out)) > 0)
        return std::string(out);
    }
  }
  return "";
}

struct SinkInfo {
  std::string nodeName;
  std::string description;
  uint32_t nodeId = 0;
};

struct PwState {
  pw_thread_loop* loop = nullptr;
  pw_context* ctx = nullptr;
  pw_core* core = nullptr;
  pw_registry* reg = nullptr;
  pw_proxy* regProxy = nullptr;
  struct spa_hook regListener{};
  struct spa_hook coreListener{};
  pw_metadata* meta = nullptr;
  pw_proxy* metaProxy = nullptr;
  struct spa_hook metaListener{};

  pw_proxy* sinkProxy = nullptr;      // fxsound node proxy
  pw_node* fxNode = nullptr;
  struct spa_hook fxNodeListener{};
  pw_node* realNode = nullptr;        // current real output node
  struct spa_hook realNodeListener{};
  std::string realNodeName;

  pw_stream* capStream = nullptr;
  pw_stream* playStream = nullptr;
  struct spa_hook capListener{};
  struct spa_hook playListener{};

  std::map<uint32_t, SinkInfo> sinks;  // registry cache, loop-locked
  uint32_t fxsoundNodeId = 0;
  std::string defaultSinkName;         // effective default (metadata default.audio.sink)
  std::string prevSink;                // recorded previous default

  std::atomic<bool> coreConnected{false};
  std::atomic<bool> streamError{false};
  std::atomic<bool> shuttingDown{false};

  // volume mirror (Mode A): last fxsound sink volume seen via Props events
  float lastFxVol[8] = {1.0f, 1.0f};
  int lastFxVolCount = 2;
  bool lastFxMute = false;
  bool fxVolumeKnown = false;

  // bidirectional volume sync: last real sink volume and cooldown tracking
  float lastRealVol[8] = {1.0f, 1.0f};
  int lastRealVolCount = 2;
  bool lastRealMute = false;
  bool realVolumeKnown = false;

  std::chrono::steady_clock::time_point lastRealWriteTime{};
  std::chrono::steady_clock::time_point lastFxWriteTime{};

  // atomics block copy assignment; clear by hand
  void reset() {
    loop = nullptr;
    ctx = nullptr;
    core = nullptr;
    reg = nullptr; regProxy = nullptr;
    meta = nullptr; metaProxy = nullptr;
    sinkProxy = nullptr; fxNode = nullptr; realNode = nullptr;
    capStream = playStream = nullptr;
    sinks.clear(); fxsoundNodeId = 0;
    defaultSinkName.clear(); prevSink.clear(); realNodeName.clear();
    coreConnected.store(false); streamError.store(false); shuttingDown.store(false);
    for (auto& v : lastFxVol) v = 1.0f;
    lastFxVolCount = 2; lastFxMute = false; fxVolumeKnown = false;
    for (auto& v : lastRealVol) v = 1.0f;
    lastRealVolCount = 2; lastRealMute = false; realVolumeKnown = false;
    lastRealWriteTime = {}; lastFxWriteTime = {};
  }
};

PwState g;
static PwDeviceCb g_cb;
void destroyStreams();
bool makePlaybackStream();
bool setDefaultMeta(const std::string& nodeName);

void flushBuffersInternal() {
  g_capR.store(g_capW.load(std::memory_order_relaxed), std::memory_order_release);
  g_playR.store(g_playW.load(std::memory_order_relaxed), std::memory_order_release);
}

SinkInfo* findSinkByNodeName(const std::string& name) {
  for (auto& entry : g.sinks)
    if (entry.second.nodeName == name) return &entry.second;
  return nullptr;
}

// ---- core / registry / metadata events ----

void onCoreDone(void*, uint32_t, int) {
  g.coreConnected.store(true, std::memory_order_relaxed);
}

void onCoreError(void*, uint32_t id, int seq, int res, const char* message) {
  if (g_verbose) {
    fprintf(stderr, "[pw] core error id=%u seq=%d res=%d: %s\n", id, seq, res, message ? message : "?");
  }
  if (id == PW_ID_CORE && (res == -EPIPE || res == -ECONNRESET)) {
    g.coreConnected.store(false, std::memory_order_relaxed);
    g.streamError.store(true, std::memory_order_relaxed);
  }
}

const pw_core_events kCoreEvents = []() {
  pw_core_events e{};
  e.version = PW_VERSION_CORE_EVENTS;
  e.done = onCoreDone;
  e.error = onCoreError;
  return e;
}();

void registryGlobal(void*, uint32_t id, uint32_t, const char* type, uint32_t, const struct spa_dict* props) {
  if (strcmp(type, PW_TYPE_INTERFACE_Node) == 0) {
    const char* mediaClass = props ? spa_dict_lookup(props, PW_KEY_MEDIA_CLASS) : nullptr;
    const char* nodeName = props ? spa_dict_lookup(props, PW_KEY_NODE_NAME) : nullptr;
    if (mediaClass && strcmp(mediaClass, "Audio/Sink") == 0 && nodeName) {
      const char* desc = spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION);
      SinkInfo s;
      s.nodeName = nodeName;
      s.description = desc ? desc : nodeName;
      s.nodeId = id;
      bool isFx = (s.nodeName == kFxNodeName);
      g.sinks[id] = s;
      if (isFx) g.fxsoundNodeId = id;
      if (g_verbose)
        printf("[pw] sink added id=%u name=%s (%s)%s\n", id, s.nodeName.c_str(), s.description.c_str(), isFx ? " [fxsound]" : "");
      if (!isFx && g.realNodeName == s.nodeName && g.playStream == nullptr && g.coreConnected.load()) {
        makePlaybackStream();
      }
      if (!isFx && g_cb) {
        g_cb();
      }
    }
  } else if (strcmp(type, PW_TYPE_INTERFACE_Metadata) == 0) {
    const char* name = props ? spa_dict_lookup(props, "metadata.name") : nullptr;
    if (name && strcmp(name, "default") == 0 && g.meta == nullptr) {
      g.metaProxy = (pw_proxy*)pw_registry_bind(g.reg, id, PW_TYPE_INTERFACE_Metadata, PW_VERSION_METADATA, 0);
      g.meta = (pw_metadata*)g.metaProxy;
      if (g.meta) {
        static const pw_metadata_events ev = []() {
          pw_metadata_events e{};
          e.version = PW_VERSION_METADATA_EVENTS;
          e.property = [](void*, uint32_t subject, const char* key, const char*, const char* value) -> int {
            if (key && subject == 0 && g.loop) {
              bool effective = strcmp(key, "default.audio.sink") == 0;
              bool configured = strcmp(key, "default.configured.audio.sink") == 0;
              if (effective || configured) {
                std::string name2 = jsonNameValue(value);
                if (effective) g.defaultSinkName = name2;
                if (g_verbose) printf("[pw] metadata %s -> '%s'\n", key, name2.c_str());
              }
            }
            return 0;
          };
          return e;
        }();
        pw_metadata_add_listener(g.meta, &g.metaListener, &ev, nullptr);
      }
    }
  }
}

void registryGlobalRemove(void*, uint32_t id) {
  auto it = g.sinks.find(id);
  if (it == g.sinks.end()) return;
  bool isFx = (it->second.nodeName == kFxNodeName);
  bool isReal = (it->second.nodeName == g.realNodeName);
  if (isFx) g.fxsoundNodeId = 0;
  if (g_verbose) printf("[pw] sink removed id=%u name=%s\n", id, it->second.nodeName.c_str());
  g.sinks.erase(it);

  if (isReal) {
    if (g.playStream) {
      pw_stream_destroy(g.playStream);
      g.playStream = nullptr;
    }
    if (g.realNode) {
      spa_hook_remove(&g.realNodeListener);
      pw_proxy_destroy((pw_proxy*)g.realNode);
      g.realNode = nullptr;
    }
    flushBuffersInternal();
  }

  if (!isFx && g_cb) {
    g_cb();
  }
}

// ---- bi-directional volume mirror ----

void applyVolumeToReal(bool mute, int nvols, const float* vols) {
  if (!g.realNode || nvols <= 0 || !vols) return;

  nvols = std::min(nvols, 8);
  g.lastRealMute = mute;
  g.lastRealVolCount = nvols;
  for (int i = 0; i < nvols; i++) g.lastRealVol[i] = vols[i];
  g.realVolumeKnown = true;
  g.lastRealWriteTime = std::chrono::steady_clock::now();

  uint8_t buf[1024];
  spa_pod_builder b;
  spa_pod_frame f;
  spa_pod_builder_init(&b, buf, sizeof(buf));
  spa_pod_builder_push_object(&b, &f, SPA_TYPE_OBJECT_Props, SPA_PARAM_Props);
  spa_pod_builder_prop(&b, SPA_PROP_mute, 0);
  spa_pod_builder_bool(&b, mute);
  spa_pod_builder_prop(&b, SPA_PROP_channelVolumes, 0);
  spa_pod_builder_array(&b, sizeof(float), SPA_TYPE_Float, nvols, vols);
  float maxVol = vols[0];
  for (int i = 1; i < nvols; i++) maxVol = std::max(maxVol, vols[i]);
  spa_pod_builder_prop(&b, SPA_PROP_volume, 0);
  spa_pod_builder_float(&b, maxVol);
  const spa_pod* pod = (const spa_pod*)spa_pod_builder_pop(&b, &f);
  if (pod && g.realNode) {
    pw_node_set_param(g.realNode, SPA_PARAM_Props, 0, pod);
  }
}

void applyVolumeToFx(bool mute, int nvols, const float* vols) {
  if (!g.fxNode || nvols <= 0 || !vols) return;

  nvols = std::min(nvols, 8);
  g.lastFxMute = mute;
  g.lastFxVolCount = nvols;
  for (int i = 0; i < nvols; i++) g.lastFxVol[i] = vols[i];
  g.fxVolumeKnown = true;
  g.lastFxWriteTime = std::chrono::steady_clock::now();

  uint8_t buf[1024];
  spa_pod_builder b;
  spa_pod_frame f;
  spa_pod_builder_init(&b, buf, sizeof(buf));
  spa_pod_builder_push_object(&b, &f, SPA_TYPE_OBJECT_Props, SPA_PARAM_Props);
  spa_pod_builder_prop(&b, SPA_PROP_mute, 0);
  spa_pod_builder_bool(&b, mute);
  spa_pod_builder_prop(&b, SPA_PROP_channelVolumes, 0);
  spa_pod_builder_array(&b, sizeof(float), SPA_TYPE_Float, nvols, vols);
  float maxVol = vols[0];
  for (int i = 1; i < nvols; i++) maxVol = std::max(maxVol, vols[i]);
  spa_pod_builder_prop(&b, SPA_PROP_volume, 0);
  spa_pod_builder_float(&b, maxVol);
  const spa_pod* pod = (const spa_pod*)spa_pod_builder_pop(&b, &f);
  if (pod && g.fxNode) {
    pw_node_set_param(g.fxNode, SPA_PARAM_Props, 0, pod);
  }
}

void onFxNodeParam(void*, int, uint32_t id, uint32_t, uint32_t, const struct spa_pod* param) {
  if (id != SPA_PARAM_Props || !param || !g.realNode) return;

  // Never overwrite the real physical sink until its current volume has been learned.
  // The physical device was already playing audio at the user's active volume;
  // fxsound must adopt the real device's volume first.
  if (!g.realVolumeKnown) return;

  auto now = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - g.lastFxWriteTime).count();
  if (elapsed < 350) return;  // Drop echo from our own write to fxNode

  float vols[8] = {0};
  int nvols = 0;
  bool mute = g.lastFxMute;
  float scalarVol = -1.0f;
  const spa_pod_prop* p;
  const spa_pod_object* obj = (const spa_pod_object*)param;
  SPA_POD_OBJECT_FOREACH(obj, p) {
    if (p->key == SPA_PROP_mute) {
      bool m = false;
      if (spa_pod_get_bool(&p->value, &m)) mute = m;
    } else if (p->key == SPA_PROP_channelVolumes) {
      uint32_t n = 0;
      const float* v = (const float*)spa_pod_get_array(&p->value, &n);
      if (v && n > 0) {
        nvols = (int)std::min((uint32_t)8, n);
        for (int i = 0; i < nvols; i++) vols[i] = v[i];
      }
    } else if (p->key == SPA_PROP_volume) {
      float sv = 1.0f;
      if (spa_pod_get_float(&p->value, &sv)) scalarVol = sv;
    }
  }
  if (nvols <= 0 && scalarVol >= 0.0f) {
    nvols = 2;
    vols[0] = vols[1] = scalarVol;
  }
  if (nvols <= 0) return;

  bool fxChanged = (mute != g.lastFxMute) || (nvols != g.lastFxVolCount);
  for (int i = 0; i < nvols && !fxChanged; i++) {
    if (std::abs(vols[i] - g.lastFxVol[i]) > 0.005f) fxChanged = true;
  }
  if (!fxChanged && g.fxVolumeKnown) return;

  g.lastFxMute = mute;
  g.lastFxVolCount = nvols;
  for (int i = 0; i < nvols; i++) g.lastFxVol[i] = vols[i];
  g.fxVolumeKnown = true;

  // Check deadband against realNode's current volume
  bool realDiffers = (mute != g.lastRealMute) || (nvols != g.lastRealVolCount);
  for (int i = 0; i < nvols && !realDiffers; i++) {
    if (std::abs(vols[i] - g.lastRealVol[i]) > 0.005f) realDiffers = true;
  }
  if (!realDiffers && g.realVolumeKnown) return;

  applyVolumeToReal(mute, nvols, vols);
  if (g_verbose) {
    printf("[pw] volume mirror: real '%s' <-", g.realNodeName.c_str());
    for (int i = 0; i < nvols; i++) printf(" %.2f", vols[i]);
    printf(" mute=%d\n", (int)mute);
  }
}

void onRealNodeParam(void*, int, uint32_t id, uint32_t, uint32_t, const struct spa_pod* param) {
  if (id != SPA_PARAM_Props || !param || !g.fxNode) return;

  auto now = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - g.lastRealWriteTime).count();
  if (g.realVolumeKnown && elapsed < 350) return;  // Drop echo from our own write to realNode

  float vols[8] = {0};
  int nvols = 0;
  bool mute = g.lastRealMute;
  float scalarVol = -1.0f;
  const spa_pod_prop* p;
  const spa_pod_object* obj = (const spa_pod_object*)param;
  SPA_POD_OBJECT_FOREACH(obj, p) {
    if (p->key == SPA_PROP_mute) {
      bool m = false;
      if (spa_pod_get_bool(&p->value, &m)) mute = m;
    } else if (p->key == SPA_PROP_channelVolumes) {
      uint32_t n = 0;
      const float* v = (const float*)spa_pod_get_array(&p->value, &n);
      if (v && n > 0) {
        nvols = (int)std::min((uint32_t)8, n);
        for (int i = 0; i < nvols; i++) vols[i] = v[i];
      }
    } else if (p->key == SPA_PROP_volume) {
      float sv = 1.0f;
      if (spa_pod_get_float(&p->value, &sv)) scalarVol = sv;
    }
  }
  if (nvols <= 0 && scalarVol >= 0.0f) {
    nvols = 2;
    vols[0] = vols[1] = scalarVol;
  }
  if (nvols <= 0) return;

  bool realChanged = (mute != g.lastRealMute) || (nvols != g.lastRealVolCount);
  for (int i = 0; i < nvols && !realChanged; i++) {
    if (std::abs(vols[i] - g.lastRealVol[i]) > 0.005f) realChanged = true;
  }
  if (!realChanged && g.realVolumeKnown) return;

  g.lastRealMute = mute;
  g.lastRealVolCount = nvols;
  for (int i = 0; i < nvols; i++) g.lastRealVol[i] = vols[i];
  g.realVolumeKnown = true;

  // Check deadband against fxNode's current volume
  bool fxDiffers = (mute != g.lastFxMute) || (nvols != g.lastFxVolCount);
  for (int i = 0; i < nvols && !fxDiffers; i++) {
    if (std::abs(vols[i] - g.lastFxVol[i]) > 0.005f) fxDiffers = true;
  }
  if (!fxDiffers && g.fxVolumeKnown) return;

  applyVolumeToFx(mute, nvols, vols);

  // Guard default sink: ensure default configured audio sink remains fxsound
  if (!g.shuttingDown.load() && !g.defaultSinkName.empty() && g.defaultSinkName != kFxNodeName) {
    setDefaultMeta(kFxNodeName);
  }

  if (g_verbose) {
    printf("[pw] volume sync: fxsound <- real '%s'", g.realNodeName.c_str());
    for (int i = 0; i < nvols; i++) printf(" %.2f", vols[i]);
    printf(" mute=%d\n", (int)mute);
  }
}

void onRealNodeInfo(void*, const pw_node_info* info) {
  if (!info || !g.realNode) return;
  uint32_t paramId = SPA_PARAM_Props;
  pw_node_subscribe_params(g.realNode, &paramId, 1);
}

const pw_node_events kRealNodeEvents = []() {
  pw_node_events e{};
  e.version = PW_VERSION_NODE_EVENTS;
  e.info = onRealNodeInfo;
  e.param = onRealNodeParam;
  return e;
}();

void onFxNodeInfo(void*, const pw_node_info* info) {
  if (!info || !g.fxNode) return;
  // (Re)subscribe to Props on every info change; idempotent.
  uint32_t paramId = SPA_PARAM_Props;
  pw_node_subscribe_params(g.fxNode, &paramId, 1);
}

const pw_node_events kFxNodeEvents = []() {
  pw_node_events e{};
  e.version = PW_VERSION_NODE_EVENTS;
  e.info = onFxNodeInfo;
  e.param = onFxNodeParam;
  return e;
}();

// ---- streams ----

void capProcess(void*) {
  pw_buffer* b = pw_stream_dequeue_buffer(g.capStream);
  if (!b || !b->buffer) return;
  spa_data* d = &b->buffer->datas[0];
  if (d->data && d->chunk && d->chunk->size > 0) {
    const float* src = (const float*)((const uint8_t*)d->data + d->chunk->offset);
    size_t frames = d->chunk->size / (kCh * sizeof(float));
    size_t pushed = ring_push(g_capRing, g_capW, g_capR, src, frames);
    if (pushed < frames) g_drift.overruns++;  // worker behind; newest dropped
  }
  pw_stream_queue_buffer(g.capStream, b);
}

void playProcess(void*) {
  pw_buffer* b = pw_stream_dequeue_buffer(g.playStream);
  if (!b || !b->buffer) return;
  spa_data* d = &b->buffer->datas[0];
  if (d->data) {
    size_t maxFrames = d->maxsize / (kCh * sizeof(float));
    size_t frames = std::min(maxFrames, (size_t)(b->requested ? b->requested : maxFrames));
    size_t got = ring_pop(g_playRing, g_playW, g_playR, (float*)d->data, frames);
    if (got < frames) {
      memset((float*)d->data + got * kCh, 0, (frames - got) * kCh * sizeof(float));
      g_drift.underruns++;
    }
    d->chunk->offset = 0;
    d->chunk->stride = (uint32_t)(kCh * sizeof(float));
    d->chunk->size = (uint32_t)(frames * kCh * sizeof(float));
  }
  pw_stream_queue_buffer(g.playStream, b);
}

void playStateChanged(void*, enum pw_stream_state old, enum pw_stream_state state, const char* error) {
  if (g_verbose) {
    printf("[pw] playStream state %s -> %s%s%s\n",
           pw_stream_state_as_string(old), pw_stream_state_as_string(state),
           error ? ": " : "", error ? error : "");
  }
  if (state == PW_STREAM_STATE_STREAMING || state == PW_STREAM_STATE_PAUSED) {
    g.streamError.store(false, std::memory_order_relaxed);
  } else if (state == PW_STREAM_STATE_ERROR) {
    g.streamError.store(true, std::memory_order_relaxed);
  }
}

const pw_stream_events kCapEvents = []() {
  pw_stream_events e{};
  e.version = PW_VERSION_STREAM_EVENTS;
  e.process = capProcess;
  return e;
}();

const pw_stream_events kPlayEvents = []() {
  pw_stream_events e{};
  e.version = PW_VERSION_STREAM_EVENTS;
  e.process = playProcess;
  e.state_changed = playStateChanged;
  return e;
}();

spa_audio_info_raw fxCapsRaw() {
  spa_audio_info_raw raw{};
  raw.format = SPA_AUDIO_FORMAT_F32;
  raw.rate = 48000;
  raw.channels = 2;
  raw.position[0] = SPA_AUDIO_CHANNEL_FL;
  raw.position[1] = SPA_AUDIO_CHANNEL_FR;
  return raw;
}

bool connectStream(pw_stream* s, pw_direction dir) {
  uint8_t paramBuf[1024];
  spa_pod_builder b;
  spa_pod_builder_init(&b, paramBuf, sizeof(paramBuf));
  spa_audio_info_raw raw = fxCapsRaw();
  const spa_pod* params[1] = {spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &raw)};
  return pw_stream_connect(s, dir, PW_ID_ANY,
                           (pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS | PW_STREAM_FLAG_RT_PROCESS),
                           params, 1) >= 0;
}

bool makeStreams() {
  if (g.realNodeName.empty() || g.realNodeName == kFxNodeName) return false;
  if (!findSinkByNodeName(g.realNodeName)) return false;

  pw_properties* cp = pw_properties_new(
      PW_KEY_MEDIA_TYPE, "Audio",
      PW_KEY_MEDIA_CATEGORY, "Capture",
      PW_KEY_MEDIA_ROLE, "Music",
      PW_KEY_STREAM_CAPTURE_SINK, "true",
      PW_KEY_TARGET_OBJECT, kFxNodeName,
      PW_KEY_NODE_NAME, "fxsound_capture",
      PW_KEY_NODE_LATENCY, "1024/48000",
      PW_KEY_NODE_DONT_RECONNECT, "true",
      "node.dont-fallback", "true",
      nullptr);
  g.capStream = pw_stream_new_simple(pw_thread_loop_get_loop(g.loop), "fxsound-capture", cp, &kCapEvents, nullptr);

  pw_properties* pp = pw_properties_new(
      PW_KEY_MEDIA_TYPE, "Audio",
      PW_KEY_MEDIA_CATEGORY, "Playback",
      PW_KEY_MEDIA_ROLE, "Music",
      PW_KEY_TARGET_OBJECT, g.realNodeName.c_str(),
      PW_KEY_NODE_NAME, "fxsound_playback",
      PW_KEY_NODE_LATENCY, "1024/48000",
      PW_KEY_NODE_DONT_RECONNECT, "true",
      "node.dont-fallback", "true",
      nullptr);
  g.playStream = pw_stream_new_simple(pw_thread_loop_get_loop(g.loop), "fxsound-playback", pp, &kPlayEvents, nullptr);

  if (!g.capStream || !g.playStream) {
    destroyStreams();
    return false;
  }

  if (!connectStream(g.capStream, PW_DIRECTION_INPUT)) {
    destroyStreams();
    return false;
  }
  if (!connectStream(g.playStream, PW_DIRECTION_OUTPUT)) {
    destroyStreams();
    return false;
  }
  return true;
}

void destroyStreams() {
  if (g.capStream) { pw_stream_destroy(g.capStream); g.capStream = nullptr; }
  if (g.playStream) { pw_stream_destroy(g.playStream); g.playStream = nullptr; }
}

bool makePlaybackStream() {
  // Output-switch path: recreate only the playback stream.
  if (g.playStream) { pw_stream_destroy(g.playStream); g.playStream = nullptr; }
  if (g.realNodeName.empty() || g.realNodeName == kFxNodeName) return false;
  if (!findSinkByNodeName(g.realNodeName)) return false;

  pw_properties* pp = pw_properties_new(
      PW_KEY_MEDIA_TYPE, "Audio",
      PW_KEY_MEDIA_CATEGORY, "Playback",
      PW_KEY_MEDIA_ROLE, "Music",
      PW_KEY_TARGET_OBJECT, g.realNodeName.c_str(),
      PW_KEY_NODE_NAME, "fxsound_playback",
      PW_KEY_NODE_LATENCY, "1024/48000",
      PW_KEY_NODE_DONT_RECONNECT, "true",
      "node.dont-fallback", "true",
      nullptr);
  g.playStream = pw_stream_new_simple(pw_thread_loop_get_loop(g.loop), "fxsound-playback", pp, &kPlayEvents, nullptr);
  if (!g.playStream) return false;
  if (!connectStream(g.playStream, PW_DIRECTION_OUTPUT)) {
    pw_stream_destroy(g.playStream);
    g.playStream = nullptr;
    return false;
  }
  return true;
}

bool makeSink() {
  pw_properties* sp = pw_properties_new(
      PW_KEY_FACTORY_NAME, "support.null-audio-sink",  // required by core create_object on 1.x
      PW_KEY_MEDIA_CLASS, "Audio/Sink",
      PW_KEY_NODE_NAME, kFxNodeName,
      PW_KEY_NODE_DESCRIPTION, kFxDescription,
      PW_KEY_OBJECT_LINGER, "false",
      PW_KEY_NODE_PAUSE_ON_IDLE, "false",
      "session.suspend-timeout-seconds", "0",
      SPA_KEY_AUDIO_POSITION, "FL,FR",
      "monitor.channel-volumes", "false",
      PW_KEY_NODE_LATENCY, "1024/48000",
      nullptr);
  g.sinkProxy = (pw_proxy*)pw_core_create_object(g.core, "adapter", PW_TYPE_INTERFACE_Node, PW_VERSION_NODE, &sp->dict, 0);
  pw_properties_free(sp);
  return g.sinkProxy != nullptr;
}

std::string currentDefaultFromWpctl() {
  if (!g.defaultSinkName.empty()) return g.defaultSinkName;
  int pfd[2];
  if (pipe(pfd) != 0) return "";
  pid_t pid = fork();
  if (pid == 0) {
    close(pfd[0]);
    dup2(pfd[1], STDOUT_FILENO);
    int devnull = open("/dev/null", O_WRONLY);
    if (devnull >= 0) { dup2(devnull, STDERR_FILENO); close(devnull); }
    close(pfd[1]);
    execlp("pw-metadata", "pw-metadata", "-n", "default", "0", "default.audio.sink", nullptr);
    _exit(127);
  }
  close(pfd[1]);
  if (pid < 0) { close(pfd[0]); return ""; }
  char buf[1024] = {0};
  ssize_t n = read(pfd[0], buf, sizeof(buf) - 1);
  close(pfd[0]);
  waitpid(pid, nullptr, 0);
  if (n <= 0) return "";
  char* v = strstr(buf, "value:'");
  if (!v) return "";
  v += 7;
  char* end = strchr(v, '\'');
  if (!end) return "";
  *end = 0;
  return jsonNameValue(v);
}

void setDefaultViaWpctl(const std::string& nodeName) {
  uint32_t id = 0;
  if (nodeName == kFxNodeName) {
    id = g.fxsoundNodeId;
  } else {
    SinkInfo* s = findSinkByNodeName(nodeName);
    if (s) id = s->nodeId;
  }
  if (!id) return;
  std::string idStr = std::to_string(id);
  pid_t pid = fork();
  if (pid == 0) {
    int devnull = open("/dev/null", O_WRONLY);
    if (devnull >= 0) { dup2(devnull, STDOUT_FILENO); dup2(devnull, STDERR_FILENO); close(devnull); }
    execlp("wpctl", "wpctl", "set-default", idStr.c_str(), nullptr);
    _exit(127);
  } else if (pid > 0) {
    waitpid(pid, nullptr, 0);
  }
}

bool setDefaultMeta(const std::string& nodeName) {
  if (!g.meta) return false;
  std::string json = std::string("{\"name\": \"") + nodeName + "\"}";
  pw_metadata_set_property(g.meta, 0, "default.configured.audio.sink", "Spa:String:JSON", json.c_str());
  return true;
}

}  // namespace

// ================= exported API =================

void pwSetVerbose(bool v) { g_verbose = v; }
bool pwVerbose() { return g_verbose; }

void pwSetDeviceCallback(PwDeviceCb cb) { g_cb = std::move(cb); }
PwDrift pwDrift() { return g_drift; }
float pwFxsoundVolume() {
  if (g.lastFxMute) return 0.0f;
  float v = g.lastFxVolCount > 0 ? g.lastFxVol[0] : 1.0f;
  return v < 0.02f ? 0.02f : v;
}

bool pwStartup(const std::string& prevSink, int /*bufferMs*/) {
  if (g.loop) return true;  // already running

  if (const char* v = std::getenv("FXSOUND_PW_VERBOSE")) g_verbose = (*v != '0');
  pw_init(nullptr, nullptr);
  g_capRing.assign(kMaxQ * kCh, 0.0f);
  g_playRing.assign(kMaxQ * kCh, 0.0f);

  g.loop = pw_thread_loop_new("fxsound-pw", nullptr);
  if (!g.loop) return false;
  if (pw_thread_loop_start(g.loop) != 0) return false;

  pw_thread_loop_lock(g.loop);
  g.ctx = pw_context_new(pw_thread_loop_get_loop(g.loop), nullptr, 0);
  if (!g.ctx) { pw_thread_loop_unlock(g.loop); return false; }
  g.core = pw_context_connect(g.ctx, nullptr, 0);
  if (!g.core) { pw_thread_loop_unlock(g.loop); return false; }

  static pw_core_events coreEv = kCoreEvents;
  pw_core_add_listener(g.core, &g.coreListener, &coreEv, nullptr);

  g.reg = pw_core_get_registry(g.core, PW_VERSION_REGISTRY, 0);
  g.regProxy = (pw_proxy*)g.reg;
  static pw_registry_events regEv = []() {
    pw_registry_events e{};
    e.version = PW_VERSION_REGISTRY_EVENTS;
    e.global = registryGlobal;
    e.global_remove = registryGlobalRemove;
    return e;
  }();
  pw_registry_add_listener(g.reg, &g.regListener, &regEv, nullptr);

  // previous default: caller-provided, else state file
  std::string prev = prevSink.empty() ? readPrevSinkFile() : prevSink;
  if (prev == kFxNodeName) prev.clear();
  g.prevSink = prev;

  // create the fxsound virtual sink
  if (!makeSink()) { pw_thread_loop_unlock(g.loop); return false; }

  // wait for the registry to report it (up to 10s); metadata events settle too
  for (int i = 0; i < 100 && (!g.fxsoundNodeId || g.defaultSinkName.empty()); i++) {
    pw_thread_loop_unlock(g.loop);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    pw_thread_loop_lock(g.loop);
    pw_core_sync(g.core, 0, 1);
  }
  if (!g.fxsoundNodeId) { pw_thread_loop_unlock(g.loop); return false; }

  // record previous default: if the saved one is missing or stale, adopt active default
  if (g.prevSink.empty() || !findSinkByNodeName(g.prevSink)) {
    g.prevSink = g.defaultSinkName.empty() ? currentDefaultFromWpctl() : g.defaultSinkName;
    if (g.prevSink == kFxNodeName) g.prevSink = "";
  }
  if (!g.prevSink.empty()) writePrevSinkFile(g.prevSink);
  g.realNodeName = g.prevSink;
  if (g.realNodeName.empty() || !findSinkByNodeName(g.realNodeName)) {
    for (auto& entry : g.sinks)
      if (entry.second.nodeName != kFxNodeName) { g.realNodeName = entry.second.nodeName; break; }
  }

  // bind fxsound node for volume events
  g.fxNode = (pw_node*)pw_registry_bind(g.reg, g.fxsoundNodeId, PW_TYPE_INTERFACE_Node, PW_VERSION_NODE, 0);
  if (g.fxNode) {
    static pw_node_events fxNodeEv = kFxNodeEvents;
    pw_node_add_listener(g.fxNode, &g.fxNodeListener, &fxNodeEv, nullptr);
  }

  // bind the real output node (for bi-directional volume sync)
  SinkInfo* rs = findSinkByNodeName(g.realNodeName);
  if (rs) {
    g.realNode = (pw_node*)pw_registry_bind(g.reg, rs->nodeId, PW_TYPE_INTERFACE_Node, PW_VERSION_NODE, 0);
    if (g.realNode) {
      static pw_node_events realNodeEv = kRealNodeEvents;
      pw_node_add_listener(g.realNode, &g.realNodeListener, &realNodeEv, nullptr);
    }
  }

  // Synchronize with PipeWire to ensure the real node's initial volume is received
  // and applied to the fxsound virtual sink before fxsound becomes default sink.
  for (int i = 0; i < 20 && !g.realVolumeKnown; i++) {
    pw_thread_loop_unlock(g.loop);
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
    pw_thread_loop_lock(g.loop);
    pw_core_sync(g.core, 0, 1);
  }

  bool ok = makeStreams();
  if (ok) {
    // make fxsound the default sink
    if (!setDefaultMeta(kFxNodeName)) setDefaultViaWpctl(kFxNodeName);
  }
  pw_thread_loop_unlock(g.loop);

  if (g_verbose)
    printf("[pw] startup %s (prev='%s' real='%s')\n", ok ? "OK" : "FAILED", g.prevSink.c_str(), g.realNodeName.c_str());
  return ok;
}

void pwShutdown(const std::string& prevSink) {
  if (!g.loop) return;
  g.shuttingDown.store(true);

  // restore the previous default only if the current default is still fxsound
  bool currentIsFxsound = g.defaultSinkName.empty() || g.defaultSinkName == kFxNodeName;
  pwRestorePrevious(prevSink.empty() ? g.prevSink : prevSink, currentIsFxsound);

  pw_thread_loop_lock(g.loop);
  destroyStreams();
  if (g.fxNode) {
    spa_hook_remove(&g.fxNodeListener);
    pw_proxy_destroy((pw_proxy*)g.fxNode);
    g.fxNode = nullptr;
  }
  if (g.realNode) {
    spa_hook_remove(&g.realNodeListener);
    pw_proxy_destroy((pw_proxy*)g.realNode);
    g.realNode = nullptr;
  }
  if (g.sinkProxy) { pw_proxy_destroy(g.sinkProxy); g.sinkProxy = nullptr; }
  if (g.metaProxy) { pw_proxy_destroy(g.metaProxy); g.meta = nullptr; g.metaProxy = nullptr; }
  if (g.regProxy) { pw_proxy_destroy(g.regProxy); g.reg = nullptr; g.regProxy = nullptr; }
  pw_thread_loop_unlock(g.loop);

  pw_thread_loop_stop(g.loop);
  pw_core_disconnect(g.core);
  pw_context_destroy(g.ctx);
  pw_thread_loop_destroy(g.loop);
  g.reset();
  pw_deinit();
}

bool pwConnectedAndSinkReady() {
  if (!g.loop || !g.core || g.fxsoundNodeId == 0) {
    return false;
  }
  pw_thread_loop_lock(g.loop);
  bool ready = false;
  if (!g.realNodeName.empty() && g.playStream != nullptr && findSinkByNodeName(g.realNodeName) != nullptr) {
    enum pw_stream_state state = pw_stream_get_state(g.playStream, nullptr);
    ready = (state != PW_STREAM_STATE_ERROR && state != PW_STREAM_STATE_UNCONNECTED);
  }
  pw_thread_loop_unlock(g.loop);
  return ready;
}

std::vector<PwDevice> pwListSinks() {
  std::vector<PwDevice> out;
  if (!g.loop) return out;
  pw_thread_loop_lock(g.loop);
  for (auto& entry : g.sinks) {
    if (entry.second.nodeName == kFxNodeName) continue;  // never list fxsound as an output
    PwDevice d;
    d.nodeName = entry.second.nodeName;
    d.description = entry.second.description;
    d.isDefault = (entry.second.nodeName == g.defaultSinkName);
    d.isFxsound = false;
    out.push_back(d);
  }
  pw_thread_loop_unlock(g.loop);
  return out;
}

bool pwSetOutput(const std::string& nodeName) {
  if (nodeName.empty() || nodeName == kFxNodeName) return false;  // never feedback
  if (!g.loop) return false;
  pw_thread_loop_lock(g.loop);
  if (nodeName == g.realNodeName && g.playStream != nullptr) {
    pw_thread_loop_unlock(g.loop);
    return true;
  }

  SinkInfo* s = findSinkByNodeName(nodeName);
  if (!s) { pw_thread_loop_unlock(g.loop); return false; }

  if (g.realNode) {
    spa_hook_remove(&g.realNodeListener);
    pw_proxy_destroy((pw_proxy*)g.realNode);
    g.realNode = nullptr;
  }
  g.realNodeName = nodeName;
  g.realNode = (pw_node*)pw_registry_bind(g.reg, s->nodeId, PW_TYPE_INTERFACE_Node, PW_VERSION_NODE, 0);
  if (g.realNode) {
    static pw_node_events realNodeEv = kRealNodeEvents;
    pw_node_add_listener(g.realNode, &g.realNodeListener, &realNodeEv, nullptr);
  }

  flushBuffersInternal();
  bool ok = makePlaybackStream();
  if (ok) g.realVolumeKnown = false;  // re-learn volumes before next write
  pw_thread_loop_unlock(g.loop);
  if (g_verbose) printf("[pw] output switched to '%s' (%s)\n", nodeName.c_str(), ok ? "ok" : "FAILED");
  return ok;
}

std::string pwGetSelectedOutput() {
  if (!g.loop) return "";
  pw_thread_loop_lock(g.loop);
  std::string name = g.realNodeName;
  pw_thread_loop_unlock(g.loop);
  return name;
}

bool pwSetDefaultFxsound() {
  if (!g.loop) return false;
  pw_thread_loop_lock(g.loop);
  bool ok = setDefaultMeta(kFxNodeName);
  if (!ok) setDefaultViaWpctl(kFxNodeName);
  pw_thread_loop_unlock(g.loop);
  return ok;
}

bool pwRestorePrevious(const std::string& prevSink, bool currentIsFxsound) {
  if (!currentIsFxsound) return true;  // never overwrite the user's own choice
  if (!g.loop) return false;
  std::string target = prevSink.empty() ? g.prevSink : prevSink;
  if (target.empty() || target == kFxNodeName) {
    target = g.realNodeName;
  }
  if (target.empty() || target == kFxNodeName) {
    for (auto& entry : g.sinks) {
      if (entry.second.nodeName != kFxNodeName) { target = entry.second.nodeName; break; }
    }
  }
  pw_thread_loop_lock(g.loop);
  bool ok = true;
  if (!target.empty()) {
    ok = setDefaultMeta(target);
    if (!ok) setDefaultViaWpctl(target);
  }
  pw_thread_loop_unlock(g.loop);
  if (g_verbose) printf("[pw] restore default -> '%s' (%s)\n", target.c_str(), ok ? "meta" : "wpctl/skip");
  return ok;
}

void pwMirrorVolumeTick() {
  // Volume sync is fully event-driven with cooldown and deadband.
}

void pwFlushBuffers() {
  flushBuffersInternal();
}

size_t pwCaptureRead(float* dst, size_t maxFrames) {
  return ring_pop(g_capRing, g_capW, g_capR, dst, maxFrames);
}

size_t pwPlaybackWrite(const float* src, size_t frames) {
  size_t n = ring_push(g_playRing, g_playW, g_playR, src, frames);
  if (n < frames) g_drift.corrections++;  // playback ring full -> drop surplus
  return n;
}

void pwSetFxsoundMuted(bool m) { g_muted.store(m); }
bool pwIsFxsoundMuted() { return g_muted.load(); }

void sndDevicesPwPlaceholder() {}
}  // namespace fxsound_linux

#else  // !__linux__ — parse-only stub; never compiled on Windows.
#include "sndDevices_pw.h"
namespace fxsound_linux {
void sndDevicesPwPlaceholder() {}
}  // namespace fxsound_linux
#endif

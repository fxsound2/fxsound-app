/*
FxSound — Linux build: StatusNotifierItem implementation (GDBus).

Runs a private GLib main loop on its own thread, owns a bus name
org.kde.StatusNotifierItem-<pid>-1, exports /StatusNotifierItem implementing the
org.kde.StatusNotifierItem interface, and registers with the
org.kde.StatusNotifierWatcher. Left click -> Activate, right click ->
ContextMenu; both call back into the app.
*/

#include "fx_tray_sni.h"

#include <gio/gio.h>
#include <glib.h>
#include <unistd.h>
#include <thread>

namespace {

const char* kIntrospection =
    "<node>"
    "  <interface name='org.kde.StatusNotifierItem'>"
    "    <property name='Category' type='s' access='read'/>"
    "    <property name='Id' type='s' access='read'/>"
    "    <property name='Title' type='s' access='read'/>"
    "    <property name='Status' type='s' access='read'/>"
    "    <property name='IconName' type='s' access='read'/>"
    "    <property name='IconPixmap' type='a(iiay)' access='read'/>"
    "    <property name='IconThemePath' type='s' access='read'/>"
    "    <property name='ItemIsMenu' type='b' access='read'/>"
    "    <property name='Menu' type='o' access='read'/>"
    "    <method name='Activate'>"
    "      <arg name='x' type='i' direction='in'/>"
    "      <arg name='y' type='i' direction='in'/>"
    "    </method>"
    "    <method name='SecondaryActivate'>"
    "      <arg name='x' type='i' direction='in'/>"
    "      <arg name='y' type='i' direction='in'/>"
    "    </method>"
    "    <method name='ContextMenu'>"
    "      <arg name='x' type='i' direction='in'/>"
    "      <arg name='y' type='i' direction='in'/>"
    "    </method>"
    "    <method name='Scroll'>"
    "      <arg name='delta' type='i' direction='in'/>"
    "      <arg name='orientation' type='s' direction='in'/>"
    "    </method>"
    "    <signal name='NewIcon'/>"
    "    <signal name='NewStatus'><arg name='status' type='s'/></signal>"
    "    <signal name='NewTitle'/>"
    "  </interface>"
    "</node>";

} // namespace

struct FxTraySNI::Impl
{
    std::function<void()> on_activate;
    std::function<void()> on_context_menu;
    std::string icon_name = "fxsound";
    std::string icon_theme_path;
    int px_w = 0, px_h = 0;
    std::vector<unsigned char> px_argb;   // RGBA bytes (R,G,B,A per pixel)

    GMainContext* ctx = nullptr;
    GMainLoop*    loop = nullptr;
    std::thread   thread;
    GDBusConnection* conn = nullptr;
    guint owner_id = 0;
    guint reg_id = 0;
    bool active = false;
    std::string bus_name;

    static void handleMethod(GDBusConnection*, const gchar*, const gchar*,
                             const gchar*, const gchar* method,
                             GVariant*, GDBusMethodInvocation* inv, gpointer user)
    {
        auto* self = static_cast<Impl*>(user);
        if (g_strcmp0(method, "Activate") == 0 || g_strcmp0(method, "SecondaryActivate") == 0)
        {
            if (self->on_activate) self->on_activate();
        }
        else if (g_strcmp0(method, "ContextMenu") == 0)
        {
            if (self->on_context_menu) self->on_context_menu();
        }
        // Scroll: ignored.
        g_dbus_method_invocation_return_value(inv, nullptr);
    }

    static GVariant* getProperty(GDBusConnection*, const gchar*, const gchar*,
                                 const gchar*, const gchar* prop,
                                 GError**, gpointer user)
    {
        auto* self = static_cast<Impl*>(user);
        if (g_strcmp0(prop, "Category") == 0)      return g_variant_new_string("ApplicationStatus");
        if (g_strcmp0(prop, "Id") == 0)            return g_variant_new_string("fxsound");
        if (g_strcmp0(prop, "Title") == 0)         return g_variant_new_string("FxSound");
        if (g_strcmp0(prop, "Status") == 0)        return g_variant_new_string("Active");
        if (g_strcmp0(prop, "IconName") == 0)      return g_variant_new_string(self->icon_name.c_str());
        if (g_strcmp0(prop, "IconThemePath") == 0)
            return g_variant_new_string(self->icon_theme_path.c_str());
        if (g_strcmp0(prop, "IconPixmap") == 0) {
            GVariantBuilder outer; g_variant_builder_init(&outer, G_VARIANT_TYPE("a(iiay)"));
            if (!self->px_argb.empty()) {
                GVariantBuilder bytes; g_variant_builder_init(&bytes, G_VARIANT_TYPE("ay"));
                for (unsigned char b : self->px_argb)
                    g_variant_builder_add(&bytes, "y", b);
                g_variant_builder_add(&outer, "(iiay)", self->px_w, self->px_h, &bytes);
            }
            return g_variant_builder_end(&outer);
        }
        if (g_strcmp0(prop, "ItemIsMenu") == 0)    return g_variant_new_boolean(FALSE);
        if (g_strcmp0(prop, "Menu") == 0)          return g_variant_new_object_path("/NO_DBUSMENU");
        return nullptr;
    }

    void registerWithWatcher()
    {
        // Register with our connection's unique name (always owned once
        // connected); the watcher uses it + the well-known /StatusNotifierItem
        // object path. This avoids a race on owning the well-known name.
        const char* unique = g_dbus_connection_get_unique_name(conn);
        GError* err = nullptr;
        GVariant* res = g_dbus_connection_call_sync(
            conn, "org.kde.StatusNotifierWatcher", "/StatusNotifierWatcher",
            "org.kde.StatusNotifierWatcher", "RegisterStatusNotifierItem",
            g_variant_new("(s)", unique ? unique : bus_name.c_str()),
            nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &err);
        if (res) { g_variant_unref(res); active = true; }
        if (err) g_error_free(err);
    }

    static void onBusAcquired(GDBusConnection* connection, const gchar*, gpointer user)
    {
        auto* self = static_cast<Impl*>(user);
        self->conn = connection;

        static const GDBusInterfaceVTable vtable = {
            handleMethod, getProperty, nullptr, { nullptr }
        };
        GDBusNodeInfo* node = g_dbus_node_info_new_for_xml(kIntrospection, nullptr);
        self->reg_id = g_dbus_connection_register_object(
            connection, "/StatusNotifierItem", node->interfaces[0],
            &vtable, self, nullptr, nullptr);
        g_dbus_node_info_unref(node);

        self->registerWithWatcher();
    }

    void run()
    {
        ctx = g_main_context_new();
        g_main_context_push_thread_default(ctx);
        loop = g_main_loop_new(ctx, FALSE);

        bus_name = "org.kde.StatusNotifierItem-" + std::to_string(getpid()) + "-1";
        owner_id = g_bus_own_name(G_BUS_TYPE_SESSION, bus_name.c_str(),
                                  G_BUS_NAME_OWNER_FLAGS_NONE,
                                  onBusAcquired, nullptr, nullptr, this, nullptr);

        g_main_loop_run(loop);

        if (owner_id) g_bus_unown_name(owner_id);
        if (loop) g_main_loop_unref(loop);
        g_main_context_pop_thread_default(ctx);
        if (ctx) g_main_context_unref(ctx);
    }

    void emitSignal(const char* name, GVariant* params)
    {
        if (!conn) return;
        g_dbus_connection_emit_signal(conn, nullptr, "/StatusNotifierItem",
                                      "org.kde.StatusNotifierItem", name, params, nullptr);
    }
};

FxTraySNI::FxTraySNI(std::function<void()> on_activate,
                     std::function<void()> on_context_menu)
    : impl_(std::make_unique<Impl>())
{
    impl_->on_activate = std::move(on_activate);
    impl_->on_context_menu = std::move(on_context_menu);
    impl_->thread = std::thread([this] { impl_->run(); });
}

FxTraySNI::~FxTraySNI()
{
    if (impl_ && impl_->loop)
        g_main_loop_quit(impl_->loop);
    if (impl_ && impl_->thread.joinable())
        impl_->thread.join();
}

bool FxTraySNI::isActive() const { return impl_ && impl_->active; }

void FxTraySNI::setIconName(const std::string& icon_name)
{
    if (!impl_) return;
    impl_->icon_name = icon_name;
    impl_->emitSignal("NewIcon", nullptr);
}

void FxTraySNI::setIconThemePath(const std::string& path)
{
    if (!impl_) return;
    impl_->icon_theme_path = path;
    impl_->emitSignal("NewIcon", nullptr);
}

void FxTraySNI::setIconPixmap(int width, int height, std::vector<unsigned char> argb)
{
    if (!impl_) return;
    impl_->px_w = width;
    impl_->px_h = height;
    impl_->px_argb = std::move(argb);
    impl_->emitSignal("NewIcon", nullptr);
}

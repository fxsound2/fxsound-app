/*
FxSound — Linux tray: StatusNotifierItem + com.canonical.dbusmenu (GDBus).

One GLib main loop / DBus connection hosts both interfaces so Wayland panels
(Waybar, KDE Plasma tray, etc.) can render the context menu natively.

SNI object path  : /StatusNotifierItem
dbusmenu path    : /com/fxsound/MenuBar
Bus name         : org.kde.StatusNotifierItem-<pid>-1
*/

#include "fx_tray_sni.h"

#include <gio/gio.h>
#include <glib.h>
#include <mutex>
#include <thread>
#include <unistd.h>

// ---------------------------------------------------------------------------
// DBus introspection XML
// ---------------------------------------------------------------------------

static const char* kSNIXml =
    "<node>"
    " <interface name='org.kde.StatusNotifierItem'>"
    "  <property name='Category'      type='s'       access='read'/>"
    "  <property name='Id'            type='s'       access='read'/>"
    "  <property name='Title'         type='s'       access='read'/>"
    "  <property name='Status'        type='s'       access='read'/>"
    "  <property name='IconName'      type='s'       access='read'/>"
    "  <property name='IconPixmap'    type='a(iiay)' access='read'/>"
    "  <property name='IconThemePath' type='s'       access='read'/>"
    "  <property name='ItemIsMenu'    type='b'       access='read'/>"
    "  <property name='Menu'          type='o'       access='read'/>"
    "  <method name='Activate'>"
    "   <arg name='x' type='i' direction='in'/>"
    "   <arg name='y' type='i' direction='in'/>"
    "  </method>"
    "  <method name='SecondaryActivate'>"
    "   <arg name='x' type='i' direction='in'/>"
    "   <arg name='y' type='i' direction='in'/>"
    "  </method>"
    "  <method name='ContextMenu'>"
    "   <arg name='x' type='i' direction='in'/>"
    "   <arg name='y' type='i' direction='in'/>"
    "  </method>"
    "  <method name='Scroll'>"
    "   <arg name='delta'       type='i' direction='in'/>"
    "   <arg name='orientation' type='s' direction='in'/>"
    "  </method>"
    "  <signal name='NewIcon'/>"
    "  <signal name='NewStatus'><arg name='status' type='s'/></signal>"
    "  <signal name='NewTitle'/>"
    " </interface>"
    "</node>";

static const char* kMenuXml =
    "<node>"
    " <interface name='com.canonical.dbusmenu'>"
    "  <property name='Version'       type='u'  access='read'/>"
    "  <property name='TextDirection' type='s'  access='read'/>"
    "  <property name='Status'        type='s'  access='read'/>"
    "  <property name='IconThemePath' type='as' access='read'/>"
    "  <method name='GetLayout'>"
    "   <arg name='parentId'       type='i'         direction='in'/>"
    "   <arg name='recursionDepth' type='i'         direction='in'/>"
    "   <arg name='propertyNames'  type='as'        direction='in'/>"
    "   <arg name='revision'       type='u'         direction='out'/>"
    "   <arg name='layout'         type='(ia{sv}av)' direction='out'/>"
    "  </method>"
    "  <method name='GetGroupProperties'>"
    "   <arg name='ids'           type='ai'       direction='in'/>"
    "   <arg name='propertyNames' type='as'       direction='in'/>"
    "   <arg name='properties'    type='a(ia{sv})' direction='out'/>"
    "  </method>"
    "  <method name='GetProperty'>"
    "   <arg name='id'    type='i' direction='in'/>"
    "   <arg name='name'  type='s' direction='in'/>"
    "   <arg name='value' type='v' direction='out'/>"
    "  </method>"
    "  <method name='Event'>"
    "   <arg name='id'        type='i'  direction='in'/>"
    "   <arg name='eventId'   type='s'  direction='in'/>"
    "   <arg name='data'      type='v'  direction='in'/>"
    "   <arg name='timestamp' type='u'  direction='in'/>"
    "  </method>"
    "  <method name='EventGroup'>"
    "   <arg name='events'   type='a(isvu)' direction='in'/>"
    "   <arg name='idErrors' type='ai'      direction='out'/>"
    "  </method>"
    "  <method name='AboutToShow'>"
    "   <arg name='id'         type='i' direction='in'/>"
    "   <arg name='needUpdate' type='b' direction='out'/>"
    "  </method>"
    "  <method name='AboutToShowGroup'>"
    "   <arg name='ids'          type='ai' direction='in'/>"
    "   <arg name='updatesNeeded' type='ai' direction='out'/>"
    "   <arg name='idErrors'     type='ai' direction='out'/>"
    "  </method>"
    "  <signal name='ItemsPropertiesUpdated'>"
    "   <arg name='updatedProps' type='a(ia{sv})'/>"
    "   <arg name='removedProps' type='a(ias)'/>"
    "  </signal>"
    "  <signal name='LayoutUpdated'>"
    "   <arg name='revision' type='u'/>"
    "   <arg name='parent'   type='i'/>"
    "  </signal>"
    "  <signal name='ItemActivationRequested'>"
    "   <arg name='id'        type='i'/>"
    "   <arg name='timestamp' type='u'/>"
    "  </signal>"
    " </interface>"
    "</node>";

static const char* kMenuPath = "/com/fxsound/MenuBar";

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string wstrToUtf8(const std::wstring& ws)
{
    if (ws.empty()) return {};
    glong written = 0;
    gchar* utf8 = g_ucs4_to_utf8(
        reinterpret_cast<const gunichar*>(ws.c_str()),
        (glong)ws.size(), nullptr, &written, nullptr);
    std::string result;
    if (utf8) { result.assign(utf8, (size_t)written); g_free(utf8); }
    return result;
}

// Build a single (ia{sv}av) variant from a MenuItem tree.
static GVariant* buildItemVariant(const MenuItem& item, int depth)
{
    GVariantBuilder props;
    g_variant_builder_init(&props, G_VARIANT_TYPE("a{sv}"));
    if (!item.label.empty())
        g_variant_builder_add(&props, "{sv}", "label",
            g_variant_new_string(item.label.c_str()));
    g_variant_builder_add(&props, "{sv}", "type",
        g_variant_new_string(item.type.c_str()));
    g_variant_builder_add(&props, "{sv}", "enabled",
        g_variant_new_boolean(item.enabled));
    g_variant_builder_add(&props, "{sv}", "visible",
        g_variant_new_boolean(item.visible));
    if (!item.toggle_type.empty()) {
        g_variant_builder_add(&props, "{sv}", "toggle-type",
            g_variant_new_string(item.toggle_type.c_str()));
        g_variant_builder_add(&props, "{sv}", "toggle-state",
            g_variant_new_int32(item.toggle_state));
    }
    if (!item.children_display.empty())
        g_variant_builder_add(&props, "{sv}", "children-display",
            g_variant_new_string(item.children_display.c_str()));

    GVariantBuilder children;
    g_variant_builder_init(&children, G_VARIANT_TYPE("av"));
    if (depth != 0) {
        int next = (depth > 0) ? depth - 1 : depth;
        for (const auto& child : item.children)
            g_variant_builder_add(&children, "v", buildItemVariant(child, next));
    }

    return g_variant_new("(ia{sv}av)", item.id, &props, &children);
}

// Build a flat props dict for one item (used by GetGroupProperties).
static GVariant* buildPropsDict(const MenuItem& item)
{
    GVariantBuilder props;
    g_variant_builder_init(&props, G_VARIANT_TYPE("a{sv}"));
    if (!item.label.empty())
        g_variant_builder_add(&props, "{sv}", "label",
            g_variant_new_string(item.label.c_str()));
    g_variant_builder_add(&props, "{sv}", "type",
        g_variant_new_string(item.type.c_str()));
    g_variant_builder_add(&props, "{sv}", "enabled",
        g_variant_new_boolean(item.enabled));
    g_variant_builder_add(&props, "{sv}", "visible",
        g_variant_new_boolean(item.visible));
    if (!item.toggle_type.empty()) {
        g_variant_builder_add(&props, "{sv}", "toggle-type",
            g_variant_new_string(item.toggle_type.c_str()));
        g_variant_builder_add(&props, "{sv}", "toggle-state",
            g_variant_new_int32(item.toggle_state));
    }
    if (!item.children_display.empty())
        g_variant_builder_add(&props, "{sv}", "children-display",
            g_variant_new_string(item.children_display.c_str()));
    return g_variant_builder_end(&props);
}

static const MenuItem* findItem(const MenuItem& root, int id)
{
    if (root.id == id) return &root;
    for (const auto& c : root.children)
        if (const MenuItem* m = findItem(c, id)) return m;
    return nullptr;
}

// ---------------------------------------------------------------------------
// Impl
// ---------------------------------------------------------------------------

struct FxTraySNI::Impl
{
    std::function<void()>    on_activate;
    std::function<void(int)> on_item_activated;

    // SNI icon state
    std::string icon_name       = "fxsound";
    std::string icon_theme_path;
    int         px_w = 0, px_h = 0;
    std::vector<unsigned char> px_argb;

    // dbusmenu state
    std::mutex   menu_mutex;
    MenuItem     root_menu;
    uint32_t     revision = 1;

    // GLib/DBus
    GMainContext*    ctx      = nullptr;
    GMainLoop*       loop     = nullptr;
    std::thread      thread;
    GDBusConnection* conn     = nullptr;
    guint            owner_id = 0;
    guint            sni_reg  = 0;
    guint            menu_reg = 0;
    bool             active   = false;
    std::string      bus_name;

    // ---- SNI method handler ------------------------------------------------
    static void sniMethod(GDBusConnection*, const gchar*, const gchar*,
                          const gchar*, const gchar* method,
                          GVariant*, GDBusMethodInvocation* inv, gpointer user)
    {
        auto* self = static_cast<Impl*>(user);
        if (g_strcmp0(method, "Activate") == 0)
        {
            if (self->on_activate) self->on_activate();
        }
        // SecondaryActivate (middle-click) and ContextMenu: hosts that
        // support dbusmenu handle right-click via the Menu object; those
        // that don't will call ContextMenu — we ignore both here because
        // actions come through the dbusmenu Event method instead.
        g_dbus_method_invocation_return_value(inv, nullptr);
    }

    // ---- SNI property handler ----------------------------------------------
    static GVariant* sniProperty(GDBusConnection*, const gchar*, const gchar*,
                                 const gchar*, const gchar* prop,
                                 GError**, gpointer user)
    {
        auto* self = static_cast<Impl*>(user);
        if (g_strcmp0(prop, "Category")      == 0) return g_variant_new_string("ApplicationStatus");
        if (g_strcmp0(prop, "Id")            == 0) return g_variant_new_string("fxsound");
        if (g_strcmp0(prop, "Title")         == 0) return g_variant_new_string("FxSound");
        if (g_strcmp0(prop, "Status")        == 0) return g_variant_new_string("Active");
        if (g_strcmp0(prop, "IconName")      == 0) return g_variant_new_string(self->icon_name.c_str());
        if (g_strcmp0(prop, "IconThemePath") == 0) return g_variant_new_string(self->icon_theme_path.c_str());
        if (g_strcmp0(prop, "ItemIsMenu")    == 0) return g_variant_new_boolean(FALSE);
        if (g_strcmp0(prop, "Menu")          == 0) return g_variant_new_object_path(kMenuPath);
        if (g_strcmp0(prop, "IconPixmap")    == 0)
        {
            GVariantBuilder outer;
            g_variant_builder_init(&outer, G_VARIANT_TYPE("a(iiay)"));
            if (!self->px_argb.empty())
            {
                GVariantBuilder bytes;
                g_variant_builder_init(&bytes, G_VARIANT_TYPE("ay"));
                for (unsigned char b : self->px_argb)
                    g_variant_builder_add(&bytes, "y", b);
                g_variant_builder_add(&outer, "(iiay)", self->px_w, self->px_h, &bytes);
            }
            return g_variant_builder_end(&outer);
        }
        return nullptr;
    }

    // ---- dbusmenu method handler -------------------------------------------
    static void menuMethod(GDBusConnection*, const gchar*, const gchar*,
                           const gchar*, const gchar* method,
                           GVariant* params, GDBusMethodInvocation* inv, gpointer user)
    {
        auto* self = static_cast<Impl*>(user);

        if (g_strcmp0(method, "GetLayout") == 0)
        {
            gint32 parent_id = 0, depth = -1;
            GVariant* prop_names = nullptr;
            g_variant_get(params, "(ii@as)", &parent_id, &depth, &prop_names);
            if (prop_names) g_variant_unref(prop_names);

            std::lock_guard<std::mutex> lock(self->menu_mutex);

            // Find the subtree rooted at parent_id.
            const MenuItem* root = findItem(self->root_menu, parent_id);
            if (!root) root = &self->root_menu;

            GVariant* layout = buildItemVariant(*root, depth);
            g_dbus_method_invocation_return_value(inv,
                g_variant_new("(u@(ia{sv}av))", self->revision, layout));
        }
        else if (g_strcmp0(method, "GetGroupProperties") == 0)
        {
            GVariant* ids_v     = nullptr;
            GVariant* names_v   = nullptr;
            g_variant_get(params, "(@ai@as)", &ids_v, &names_v);
            if (names_v) g_variant_unref(names_v);

            GVariantBuilder result;
            g_variant_builder_init(&result, G_VARIANT_TYPE("a(ia{sv})"));

            std::lock_guard<std::mutex> lock(self->menu_mutex);
            GVariantIter iter;
            g_variant_iter_init(&iter, ids_v);
            gint32 id;
            while (g_variant_iter_next(&iter, "i", &id))
            {
                if (const MenuItem* m = findItem(self->root_menu, id))
                    g_variant_builder_add(&result, "(i@a{sv})", id, buildPropsDict(*m));
            }
            if (ids_v) g_variant_unref(ids_v);
            g_dbus_method_invocation_return_value(inv,
                g_variant_new("(a(ia{sv}))", &result));
        }
        else if (g_strcmp0(method, "GetProperty") == 0)
        {
            gint32 id = 0;
            const gchar* name = nullptr;
            g_variant_get(params, "(i&s)", &id, &name);

            std::lock_guard<std::mutex> lock(self->menu_mutex);
            const MenuItem* m = findItem(self->root_menu, id);
            GVariant* val = nullptr;
            if (m && g_strcmp0(name, "label") == 0)
                val = g_variant_new_string(m->label.c_str());
            else if (m && g_strcmp0(name, "enabled") == 0)
                val = g_variant_new_boolean(m->enabled);
            else if (m && g_strcmp0(name, "visible") == 0)
                val = g_variant_new_boolean(m->visible);
            else
                val = g_variant_new_string("");
            g_dbus_method_invocation_return_value(inv,
                g_variant_new("(v)", val));
        }
        else if (g_strcmp0(method, "Event") == 0)
        {
            gint32 id = 0;
            const gchar* event_id = nullptr;
            GVariant* data = nullptr;
            guint32 ts = 0;
            g_variant_get(params, "(is@vu)", &id, &event_id, &data, &ts);
            if (data) g_variant_unref(data);
            if (g_strcmp0(event_id, "clicked") == 0 && self->on_item_activated)
                self->on_item_activated(id);
            g_dbus_method_invocation_return_value(inv, nullptr);
        }
        else if (g_strcmp0(method, "EventGroup") == 0)
        {
            // Process all events in the group.
            GVariant* events = nullptr;
            g_variant_get(params, "(@a(isvu))", &events);
            if (events)
            {
                GVariantIter iter;
                g_variant_iter_init(&iter, events);
                gint32 id; const gchar* ev; GVariant* d; guint32 ts;
                while (g_variant_iter_next(&iter, "(is@vu)", &id, &ev, &d, &ts))
                {
                    if (d) g_variant_unref(d);
                    if (g_strcmp0(ev, "clicked") == 0 && self->on_item_activated)
                        self->on_item_activated(id);
                    g_free((gpointer)ev);
                }
                g_variant_unref(events);
            }
            GVariantBuilder errs;
            g_variant_builder_init(&errs, G_VARIANT_TYPE("ai"));
            g_dbus_method_invocation_return_value(inv,
                g_variant_new("(ai)", &errs));
        }
        else if (g_strcmp0(method, "AboutToShow") == 0)
        {
            // The host is about to display a submenu; return FALSE (no forced
            // update). The host will have received LayoutUpdated when we rebuilt.
            g_dbus_method_invocation_return_value(inv, g_variant_new("(b)", FALSE));
        }
        else if (g_strcmp0(method, "AboutToShowGroup") == 0)
        {
            GVariantBuilder upd, err;
            g_variant_builder_init(&upd, G_VARIANT_TYPE("ai"));
            g_variant_builder_init(&err, G_VARIANT_TYPE("ai"));
            g_dbus_method_invocation_return_value(inv,
                g_variant_new("(aiai)", &upd, &err));
        }
        else
        {
            g_dbus_method_invocation_return_error_literal(
                inv, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD, method);
        }
    }

    // ---- dbusmenu property handler -----------------------------------------
    static GVariant* menuProperty(GDBusConnection*, const gchar*, const gchar*,
                                  const gchar*, const gchar* prop,
                                  GError**, gpointer)
    {
        if (g_strcmp0(prop, "Version")       == 0) return g_variant_new_uint32(3);
        if (g_strcmp0(prop, "TextDirection") == 0) return g_variant_new_string("ltr");
        if (g_strcmp0(prop, "Status")        == 0) return g_variant_new_string("normal");
        if (g_strcmp0(prop, "IconThemePath") == 0)
        {
            GVariantBuilder b;
            g_variant_builder_init(&b, G_VARIANT_TYPE("as"));
            return g_variant_builder_end(&b);
        }
        return nullptr;
    }

    // ---- registration -------------------------------------------------------
    static void onBusAcquired(GDBusConnection* connection,
                              const gchar*, gpointer user)
    {
        auto* self = static_cast<Impl*>(user);
        self->conn = connection;

        static const GDBusInterfaceVTable sniVtable = {
            sniMethod, sniProperty, nullptr, { nullptr }
        };
        static const GDBusInterfaceVTable menuVtable = {
            menuMethod, menuProperty, nullptr, { nullptr }
        };

        GDBusNodeInfo* sni_node = g_dbus_node_info_new_for_xml(kSNIXml, nullptr);
        self->sni_reg = g_dbus_connection_register_object(
            connection, "/StatusNotifierItem",
            sni_node->interfaces[0], &sniVtable, self, nullptr, nullptr);
        g_dbus_node_info_unref(sni_node);

        GDBusNodeInfo* menu_node = g_dbus_node_info_new_for_xml(kMenuXml, nullptr);
        self->menu_reg = g_dbus_connection_register_object(
            connection, kMenuPath,
            menu_node->interfaces[0], &menuVtable, self, nullptr, nullptr);
        g_dbus_node_info_unref(menu_node);

        // Register with StatusNotifierWatcher using the connection's unique name.
        const char* unique = g_dbus_connection_get_unique_name(connection);
        GVariant* res = g_dbus_connection_call_sync(
            connection,
            "org.kde.StatusNotifierWatcher", "/StatusNotifierWatcher",
            "org.kde.StatusNotifierWatcher", "RegisterStatusNotifierItem",
            g_variant_new("(s)", unique ? unique : self->bus_name.c_str()),
            nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr);
        if (res) { g_variant_unref(res); self->active = true; }
    }

    void emitSNISignal(const char* name, GVariant* params)
    {
        if (!conn) return;
        g_dbus_connection_emit_signal(conn, nullptr, "/StatusNotifierItem",
            "org.kde.StatusNotifierItem", name, params, nullptr);
    }

    void emitLayoutUpdated()
    {
        if (!conn) return;
        g_dbus_connection_emit_signal(conn, nullptr, kMenuPath,
            "com.canonical.dbusmenu", "LayoutUpdated",
            g_variant_new("(ui)", revision, 0), nullptr);
    }

    void run()
    {
        ctx  = g_main_context_new();
        g_main_context_push_thread_default(ctx);
        loop = g_main_loop_new(ctx, FALSE);

        bus_name = "org.kde.StatusNotifierItem-" + std::to_string(getpid()) + "-1";
        owner_id = g_bus_own_name(G_BUS_TYPE_SESSION, bus_name.c_str(),
                                  G_BUS_NAME_OWNER_FLAGS_NONE,
                                  onBusAcquired, nullptr, nullptr, this, nullptr);
        g_main_loop_run(loop);

        if (owner_id) g_bus_unown_name(owner_id);
        if (loop)     g_main_loop_unref(loop);
        g_main_context_pop_thread_default(ctx);
        if (ctx) g_main_context_unref(ctx);
    }
};

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

FxTraySNI::FxTraySNI(std::function<void()>    on_activate,
                     std::function<void(int)>  on_item_activated)
    : impl_(std::make_unique<Impl>())
{
    impl_->on_activate       = std::move(on_activate);
    impl_->on_item_activated = std::move(on_item_activated);
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

void FxTraySNI::updateMenu(MenuItem root)
{
    if (!impl_) return;
    {
        std::lock_guard<std::mutex> lock(impl_->menu_mutex);
        impl_->root_menu = std::move(root);
        impl_->revision++;
    }
    impl_->emitLayoutUpdated();
}

void FxTraySNI::setIconName(const std::string& name)
{
    if (!impl_) return;
    impl_->icon_name = name;
    impl_->emitSNISignal("NewIcon", nullptr);
}

void FxTraySNI::setIconThemePath(const std::string& path)
{
    if (!impl_) return;
    impl_->icon_theme_path = path;
    impl_->emitSNISignal("NewIcon", nullptr);
}

void FxTraySNI::setIconPixmap(int w, int h, std::vector<unsigned char> rgba)
{
    if (!impl_) return;
    // SNI IconPixmap expects network-byte-order ARGB (big-endian per pixel).
    // Convert from RGBA to ARGB big-endian.
    impl_->px_w = w;
    impl_->px_h = h;
    impl_->px_argb.clear();
    impl_->px_argb.reserve(rgba.size());
    for (size_t i = 0; i + 3 < rgba.size(); i += 4)
    {
        unsigned char r = rgba[i], g = rgba[i+1], b = rgba[i+2], a = rgba[i+3];
        // Pack as big-endian 32-bit ARGB: A R G B bytes
        impl_->px_argb.push_back(a);
        impl_->px_argb.push_back(r);
        impl_->px_argb.push_back(g);
        impl_->px_argb.push_back(b);
    }
    impl_->emitSNISignal("NewIcon", nullptr);
}

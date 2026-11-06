/*
FxSound Linux port — linux/tray_sni.cpp
StatusNotifierItem (SNI) + DBusMenu (com.canonical.dbusmenu) + D-Bus control
interface (org.fxsound.FxSound) over sd-bus.
*/
#include "tray_sni.h"
#include "sndDevices_pw.h"

#if defined(HAVE_SYSTEMD)
#include <systemd/sd-bus.h>
#include <JuceHeader.h>
#include "FxController.h"
#include "FxModel.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

namespace fxsound_linux {
namespace {

static sd_bus* s_bus = nullptr;
static std::thread s_bus_thread;
static std::atomic<bool> s_running{false};
static std::atomic<bool> s_powerOn{false};
static std::atomic<bool> s_processing{false};
static std::mutex s_titleMutex;
static std::string s_activePreset;

// Actions dispatched to the JUCE message thread
void actionToggleWindow() {
  juce::MessageManager::callAsync([] {
    if (FxController::getInstance().isMainWindowVisible())
      FxController::getInstance().hideMainWindow();
    else
      FxController::getInstance().showMainWindow();
  });
}

void actionTogglePower() {
  juce::MessageManager::callAsync([] {
    bool p = FxModel::getModel().getPowerState();
    FxController::getInstance().setPowerState(!p);
  });
}

void actionNextPreset() {
  juce::MessageManager::callAsync([] {
    int cur = FxModel::getModel().getSelectedPreset();
    int count = FxModel::getModel().getPresetCount();
    if (count > 1) {
      int next = (cur + 1) % count;
      FxController::getInstance().setPreset(next);
    }
  });
}

void actionPrevPreset() {
  juce::MessageManager::callAsync([] {
    int cur = FxModel::getModel().getSelectedPreset();
    int count = FxModel::getModel().getPresetCount();
    if (count > 1) {
      int prev = (cur > 0) ? (cur - 1) : (count - 1);
      FxController::getInstance().setPreset(prev);
    }
  });
}

void actionNextOutput() {
  juce::MessageManager::callAsync([] {
    auto sinks = pwListSinks();
    if (sinks.empty()) return;
    auto curOut = FxModel::getModel().getSelectedOutput().pwszID;
    size_t idx = 0;
    for (size_t i = 0; i < sinks.size(); ++i) {
      std::wstring wid = L"pw:" + std::wstring(sinks[i].nodeName.begin(), sinks[i].nodeName.end());
      if (wid == curOut) { idx = i; break; }
    }
    size_t nextIdx = (idx + 1) % sinks.size();
    pwSetOutput(sinks[nextIdx].nodeName);
  });
}

void actionQuit() {
  juce::MessageManager::callAsync([] {
    FxController::getInstance().exit();
  });
}

void showNativeContextMenu(int x, int y) {
  juce::MessageManager::callAsync([x, y] {
    juce::PopupMenu menu;

    // 1. Show / Hide
    bool visible = FxController::getInstance().isMainWindowVisible();
    menu.addItem(visible ? "Hide FxSound" : "Show FxSound", true, false, [] {
      if (FxController::getInstance().isMainWindowVisible())
        FxController::getInstance().hideMainWindow();
      else
        FxController::getInstance().showMainWindow();
    });

    // 2. Power Toggle
    bool power = FxModel::getModel().getPowerState();
    menu.addItem(power ? "Power: On (Turn Off)" : "Power: Off (Turn On)", true, false, [] {
      bool p = FxModel::getModel().getPowerState();
      FxController::getInstance().setPowerState(!p);
    });

    menu.addSeparator();

    // 3. Presets Submenu
    juce::PopupMenu presetMenu;
    int count = FxModel::getModel().getPresetCount();
    int curPreset = FxModel::getModel().getSelectedPreset();
    for (int i = 0; i < count; ++i) {
      auto p = FxModel::getModel().getPreset(i);
      juce::String name = p.modified ? (p.name + " *") : p.name;
      presetMenu.addItem(name, true, i == curPreset, [i] {
        FxController::getInstance().setPreset(i);
      });
    }
    if (count > 0) {
      menu.addSubMenu("Presets", presetMenu);
    }

    // 4. Output Device Submenu
    juce::PopupMenu outputMenu;
    auto sinks = pwListSinks();
    auto curOut = FxModel::getModel().getSelectedOutput().pwszID;
    for (size_t i = 0; i < sinks.size(); ++i) {
      std::wstring wid = L"pw:" + std::wstring(sinks[i].nodeName.begin(), sinks[i].nodeName.end());
      bool isCur = (wid == curOut);
      std::string desc = sinks[i].description.empty() ? sinks[i].nodeName : sinks[i].description;
      outputMenu.addItem(juce::String::fromUTF8(desc.c_str()), true, isCur, [nodeName = sinks[i].nodeName] {
        pwSetOutput(nodeName);
      });
    }
    if (!sinks.empty()) {
      menu.addSubMenu("Output Device", outputMenu);
    }

    menu.addSeparator();

    // 5. Exit
    menu.addItem("Exit", true, false, [] {
      FxController::getInstance().exit();
    });

    juce::PopupMenu::Options opts;
    if (x > 0 || y > 0) {
      opts = opts.withTargetScreenArea(juce::Rectangle<int>(x, y, 1, 1));
    } else {
      opts = opts.withMousePosition();
    }
    menu.showMenuAsync(opts);
  });
}

// ----------------- org.kde.StatusNotifierItem -----------------

static std::vector<uint8_t> s_pixmapOn;
static std::vector<uint8_t> s_pixmapOff;

static void initPixmaps() {
  if (!s_pixmapOn.empty()) return;

  juce::Image img = juce::ImageFileFormat::loadFrom(BinaryData::fxsound_png, (size_t)BinaryData::fxsound_pngSize);
  if (!img.isValid()) return;

  int w = img.getWidth();
  int h = img.getHeight();
  s_pixmapOn.resize((size_t)(w * h * 4));
  s_pixmapOff.resize((size_t)(w * h * 4));

  juce::Image::BitmapData bm(img, juce::Image::BitmapData::readOnly);
  size_t idx = 0;
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      juce::Colour c = bm.getPixelColour(x, y);
      uint8_t a = c.getAlpha();
      uint8_t r = c.getRed();
      uint8_t g = c.getGreen();
      uint8_t b = c.getBlue();

      // Active state: vibrant red flame (ARGB32 in network byte order)
      s_pixmapOn[idx]     = a;
      s_pixmapOn[idx + 1] = r;
      s_pixmapOn[idx + 2] = g;
      s_pixmapOn[idx + 3] = b;

      // Passive/Off state: dimmed greyscale flame
      uint8_t gray = (uint8_t)(0.299f * r + 0.587f * g + 0.114f * b);
      s_pixmapOff[idx]     = (uint8_t)(a * 0.55f);
      s_pixmapOff[idx + 1] = gray;
      s_pixmapOff[idx + 2] = gray;
      s_pixmapOff[idx + 3] = gray;

      idx += 4;
    }
  }
}

static int sni_get_category(sd_bus*, const char*, const char*, const char*, sd_bus_message* reply, void*, sd_bus_error*) {
  return sd_bus_message_append(reply, "s", "ApplicationStatus");
}

static int sni_get_id(sd_bus*, const char*, const char*, const char*, sd_bus_message* reply, void*, sd_bus_error*) {
  return sd_bus_message_append(reply, "s", "FxSound");
}

static int sni_get_title(sd_bus*, const char*, const char*, const char*, sd_bus_message* reply, void*, sd_bus_error*) {
  std::string title;
  if (!s_powerOn.load()) {
    title = "FxSound (Off)";
  } else {
    std::lock_guard<std::mutex> lock(s_titleMutex);
    if (!s_activePreset.empty()) {
      title = "FxSound - " + s_activePreset;
    } else {
      title = "FxSound";
    }
  }
  return sd_bus_message_append(reply, "s", title.c_str());
}

static int sni_get_tooltip(sd_bus*, const char*, const char*, const char*, sd_bus_message* reply, void*, sd_bus_error*) {
  std::string desc;
  if (!s_powerOn.load()) {
    desc = "Off";
  } else {
    std::lock_guard<std::mutex> lock(s_titleMutex);
    desc = s_activePreset.empty() ? "Active" : s_activePreset;
  }
  const char* iconName = s_powerOn.load() ? "fxsound" : "fxsound-muted";

  int r = sd_bus_message_open_container(reply, 'r', "sa(iiay)ss");
  if (r < 0) return r;
  sd_bus_message_append(reply, "s", iconName);
  sd_bus_message_open_container(reply, 'a', "(iiay)");
  sd_bus_message_close_container(reply);
  sd_bus_message_append(reply, "ss", "FxSound", desc.c_str());
  return sd_bus_message_close_container(reply);
}

static int sni_get_status(sd_bus*, const char*, const char*, const char*, sd_bus_message* reply, void*, sd_bus_error*) {
  return sd_bus_message_append(reply, "s", s_powerOn.load() ? "Active" : "Passive");
}

static int sni_get_icon_name(sd_bus*, const char*, const char*, const char*, sd_bus_message* reply, void*, sd_bus_error*) {
  return sd_bus_message_append(reply, "s", s_powerOn.load() ? "fxsound" : "fxsound-muted");
}

static int sni_get_icon_pixmap(sd_bus*, const char*, const char*, const char*, sd_bus_message* reply, void*, sd_bus_error*) {
  initPixmaps();

  int r = sd_bus_message_open_container(reply, 'a', "(iiay)");
  if (r < 0) return r;

  const auto& px = s_powerOn.load() ? s_pixmapOn : s_pixmapOff;
  if (!px.empty()) {
    r = sd_bus_message_open_container(reply, 'r', "iiay");
    if (r >= 0) {
      sd_bus_message_append(reply, "ii", 32, 32);
      sd_bus_message_append_array(reply, 'y', px.data(), px.size());
      sd_bus_message_close_container(reply);
    }
  }

  return sd_bus_message_close_container(reply);
}

static int sni_get_icon_theme_path(sd_bus*, const char*, const char*, const char*, sd_bus_message* reply, void*, sd_bus_error*) {
  static std::string themePath = []() -> std::string {
    const char* home = std::getenv("HOME");
    if (home) {
      std::string userPath = std::string(home) + "/.local/share/icons";
      if (std::filesystem::exists(userPath + "/hicolor/48x48/apps/fxsound.png") ||
          std::filesystem::exists(userPath + "/hicolor/scalable/apps/fxsound.svg")) {
        return userPath;
      }
    }
    if (std::filesystem::exists("/usr/share/icons/hicolor/48x48/apps/fxsound.png") ||
        std::filesystem::exists("/usr/share/icons/hicolor/scalable/apps/fxsound.svg")) {
      return "/usr/share/icons";
    }
    return "";
  }();

  return sd_bus_message_append(reply, "s", themePath.c_str());
}

static int sni_get_menu(sd_bus*, const char*, const char*, const char*, sd_bus_message* reply, void*, sd_bus_error*) {
  return sd_bus_message_append(reply, "o", "/MenuBar");
}

static int sni_get_item_is_menu(sd_bus*, const char*, const char*, const char*, sd_bus_message* reply, void*, sd_bus_error*) {
  return sd_bus_message_append(reply, "b", 0);
}

static int sni_method_activate(sd_bus_message* m, void*, sd_bus_error*) {
  actionToggleWindow();
  return sd_bus_reply_method_return(m, nullptr);
}

static int sni_method_context_menu(sd_bus_message* m, void*, sd_bus_error*) {
  int x = 0, y = 0;
  sd_bus_message_read(m, "ii", &x, &y);
  showNativeContextMenu(x, y);
  return sd_bus_reply_method_return(m, nullptr);
}

static int sni_method_secondary_activate(sd_bus_message* m, void*, sd_bus_error*) {
  actionTogglePower();
  return sd_bus_reply_method_return(m, nullptr);
}

static int sni_method_scroll(sd_bus_message* m, void*, sd_bus_error*) {
  int delta = 0;
  const char* orientation = nullptr;
  if (sd_bus_message_read(m, "is", &delta, &orientation) >= 0) {
    if (delta > 0) actionNextPreset();
    else if (delta < 0) actionPrevPreset();
  }
  return sd_bus_reply_method_return(m, nullptr);
}

static const sd_bus_vtable sni_vtable[] = {
  SD_BUS_VTABLE_START(0),
  SD_BUS_PROPERTY("Category", "s", sni_get_category, 0, SD_BUS_VTABLE_PROPERTY_CONST),
  SD_BUS_PROPERTY("Id", "s", sni_get_id, 0, SD_BUS_VTABLE_PROPERTY_CONST),
  SD_BUS_PROPERTY("Title", "s", sni_get_title, 0, SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
  SD_BUS_PROPERTY("ToolTip", "(sa(iiay)ss)", sni_get_tooltip, 0, SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
  SD_BUS_PROPERTY("Status", "s", sni_get_status, 0, SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
  SD_BUS_PROPERTY("IconName", "s", sni_get_icon_name, 0, SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
  SD_BUS_PROPERTY("IconPixmap", "a(iiay)", sni_get_icon_pixmap, 0, SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
  SD_BUS_PROPERTY("IconThemePath", "s", sni_get_icon_theme_path, 0, SD_BUS_VTABLE_PROPERTY_CONST),
  SD_BUS_PROPERTY("Menu", "o", sni_get_menu, 0, SD_BUS_VTABLE_PROPERTY_CONST),
  SD_BUS_PROPERTY("ItemIsMenu", "b", sni_get_item_is_menu, 0, SD_BUS_VTABLE_PROPERTY_CONST),
  SD_BUS_METHOD("Activate", "ii", nullptr, sni_method_activate, SD_BUS_VTABLE_UNPRIVILEGED),
  SD_BUS_METHOD("ContextMenu", "ii", nullptr, sni_method_context_menu, SD_BUS_VTABLE_UNPRIVILEGED),
  SD_BUS_METHOD("SecondaryActivate", "ii", nullptr, sni_method_secondary_activate, SD_BUS_VTABLE_UNPRIVILEGED),
  SD_BUS_METHOD("Scroll", "is", nullptr, sni_method_scroll, SD_BUS_VTABLE_UNPRIVILEGED),
  SD_BUS_SIGNAL("NewTitle", nullptr, 0),
  SD_BUS_SIGNAL("NewToolTip", nullptr, 0),
  SD_BUS_SIGNAL("NewIcon", nullptr, 0),
  SD_BUS_SIGNAL("NewStatus", "s", 0),
  SD_BUS_VTABLE_END
};

// ----------------- com.canonical.dbusmenu -----------------

static int menu_get_version(sd_bus*, const char*, const char*, const char*, sd_bus_message* reply, void*, sd_bus_error*) {
  return sd_bus_message_append(reply, "u", 3);
}

static int menu_get_status(sd_bus*, const char*, const char*, const char*, sd_bus_message* reply, void*, sd_bus_error*) {
  return sd_bus_message_append(reply, "s", "normal");
}

static void appendMenuItem(sd_bus_message* reply, int id, const char* label, bool isSeparator = false) {
  sd_bus_message_open_container(reply, 'v', "(ia{sv}av)");
  sd_bus_message_open_container(reply, 'r', "ia{sv}av");
  sd_bus_message_append(reply, "i", id);
  sd_bus_message_open_container(reply, 'a', "{sv}");
  if (isSeparator) {
    sd_bus_message_open_container(reply, 'e', "sv");
    sd_bus_message_append(reply, "s", "type");
    sd_bus_message_open_container(reply, 'v', "s");
    sd_bus_message_append(reply, "s", "separator");
    sd_bus_message_close_container(reply);
    sd_bus_message_close_container(reply);
  } else {
    sd_bus_message_open_container(reply, 'e', "sv");
    sd_bus_message_append(reply, "s", "label");
    sd_bus_message_open_container(reply, 'v', "s");
    sd_bus_message_append(reply, "s", label);
    sd_bus_message_close_container(reply);
    sd_bus_message_close_container(reply);

    sd_bus_message_open_container(reply, 'e', "sv");
    sd_bus_message_append(reply, "s", "enabled");
    sd_bus_message_open_container(reply, 'v', "b");
    sd_bus_message_append(reply, "b", 1);
    sd_bus_message_close_container(reply);
    sd_bus_message_close_container(reply);
  }
  sd_bus_message_close_container(reply);  // a{sv}
  sd_bus_message_open_container(reply, 'a', "v");
  sd_bus_message_close_container(reply);  // av
  sd_bus_message_close_container(reply);  // r
  sd_bus_message_close_container(reply);  // v
}

static int menu_method_get_layout(sd_bus_message* m, void*, sd_bus_error*) {
  sd_bus_message* reply = nullptr;
  int r = sd_bus_message_new_method_return(m, &reply);
  if (r < 0) return r;

  sd_bus_message_append(reply, "u", 1);  // revision

  // root item (0)
  sd_bus_message_open_container(reply, 'r', "ia{sv}av");
  sd_bus_message_append(reply, "i", 0);
  sd_bus_message_open_container(reply, 'a', "{sv}");
  sd_bus_message_close_container(reply);  // empty properties on root
  sd_bus_message_open_container(reply, 'a', "v");

  std::string pLabel = s_powerOn.load() ? "Power: On (Turn Off)" : "Power: Off (Turn On)";
  appendMenuItem(reply, 1, "Show / Hide FxSound");
  appendMenuItem(reply, 2, pLabel.c_str());
  appendMenuItem(reply, 3, "Next Preset");
  appendMenuItem(reply, 4, "Previous Preset");
  appendMenuItem(reply, 5, "Next Output Device");
  appendMenuItem(reply, 6, "", true);
  appendMenuItem(reply, 7, "Quit");

  sd_bus_message_close_container(reply);  // children av
  sd_bus_message_close_container(reply);  // root r

  r = sd_bus_send(sd_bus_message_get_bus(m), reply, nullptr);
  sd_bus_message_unref(reply);
  return r;
}

static int menu_method_get_group_properties(sd_bus_message* m, void*, sd_bus_error*) {
  sd_bus_message* reply = nullptr;
  int r = sd_bus_message_new_method_return(m, &reply);
  if (r < 0) return r;

  sd_bus_message_open_container(reply, 'a', "(ia{sv})");
  sd_bus_message_close_container(reply);

  r = sd_bus_send(sd_bus_message_get_bus(m), reply, nullptr);
  sd_bus_message_unref(reply);
  return r;
}

static int menu_method_get_property(sd_bus_message* m, void*, sd_bus_error*) {
  int id = 0;
  const char* name = nullptr;
  sd_bus_message_read(m, "is", &id, &name);
  sd_bus_message* reply = nullptr;
  int r = sd_bus_message_new_method_return(m, &reply);
  if (r < 0) return r;

  std::string val;
  if (name && std::strcmp(name, "label") == 0) {
    if (id == 1) val = "Show / Hide FxSound";
    else if (id == 2) val = s_powerOn.load() ? "Power: On (Turn Off)" : "Power: Off (Turn On)";
    else if (id == 3) val = "Next Preset";
    else if (id == 4) val = "Previous Preset";
    else if (id == 5) val = "Next Output Device";
    else if (id == 7) val = "Quit";
  } else if (name && std::strcmp(name, "type") == 0 && id == 6) {
    val = "separator";
  }

  if (name && std::strcmp(name, "enabled") == 0) {
    sd_bus_message_open_container(reply, 'v', "b");
    sd_bus_message_append(reply, "b", 1);
    sd_bus_message_close_container(reply);
  } else {
    sd_bus_message_open_container(reply, 'v', "s");
    sd_bus_message_append(reply, "s", val.c_str());
    sd_bus_message_close_container(reply);
  }

  r = sd_bus_send(sd_bus_message_get_bus(m), reply, nullptr);
  sd_bus_message_unref(reply);
  return r;
}

static int menu_method_event(sd_bus_message* m, void*, sd_bus_error*) {
  int id = 0;
  const char* eventId = nullptr;
  if (sd_bus_message_read(m, "is", &id, &eventId) >= 0) {
    if (std::strcmp(eventId, "clicked") == 0) {
      switch (id) {
        case 1: actionToggleWindow(); break;
        case 2: actionTogglePower(); break;
        case 3: actionNextPreset(); break;
        case 4: actionPrevPreset(); break;
        case 5: actionNextOutput(); break;
        case 7: actionQuit(); break;
        default: break;
      }
    }
  }
  return sd_bus_reply_method_return(m, nullptr);
}

static int menu_method_about_to_show(sd_bus_message* m, void*, sd_bus_error*) {
  return sd_bus_reply_method_return(m, "b", 0);
}

static const sd_bus_vtable dbusmenu_vtable[] = {
  SD_BUS_VTABLE_START(0),
  SD_BUS_PROPERTY("Version", "u", menu_get_version, 0, SD_BUS_VTABLE_PROPERTY_CONST),
  SD_BUS_PROPERTY("Status", "s", menu_get_status, 0, SD_BUS_VTABLE_PROPERTY_CONST),
  SD_BUS_METHOD("GetLayout", "iias", "u(ia{sv}av)", menu_method_get_layout, SD_BUS_VTABLE_UNPRIVILEGED),
  SD_BUS_METHOD("GetGroupProperties", "aias", "a(ia{sv})", menu_method_get_group_properties, SD_BUS_VTABLE_UNPRIVILEGED),
  SD_BUS_METHOD("GetProperty", "is", "v", menu_method_get_property, SD_BUS_VTABLE_UNPRIVILEGED),
  SD_BUS_METHOD("Event", "isvu", nullptr, menu_method_event, SD_BUS_VTABLE_UNPRIVILEGED),
  SD_BUS_METHOD("AboutToShow", "i", "b", menu_method_about_to_show, SD_BUS_VTABLE_UNPRIVILEGED),
  SD_BUS_SIGNAL("LayoutUpdated", "ui", 0),
  SD_BUS_VTABLE_END
};

// ----------------- org.fxsound.FxSound (Direct Control) -----------------

static int fx_method_toggle_power(sd_bus_message* m, void*, sd_bus_error*) {
  actionTogglePower();
  return sd_bus_reply_method_return(m, "b", !s_powerOn.load());
}

static int fx_method_toggle_window(sd_bus_message* m, void*, sd_bus_error*) {
  actionToggleWindow();
  return sd_bus_reply_method_return(m, "b", 1);
}

static int fx_method_next_preset(sd_bus_message* m, void*, sd_bus_error*) {
  actionNextPreset();
  return sd_bus_reply_method_return(m, "s", "ok");
}

static int fx_method_prev_preset(sd_bus_message* m, void*, sd_bus_error*) {
  actionPrevPreset();
  return sd_bus_reply_method_return(m, "s", "ok");
}

static int fx_method_next_output(sd_bus_message* m, void*, sd_bus_error*) {
  actionNextOutput();
  return sd_bus_reply_method_return(m, "s", "ok");
}

static int fx_method_quit(sd_bus_message* m, void*, sd_bus_error*) {
  actionQuit();
  return sd_bus_reply_method_return(m, nullptr);
}

static const sd_bus_vtable fxsound_vtable[] = {
  SD_BUS_VTABLE_START(0),
  SD_BUS_METHOD("TogglePower", nullptr, "b", fx_method_toggle_power, SD_BUS_VTABLE_UNPRIVILEGED),
  SD_BUS_METHOD("ToggleWindow", nullptr, "b", fx_method_toggle_window, SD_BUS_VTABLE_UNPRIVILEGED),
  SD_BUS_METHOD("NextPreset", nullptr, "s", fx_method_next_preset, SD_BUS_VTABLE_UNPRIVILEGED),
  SD_BUS_METHOD("PreviousPreset", nullptr, "s", fx_method_prev_preset, SD_BUS_VTABLE_UNPRIVILEGED),
  SD_BUS_METHOD("NextOutput", nullptr, "s", fx_method_next_output, SD_BUS_VTABLE_UNPRIVILEGED),
  SD_BUS_METHOD("Quit", nullptr, nullptr, fx_method_quit, SD_BUS_VTABLE_UNPRIVILEGED),
  SD_BUS_VTABLE_END
};

// ----------------- Global Shortcuts Portal Listener -----------------

static int onPortalShortcutActivated(sd_bus_message* m, void*, sd_bus_error*) {
  const char* session = nullptr;
  const char* shortcut_id = nullptr;
  uint64_t timestamp = 0;
  if (sd_bus_message_read(m, "ost", &session, &shortcut_id, &timestamp) >= 0) {
    if (std::strcmp(shortcut_id, "power") == 0) actionTogglePower();
    else if (std::strcmp(shortcut_id, "window") == 0) actionToggleWindow();
    else if (std::strcmp(shortcut_id, "next_preset") == 0) actionNextPreset();
    else if (std::strcmp(shortcut_id, "prev_preset") == 0) actionPrevPreset();
    else if (std::strcmp(shortcut_id, "next_output") == 0) actionNextOutput();
  }
  return 0;
}

}  // namespace

bool traySniStart() {
  if (s_running.load()) return true;

  initPixmaps();

  int r = sd_bus_open_user(&s_bus);
  if (r < 0 || !s_bus) return false;

  // Request unique and well-known names
  char sniName[64];
  std::snprintf(sniName, sizeof(sniName), "org.kde.StatusNotifierItem-%d-1", static_cast<int>(getpid()));
  sd_bus_request_name(s_bus, sniName, 0);
  sd_bus_request_name(s_bus, "org.fxsound.FxSound", 0);

  // Register objects
  sd_bus_add_object_vtable(s_bus, nullptr, "/StatusNotifierItem", "org.kde.StatusNotifierItem", sni_vtable, nullptr);
  sd_bus_add_object_vtable(s_bus, nullptr, "/MenuBar", "com.canonical.dbusmenu", dbusmenu_vtable, nullptr);
  sd_bus_add_object_vtable(s_bus, nullptr, "/org/fxsound/FxSound", "org.fxsound.FxSound", fxsound_vtable, nullptr);

  // Listen for GlobalShortcuts Portal signals
  sd_bus_match_signal(s_bus, nullptr, "org.freedesktop.portal.Desktop", nullptr,
                      "org.freedesktop.portal.GlobalShortcuts", "Activated",
                      onPortalShortcutActivated, nullptr);

  // Register with StatusNotifierWatcher
  sd_bus_call_method_async(s_bus, nullptr,
                           "org.kde.StatusNotifierWatcher",
                           "/StatusNotifierWatcher",
                           "org.kde.StatusNotifierWatcher",
                           "RegisterStatusNotifierItem",
                           nullptr, nullptr,
                           "s", "/StatusNotifierItem");

  s_running.store(true);
  s_bus_thread = std::thread([] {
    while (s_running.load()) {
      int ret = sd_bus_process(s_bus, nullptr);
      if (ret < 0) break;
      if (ret > 0) continue;
      sd_bus_wait(s_bus, 250000);  // 250ms
    }
  });

  return true;
}

void traySniStop() {
  if (!s_running.exchange(false)) return;
  if (s_bus) sd_bus_close(s_bus);
  if (s_bus_thread.joinable()) s_bus_thread.join();
  if (s_bus) {
    sd_bus_unref(s_bus);
    s_bus = nullptr;
  }
}

void traySniSetStatus(bool powerOn, bool processing, const std::string& presetName) {
  s_powerOn.store(powerOn);
  s_processing.store(processing);
  {
    std::lock_guard<std::mutex> lock(s_titleMutex);
    if (!presetName.empty()) {
      s_activePreset = presetName;
    }
  }
  if (s_bus && s_running.load()) {
    sd_bus_emit_signal(s_bus, "/StatusNotifierItem", "org.kde.StatusNotifierItem", "NewTitle", nullptr);
    sd_bus_emit_signal(s_bus, "/StatusNotifierItem", "org.kde.StatusNotifierItem", "NewToolTip", nullptr);
    sd_bus_emit_signal(s_bus, "/StatusNotifierItem", "org.kde.StatusNotifierItem", "NewStatus", "s", powerOn ? "Active" : "Passive");
    sd_bus_emit_signal(s_bus, "/StatusNotifierItem", "org.kde.StatusNotifierItem", "NewIcon", nullptr);
    sd_bus_emit_signal(s_bus, "/MenuBar", "com.canonical.dbusmenu", "LayoutUpdated", "ui", 1, 0);
  }
}

}  // namespace fxsound_linux
#else
namespace fxsound_linux {
bool traySniStart() { return false; }
void traySniStop() {}
void traySniSetStatus(bool, bool, const std::string&) {}
}  // namespace fxsound_linux
#endif

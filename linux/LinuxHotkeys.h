#pragma once
#ifndef _WIN32

#include <functional>
#include <map>
#include <thread>
#include <atomic>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

class LinuxHotkeys
{
public:
    using Callback = std::function<void(int cmd_id)>;

    explicit LinuxHotkeys(Callback cb);
    ~LinuxHotkeys();

    bool isAvailable() const { return conn_ != nullptr; }

    // win_mod: MOD_CONTROL|MOD_ALT|MOD_SHIFT from win_compat.h
    // win_vk:  Windows VK code (0x30-0x5A alphanumeric, 0x70-0x7B F1-F12)
    bool registerKey(int cmd_id, int win_mod, int win_vk);
    void unregisterKey(int cmd_id);
    void unregisterAll();

private:
    static uint16_t winModToXcb(int win_mod);
    static uint32_t winVkToKeysym(int win_vk);

    void loop();
    void grabKey(uint16_t xcb_mod, xcb_keycode_t kc);
    void ungrabKey(uint16_t xcb_mod, xcb_keycode_t kc);

    struct Binding { int win_mod; int win_vk; xcb_keycode_t kc; uint16_t xmod; };

    Callback         callback_;
    xcb_connection_t* conn_  = nullptr;
    xcb_key_symbols_t* syms_ = nullptr;
    xcb_window_t     root_   = 0;
    std::map<int, Binding> bindings_;
    std::thread      thread_;
    std::atomic<bool> running_{false};
    int pipe_[2] = {-1, -1};
};

#endif // !_WIN32

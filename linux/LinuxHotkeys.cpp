#ifndef _WIN32

#include "LinuxHotkeys.h"
#include <xcb/xcb_keysyms.h>
#include <X11/keysym.h>
#include <unistd.h>
#include <sys/select.h>
#include <cstdlib>
#include <algorithm>

// Mirrors win_compat.h definitions.
#ifndef MOD_ALT
#define MOD_ALT     0x0001
#define MOD_CONTROL 0x0002
#define MOD_SHIFT   0x0004
#endif

LinuxHotkeys::LinuxHotkeys(Callback cb) : callback_(std::move(cb))
{
    if (!getenv("DISPLAY")) return;

    int screen_num = 0;
    conn_ = xcb_connect(nullptr, &screen_num);
    if (xcb_connection_has_error(conn_)) {
        xcb_disconnect(conn_);
        conn_ = nullptr;
        return;
    }

    const xcb_setup_t* setup = xcb_get_setup(conn_);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    for (int i = 0; i < screen_num; i++) xcb_screen_next(&iter);
    root_ = iter.data->root;

    syms_ = xcb_key_symbols_alloc(conn_);

    if (pipe(pipe_) != 0) {
        xcb_key_symbols_free(syms_);
        syms_ = nullptr;
        xcb_disconnect(conn_);
        conn_ = nullptr;
        return;
    }

    running_ = true;
    thread_ = std::thread(&LinuxHotkeys::loop, this);
}

LinuxHotkeys::~LinuxHotkeys()
{
    unregisterAll();
    running_ = false;
    if (pipe_[1] >= 0) {
        char b = 0;
        (void)write(pipe_[1], &b, 1);
    }
    if (thread_.joinable()) thread_.join();
    if (pipe_[0] >= 0) { close(pipe_[0]); close(pipe_[1]); }
    if (syms_)  xcb_key_symbols_free(syms_);
    if (conn_)  xcb_disconnect(conn_);
}

uint16_t LinuxHotkeys::winModToXcb(int win_mod)
{
    uint16_t m = 0;
    if (win_mod & MOD_CONTROL) m |= XCB_MOD_MASK_CONTROL;
    if (win_mod & MOD_ALT)     m |= XCB_MOD_MASK_1;
    if (win_mod & MOD_SHIFT)   m |= XCB_MOD_MASK_SHIFT;
    return m;
}

uint32_t LinuxHotkeys::winVkToKeysym(int vk)
{
    if (vk >= 0x30 && vk <= 0x39) return (uint32_t)vk;           // '0'-'9'
    if (vk >= 0x41 && vk <= 0x5A) return (uint32_t)(vk + 0x20);  // 'A'-'Z' → lowercase
    if (vk >= 0x70 && vk <= 0x7B) return XK_F1 + (vk - 0x70);    // VK_F1-F12
    return 0;
}

bool LinuxHotkeys::registerKey(int cmd_id, int win_mod, int win_vk)
{
    if (!conn_) return false;

    uint32_t keysym = winVkToKeysym(win_vk);
    if (!keysym) return false;

    xcb_keycode_t* codes = xcb_key_symbols_get_keycode(syms_, keysym);
    if (!codes) return false;
    xcb_keycode_t kc = codes[0];
    free(codes);
    if (kc == XCB_NO_SYMBOL) return false;

    uint16_t xmod = winModToXcb(win_mod);

    auto it = bindings_.find(cmd_id);
    if (it != bindings_.end()) {
        ungrabKey(it->second.xmod, it->second.kc);
        bindings_.erase(it);
    }

    grabKey(xmod, kc);
    bindings_[cmd_id] = {win_mod, win_vk, kc, xmod};
    return true;
}

void LinuxHotkeys::unregisterKey(int cmd_id)
{
    auto it = bindings_.find(cmd_id);
    if (it == bindings_.end()) return;
    ungrabKey(it->second.xmod, it->second.kc);
    bindings_.erase(it);
}

void LinuxHotkeys::unregisterAll()
{
    if (!conn_) return;
    for (auto& [cmd, b] : bindings_) ungrabKey(b.xmod, b.kc);
    bindings_.clear();
}

void LinuxHotkeys::grabKey(uint16_t xmod, xcb_keycode_t kc)
{
    // Grab with NumLock (Mod2) and CapsLock combinations so the hotkey fires
    // regardless of their state.
    for (uint16_t extra : {uint16_t(0),
                           uint16_t(XCB_MOD_MASK_2),
                           uint16_t(XCB_MOD_MASK_LOCK),
                           uint16_t(XCB_MOD_MASK_2 | XCB_MOD_MASK_LOCK)}) {
        xcb_grab_key(conn_, 1, root_, uint16_t(xmod | extra), kc,
                     XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    }
    xcb_flush(conn_);
}

void LinuxHotkeys::ungrabKey(uint16_t xmod, xcb_keycode_t kc)
{
    for (uint16_t extra : {uint16_t(0),
                           uint16_t(XCB_MOD_MASK_2),
                           uint16_t(XCB_MOD_MASK_LOCK),
                           uint16_t(XCB_MOD_MASK_2 | XCB_MOD_MASK_LOCK)}) {
        xcb_ungrab_key(conn_, kc, root_, uint16_t(xmod | extra));
    }
    xcb_flush(conn_);
}

void LinuxHotkeys::loop()
{
    int xfd = xcb_get_file_descriptor(conn_);
    while (running_) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(xfd, &fds);
        FD_SET(pipe_[0], &fds);
        int maxfd = std::max(xfd, pipe_[0]) + 1;

        struct timeval tv{0, 50000};
        int ret = select(maxfd, &fds, nullptr, nullptr, &tv);
        if (ret < 0) break;
        if (FD_ISSET(pipe_[0], &fds)) break;

        if (FD_ISSET(xfd, &fds)) {
            xcb_generic_event_t* ev;
            while ((ev = xcb_poll_for_event(conn_)) != nullptr) {
                if ((ev->response_type & ~0x80) == XCB_KEY_PRESS) {
                    auto* kp = reinterpret_cast<xcb_key_press_event_t*>(ev);
                    uint16_t clean = kp->state & ~uint16_t(XCB_MOD_MASK_2 | XCB_MOD_MASK_LOCK);
                    for (auto& [cmd, b] : bindings_) {
                        if (kp->detail == b.kc && clean == b.xmod) {
                            int c = cmd;
                            // Post to JUCE message thread via the callback.
                            // The caller (FxController) wraps this in callAsync.
                            callback_(c);
                            break;
                        }
                    }
                }
                free(ev);
            }
        }
    }
}

#endif // !_WIN32

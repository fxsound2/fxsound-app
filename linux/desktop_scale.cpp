#if ! defined(_WIN32) && ! defined(_WIN64)

#include "desktop_scale.h"
#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>
#include <algorithm>
#include <dlfcn.h>

struct _XDisplay;
typedef struct _XDisplay Display;

static float clampAndSnap(float s)
{
    if (s >= 0.95f && s <= 1.08f)
        return 1.0f;
    return std::max(0.75f, std::min(3.0f, s));
}

float detectLinuxDesktopScale()
{
    // 1. Explicit environment overrides
    if (const char* env = getenv("GDK_SCALE"))
    {
        float s = std::strtof(env, nullptr);
        if (s >= 0.5f && s <= 4.0f)
            return clampAndSnap(s);
    }
    if (const char* env = getenv("QT_SCALE_FACTOR"))
    {
        float s = std::strtof(env, nullptr);
        if (s >= 0.5f && s <= 4.0f)
            return clampAndSnap(s);
    }

    // 2. Query X11 / XWayland Xft.dpi dynamically via dlopen
    const char* dpyName = getenv("DISPLAY");
    if (dpyName && dpyName[0] != '\0')
    {
        void* x11 = dlopen("libX11.so.6", RTLD_LAZY);
        if (!x11) x11 = dlopen("libX11.so", RTLD_LAZY);
        if (x11)
        {
            typedef Display* (*FnOpenDisplay)(const char*);
            typedef int (*FnCloseDisplay)(Display*);
            typedef char* (*FnGetDefault)(Display*, const char*, const char*);

            auto fnOpen = (FnOpenDisplay) dlsym(x11, "XOpenDisplay");
            auto fnClose = (FnCloseDisplay) dlsym(x11, "XCloseDisplay");
            auto fnGet = (FnGetDefault) dlsym(x11, "XGetDefault");

            if (fnOpen && fnClose && fnGet)
            {
                if (Display* dpy = fnOpen(dpyName))
                {
                    char* dpiStr = fnGet(dpy, "Xft", "dpi");
                    if (dpiStr)
                    {
                        float dpi = std::strtof(dpiStr, nullptr);
                        if (dpi >= 48.0f && dpi <= 480.0f)
                        {
                            fnClose(dpy);
                            dlclose(x11);
                            return clampAndSnap(dpi / 96.0f);
                        }
                    }
                    fnClose(dpy);
                }
            }
            dlclose(x11);
        }
    }

    // 3. KDE Plasma Wayland configuration (~/.config/kwinoutputconfig.json)
    const char* home = getenv("HOME");
    if (home)
    {
        std::string path = std::string(home) + "/.config/kwinoutputconfig.json";
        std::ifstream f(path);
        if (f.is_open())
        {
            std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
            auto pos = content.find("\"scale\"");
            if (pos != std::string::npos)
            {
                auto colon = content.find(':', pos);
                if (colon != std::string::npos)
                {
                    float s = std::strtof(content.c_str() + colon + 1, nullptr);
                    if (s >= 0.5f && s <= 4.0f)
                        return clampAndSnap(s);
                }
            }
        }
    }

    // 4. GNOME gsettings scaling-factor
    FILE* pipe = popen("/usr/bin/gsettings get org.gnome.desktop.interface scaling-factor 2>/dev/null", "r");
    if (pipe)
    {
        char buffer[128];
        if (fgets(buffer, sizeof(buffer), pipe))
        {
            // Format is "uint32 <val>\n"
            char* p = strstr(buffer, "uint32");
            if (p)
            {
                float s = std::strtof(p + 6, nullptr);
                if (s >= 0.5f && s <= 4.0f)
                {
                    pclose(pipe);
                    return clampAndSnap(s);
                }
            }
        }
        pclose(pipe);
    }

    return 1.0f;
}

#endif

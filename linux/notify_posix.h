/*
FxSound Linux port — linux/notify_posix.h (Phase 9)
Keep FxNotification + hide_notifications semantics behind interface.
If custom popup looks wrong under XWayland, use org.freedesktop.Notifications.
Untested.
*/
#pragma once
#include <string>

namespace fxsound_linux {
void notify(const std::string& title, const std::string& body);
}  // namespace fxsound_linux

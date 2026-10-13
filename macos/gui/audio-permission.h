#pragma once
#include <functional>

namespace fxgui {
enum class AudioPermissionStatus { denied, restricted, notDetermined, granted };
AudioPermissionStatus audioPermissionStatus();
void requestAudioPermission(std::function<void(bool)> completion);
}

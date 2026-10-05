#include "audio-permission.h"
#import <AVFoundation/AVFoundation.h>

namespace fxgui {
AudioPermissionStatus audioPermissionStatus() {
    switch ([AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio]) {
        case AVAuthorizationStatusAuthorized: return AudioPermissionStatus::granted;
        case AVAuthorizationStatusNotDetermined: return AudioPermissionStatus::notDetermined;
        case AVAuthorizationStatusRestricted: return AudioPermissionStatus::restricted;
        default: return AudioPermissionStatus::denied;
    }
}
void requestAudioPermission(std::function<void(bool)> completion) {
    [AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio completionHandler:^(BOOL granted) {
        completion(granted);
    }];
}
}

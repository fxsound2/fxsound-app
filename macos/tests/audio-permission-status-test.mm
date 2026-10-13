#include "../gui/audio-permission.h"
#import <AVFoundation/AVFoundation.h>
#include <iostream>
#include <stdexcept>
int main() {
 @autoreleasepool {
  const auto native=[AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio];
  auto expected=fxgui::AudioPermissionStatus::denied;
  switch(native) {
   case AVAuthorizationStatusAuthorized:expected=fxgui::AudioPermissionStatus::granted;break;
   case AVAuthorizationStatusNotDetermined:expected=fxgui::AudioPermissionStatus::notDetermined;break;
   case AVAuthorizationStatusRestricted:expected=fxgui::AudioPermissionStatus::restricted;break;
   default:break;
  }
  if(fxgui::audioPermissionStatus()!=expected)throw std::runtime_error("native permission mapping mismatch");
  std::cout<<"Actual AVFoundation read-only status="<<int(native)<<" matches FxSound wrapper; no request/no helper/no audio/no app-identity claim\n";
 }
}

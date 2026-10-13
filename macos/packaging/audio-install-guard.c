#include <CoreAudio/CoreAudio.h>
#include <CoreFoundation/CoreFoundation.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int uid(AudioDeviceID device) {
    AudioObjectPropertyAddress a={kAudioDevicePropertyDeviceUID,kAudioObjectPropertyScopeGlobal,0};
    CFStringRef value=NULL; UInt32 size=sizeof(value);
    if(AudioObjectGetPropertyData(device,&a,0,NULL,&size,&value)!=noErr || !value) return -1;
    int match=CFEqual(value,CFSTR("FxSound_Mac_Virtual")); CFRelease(value); return match;
}
int main(int argc, char** argv) {
    int defaults_only=argc==2 && strcmp(argv[1],"--defaults-only")==0;
    if(argc>2 || (argc==2 && !defaults_only)) return 2;
    AudioObjectPropertySelector selectors[]={kAudioHardwarePropertyDefaultOutputDevice,
                                            kAudioHardwarePropertyDefaultSystemOutputDevice};
    for(unsigned i=0;i<2;i++) {
        AudioObjectPropertyAddress a={selectors[i],kAudioObjectPropertyScopeGlobal,0};
        AudioDeviceID device=0; UInt32 size=sizeof(device);
        if(AudioObjectGetPropertyData(kAudioObjectSystemObject,&a,0,NULL,&size,&device)!=noErr || !device) {
            fputs("Cannot verify a physical default output; operation refused.\n",stderr); return 1;
        }
        if(uid(device)!=0) { fputs("Restore physical default and system output before continuing.\n",stderr); return 1; }
    }
    if(defaults_only) { puts("Physical default/system outputs verified."); return 0; }
    AudioObjectPropertyAddress a={kAudioHardwarePropertyDevices,kAudioObjectPropertyScopeGlobal,0};
    UInt32 size=0;
    if(AudioObjectGetPropertyDataSize(kAudioObjectSystemObject,&a,0,NULL,&size)!=noErr) return 1;
    AudioDeviceID* devices=malloc(size); if(!devices && size) return 1;
    if(AudioObjectGetPropertyData(kAudioObjectSystemObject,&a,0,NULL,&size,devices)!=noErr) { free(devices); return 1; }
    for(UInt32 i=0;i<size/sizeof(*devices);i++) {
        int virtual=uid(devices[i]); if(virtual<0) { free(devices); return 1; }
        if(!virtual) continue;
        AudioObjectPropertyAddress running={kAudioDevicePropertyDeviceIsRunning,kAudioObjectPropertyScopeGlobal,0};
        UInt32 value=1,n=sizeof(value);
        if(AudioObjectGetPropertyData(devices[i],&running,0,NULL,&n,&value)!=noErr || value) {
            fputs("FxSound still has active audio clients; operation refused.\n",stderr); free(devices); return 1;
        }
    }
    free(devices); puts("Physical defaults verified; FxSound has no active HAL clients."); return 0;
}

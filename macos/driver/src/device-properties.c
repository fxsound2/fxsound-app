#include "driver.h"

OSStatus fx_device_value(AudioObjectID o,const AudioObjectPropertyAddress* a,
                         UInt32 qn,const void* q,UInt32* n,void* out) {
    UInt32 v=0; AudioObjectID ids[]={FX_INPUT,FX_OUTPUT,FX_VOLUME,FX_MUTE,FX_SOURCE};
    Boolean input=a->mScope==kAudioObjectPropertyScopeInput;
    Boolean output=a->mScope==kAudioObjectPropertyScopeOutput;
    Boolean global=a->mScope==kAudioObjectPropertyScopeGlobal;
    if(!input && !output && !global) return kAudioHardwareUnknownPropertyError;
    if(o==kAudioObjectPlugInObject) {
        switch(a->mSelector) {
        case kAudioPlugInPropertyBundleID: return fx_string(n,out,CFSTR("org.fxsound.virtual-audio"));
        case kAudioObjectPropertyOwnedObjects:
        case kAudioPlugInPropertyDeviceList: v=FX_DEVICE; break;
        case kAudioPlugInPropertyTranslateUIDToDevice: {
            CFStringRef uid=NULL;
            if(!out) { *n=sizeof(UInt32); return noErr; }
            if(qn!=sizeof(uid) || !q) return kAudioHardwareBadPropertySizeError;
            memcpy(&uid,q,sizeof(uid));
            if(uid && CFEqual(uid,CFSTR("FxSound_Mac_Virtual"))) v=FX_DEVICE;
            break;
        }
        case kAudioPlugInPropertyBoxList: return fx_copy(n,out,NULL,0);
        default: return kAudioHardwareUnknownPropertyError;
        }
        return fx_copy(n,out,&v,sizeof(v));
    }
    switch(a->mSelector) {
    case kAudioDevicePropertyDeviceUID: return fx_string(n,out,CFSTR("FxSound_Mac_Virtual"));
    case kAudioDevicePropertyModelUID: return fx_string(n,out,CFSTR("FxSound_Mac_Stereo48"));
    case kAudioDevicePropertyTransportType: v=kAudioDeviceTransportTypeVirtual; break;
    case kAudioDevicePropertyClockDomain: v=0; break;
    case kAudioDevicePropertyDeviceIsAlive: v=1; break;
    case kAudioDevicePropertyDeviceIsRunning: v=atomic_load(&fx_running)>0; break;
    case kAudioDevicePropertyDeviceCanBeDefaultDevice:
    case kAudioDevicePropertyDeviceCanBeDefaultSystemDevice: v=1; break;
    case kAudioDevicePropertyIsHidden: v=0; break;
    case kAudioDevicePropertyLatency: v=input?FX_DELAY:0; break;
    case kAudioDevicePropertySafetyOffset: v=0; break;
    case kAudioDevicePropertyZeroTimeStampPeriod: v=FX_PERIOD; break;
    case FX_VERSION: return fx_string(n,out,CFSTR("1"));
    case kAudioDevicePropertyNominalSampleRate: {
        Float64 rate=48000; return fx_copy(n,out,&rate,sizeof(rate));
    }
    case kAudioDevicePropertyAvailableNominalSampleRates: {
        AudioValueRange range={48000,48000}; return fx_copy(n,out,&range,sizeof(range));
    }
    case kAudioDevicePropertyRelatedDevices: v=FX_DEVICE; break;
    case kAudioObjectPropertyOwnedObjects:
        if(global) return fx_copy(n,out,ids,sizeof(ids));
        if(input) return fx_copy(n,out,ids,sizeof(AudioObjectID));
        return fx_copy(n,out,ids+1,4*sizeof(AudioObjectID));
    case kAudioDevicePropertyStreams:
        if(global) return fx_copy(n,out,ids,2*sizeof(AudioObjectID));
        return fx_copy(n,out,ids+(input?0:1),sizeof(AudioObjectID));
    case kAudioObjectPropertyControlList:
        return fx_copy(n,out,ids+2,3*sizeof(AudioObjectID));
    case kAudioDevicePropertyPreferredChannelsForStereo: {
        UInt32 channels[]={1,2}; return fx_copy(n,out,channels,sizeof(channels));
    }
    case kAudioDevicePropertyPreferredChannelLayout: {
        AudioChannelLayout layout={kAudioChannelLayoutTag_Stereo,0,0,{{0}}};
        return fx_copy(n,out,&layout,offsetof(AudioChannelLayout,mChannelDescriptions));
    }
    case kAudioDevicePropertyStreamConfiguration: {
        AudioBufferList buffers={1,{{2,0,NULL}}}; return fx_copy(n,out,&buffers,sizeof(buffers));
    }
    case kAudioObjectPropertyCustomPropertyInfoList: {
        AudioServerPlugInCustomPropertyInfo info={FX_VERSION,
            kAudioServerPlugInCustomPropertyDataTypeCFString,
            kAudioServerPlugInCustomPropertyDataTypeNone};
        return fx_copy(n,out,&info,sizeof(info));
    }
    default: return kAudioHardwareUnknownPropertyError;
    }
    return fx_copy(n,out,&v,sizeof(v));
}

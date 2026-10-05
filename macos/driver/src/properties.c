#include "driver.h"
#include <math.h>

Boolean fx_object(AudioObjectID o) { return o>=kAudioObjectPlugInObject && o<=FX_SOURCE; }
OSStatus fx_copy(UInt32* n, void* out, const void* value, UInt32 size) {
    if(out && *n<size) return kAudioHardwareBadPropertySizeError;
    if(out && size) memcpy(out,value,size);
    *n=size; return noErr;
}
OSStatus fx_string(UInt32* n, void* out, CFStringRef value) {
    if(out && *n<sizeof(value)) return kAudioHardwareBadPropertySizeError;
    if(out) { CFRetain(value); memcpy(out,&value,sizeof(value)); }
    *n=sizeof(value); return noErr;
}
AudioStreamBasicDescription fx_format(void) {
    return (AudioStreamBasicDescription){48000,kAudioFormatLinearPCM,
        kAudioFormatFlagsNativeFloatPacked,8,1,8,2,32,0};
}
void fx_notify(AudioObjectID o, AudioObjectPropertySelector s) {
    AudioObjectPropertyAddress a={s,kAudioObjectPropertyScopeGlobal,kAudioObjectPropertyElementMain};
    if(fx_host) fx_host->PropertiesChanged(fx_host,o,1,&a);
}
Boolean fx_settable(AudioObjectID o, AudioObjectPropertySelector s) {
    return (o==FX_VOLUME && (s==kAudioLevelControlPropertyScalarValue || s==kAudioLevelControlPropertyDecibelValue))
        || (o==FX_MUTE && s==kAudioBooleanControlPropertyValue)
        || (o==FX_SOURCE && s==kAudioSelectorControlPropertyCurrentItem)
        || ((o==FX_INPUT || o==FX_OUTPUT) && s==kAudioStreamPropertyIsActive);
}
OSStatus fx_value(AudioObjectID o,const AudioObjectPropertyAddress* a,UInt32 qn,const void* q,UInt32* n,void* out) {
    if(!a || !n) return kAudioHardwareIllegalOperationError;
    if(!fx_object(o)) return kAudioHardwareBadObjectError;
    if(a->mElement!=kAudioObjectPropertyElementMain) return kAudioHardwareUnknownPropertyError;
    if(a->mScope!=kAudioObjectPropertyScopeGlobal &&
       (o!=FX_DEVICE || (a->mScope!=kAudioObjectPropertyScopeInput && a->mScope!=kAudioObjectPropertyScopeOutput)))
        return kAudioHardwareUnknownPropertyError;
    if(o==FX_SOURCE && a->mSelector==kAudioSelectorControlPropertyItemName) {
        if(!out) { *n=sizeof(CFStringRef); return noErr; }
        UInt32 item=0;
        if(qn!=sizeof(item) || !q) return kAudioHardwareBadPropertySizeError;
        memcpy(&item,q,4);
        if(item!=1) return kAudioHardwareIllegalOperationError;
        return fx_string(n,out,CFSTR("Engine routing"));
    }
    UInt32 v=0;
    switch(a->mSelector) {
    case kAudioObjectPropertyBaseClass:
        v=o==1?kAudioObjectClassID:o==FX_DEVICE?kAudioObjectClassID:
          o<=FX_OUTPUT?kAudioObjectClassID:o==FX_VOLUME?kAudioLevelControlClassID:
          o==FX_MUTE?kAudioBooleanControlClassID:kAudioSelectorControlClassID; break;
    case kAudioObjectPropertyClass:
        v=o==1?kAudioPlugInClassID:o==FX_DEVICE?kAudioDeviceClassID:
          o<=FX_OUTPUT?kAudioStreamClassID:o==FX_VOLUME?kAudioVolumeControlClassID:
          o==FX_MUTE?kAudioMuteControlClassID:kAudioDataSourceControlClassID; break;
    case kAudioObjectPropertyOwner: v=o==1?0:o==FX_DEVICE?1:FX_DEVICE; break;
    case kAudioObjectPropertyManufacturer: return fx_string(n,out,CFSTR("FxSound"));
    case kAudioObjectPropertyName:
        return fx_string(n,out,o==FX_INPUT?CFSTR("FxSound Loopback"):
            o==FX_OUTPUT?CFSTR("FxSound Output"):CFSTR("FxSound Audio Enhancer (macOS)"));
    default:
        if(o<=FX_DEVICE) return fx_device_value(o,a,qn,q,n,out);
        return fx_stream_control_value(o,a,n,out);
    }
    return fx_copy(n,out,&v,sizeof(v));
}
OSStatus fx_set(AudioObjectID o,const AudioObjectPropertyAddress* a,UInt32 n,const void* data) {
    if(!a || !data) return kAudioHardwareIllegalOperationError;
    if(!fx_settable(o,a->mSelector)) return kAudioHardwareUnsupportedOperationError;
    if(n!=sizeof(UInt32)) return kAudioHardwareBadPropertySizeError;
    UInt32 v; memcpy(&v,data,4);
    if(o==FX_VOLUME) {
        float value; memcpy(&value,data,4);
        if(!isfinite(value)) return kAudioHardwareIllegalOperationError;
        if(a->mSelector==kAudioLevelControlPropertyDecibelValue) {
            if(value < -96 || value > 0) return kAudioHardwareIllegalOperationError;
            value=powf(10,value/20);
        }
        if(value<0 || value>1) return kAudioHardwareIllegalOperationError;
        memcpy(&v,&value,4); atomic_store(&fx_gain_bits,v);
        fx_notify(o,kAudioLevelControlPropertyScalarValue);
        fx_notify(o,kAudioLevelControlPropertyDecibelValue);
    } else if(o==FX_MUTE) {
        if(v>1) return kAudioHardwareIllegalOperationError;
        atomic_store(&fx_muted,v); fx_notify(o,a->mSelector);
    } else if(o==FX_SOURCE) {
        if(v!=1) return kAudioHardwareIllegalOperationError;
    } else {
        if(v>1) return kAudioHardwareIllegalOperationError;
        atomic_store(o==FX_INPUT?&fx_input_active:&fx_output_active,v); fx_notify(o,a->mSelector);
    }
    return noErr;
}

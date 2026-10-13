#include "driver.h"
#include <math.h>

OSStatus fx_stream_control_value(AudioObjectID o,const AudioObjectPropertyAddress* a,UInt32* n,void* out) {
    UInt32 v=0;
    if(a->mScope!=kAudioObjectPropertyScopeGlobal) return kAudioHardwareUnknownPropertyError;
    if(o<=FX_OUTPUT) {
        switch(a->mSelector) {
        case kAudioObjectPropertyOwnedObjects: return fx_copy(n,out,NULL,0);
        case kAudioStreamPropertyIsActive: v=atomic_load(o==FX_INPUT?&fx_input_active:&fx_output_active); break;
        case kAudioStreamPropertyDirection: v=o==FX_INPUT; break;
        case kAudioStreamPropertyTerminalType: v=kAudioStreamTerminalTypeLine; break;
        case kAudioStreamPropertyStartingChannel: v=1; break;
        case kAudioStreamPropertyLatency: v=0; break;
        case kAudioStreamPropertyVirtualFormat:
        case kAudioStreamPropertyPhysicalFormat: {
            AudioStreamBasicDescription f=fx_format(); return fx_copy(n,out,&f,sizeof(f));
        }
        case kAudioStreamPropertyAvailableVirtualFormats:
        case kAudioStreamPropertyAvailablePhysicalFormats: {
            AudioStreamRangedDescription f={fx_format(),{48000,48000}};
            return fx_copy(n,out,&f,sizeof(f));
        }
        default: return kAudioHardwareUnknownPropertyError;
        }
    } else {
        switch(a->mSelector) {
        case kAudioObjectPropertyOwnedObjects: return fx_copy(n,out,NULL,0);
        case kAudioControlPropertyScope: v=kAudioObjectPropertyScopeOutput; break;
        case kAudioControlPropertyElement: v=kAudioObjectPropertyElementMain; break;
        case kAudioLevelControlPropertyScalarValue:
        case kAudioLevelControlPropertyDecibelValue: {
            if(o!=FX_VOLUME) return kAudioHardwareUnknownPropertyError;
            UInt32 bits=atomic_load(&fx_gain_bits); float f; memcpy(&f,&bits,4);
            if(a->mSelector==kAudioLevelControlPropertyDecibelValue) f=f>0?fmaxf(-96,20*log10f(f)):-96;
            return fx_copy(n,out,&f,sizeof(f));
        }
        case kAudioLevelControlPropertyDecibelRange: {
            if(o!=FX_VOLUME) return kAudioHardwareUnknownPropertyError;
            AudioValueRange r={-96,0}; return fx_copy(n,out,&r,sizeof(r));
        }
        case kAudioLevelControlPropertyConvertScalarToDecibels:
        case kAudioLevelControlPropertyConvertDecibelsToScalar: {
            if(o!=FX_VOLUME) return kAudioHardwareUnknownPropertyError;
            if(out && *n<sizeof(float)) return kAudioHardwareBadPropertySizeError;
            if(out) {
                float f; memcpy(&f,out,4);
                if(!isfinite(f)) return kAudioHardwareIllegalOperationError;
                f=a->mSelector==kAudioLevelControlPropertyConvertScalarToDecibels?
                    (f>0?fmaxf(-96,20*log10f(f)):-96):powf(10,f/20);
                memcpy(out,&f,4);
            }
            *n=4; return noErr;
        }
        case kAudioBooleanControlPropertyValue:
            if(o!=FX_MUTE) return kAudioHardwareUnknownPropertyError;
            v=atomic_load(&fx_muted); break;
        case kAudioSelectorControlPropertyCurrentItem:
        case kAudioSelectorControlPropertyAvailableItems:
            if(o!=FX_SOURCE) return kAudioHardwareUnknownPropertyError;
            v=1; break;
        default: return kAudioHardwareUnknownPropertyError;
        }
    }
    return fx_copy(n,out,&v,sizeof(v));
}

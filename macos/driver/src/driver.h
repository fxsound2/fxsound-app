#ifndef FXSOUND_DRIVER_H
#define FXSOUND_DRIVER_H
#include <CoreAudio/AudioServerPlugIn.h>
#include <CoreAudio/AudioHardware.h>
#include <CoreFoundation/CFPlugInCOM.h>
#include <stdatomic.h>
#include <stdint.h>
#include <string.h>

enum { FX_DEVICE=2, FX_INPUT=3, FX_OUTPUT=4, FX_VOLUME=5, FX_MUTE=6, FX_SOURCE=7 };
enum { FX_CAPACITY=32768, FX_PERIOD=512, FX_DELAY=512, FX_VERSION='fxvr' };
extern AudioServerPlugInDriverRef fx_driver;
extern AudioServerPlugInHostRef fx_host;
extern _Atomic UInt32 fx_running, fx_gain_bits, fx_muted, fx_input_active, fx_output_active;
extern _Atomic UInt64 fx_anchor, fx_epoch;
extern double fx_ticks_per_frame;
Boolean fx_object(AudioObjectID object);
OSStatus fx_value(AudioObjectID, const AudioObjectPropertyAddress*, UInt32, const void*, UInt32*, void*);
OSStatus fx_device_value(AudioObjectID, const AudioObjectPropertyAddress*, UInt32, const void*, UInt32*, void*);
OSStatus fx_stream_control_value(AudioObjectID, const AudioObjectPropertyAddress*, UInt32*, void*);
Boolean fx_settable(AudioObjectID, AudioObjectPropertySelector);
void fx_notify(AudioObjectID, AudioObjectPropertySelector);
AudioStreamBasicDescription fx_format(void);
OSStatus fx_copy(UInt32*, void*, const void*, UInt32);
OSStatus fx_string(UInt32*, void*, CFStringRef);
OSStatus fx_set(AudioObjectID, const AudioObjectPropertyAddress*, UInt32, const void*);
OSStatus fx_start(AudioServerPlugInDriverRef,AudioObjectID,UInt32);
OSStatus fx_stop(AudioServerPlugInDriverRef,AudioObjectID,UInt32);
OSStatus fx_zero(AudioServerPlugInDriverRef,AudioObjectID,UInt32,Float64*,UInt64*,UInt64*);
OSStatus fx_will(AudioServerPlugInDriverRef,AudioObjectID,UInt32,UInt32,Boolean*,Boolean*);
OSStatus fx_cycle(AudioServerPlugInDriverRef,AudioObjectID,UInt32,UInt32,UInt32,const AudioServerPlugInIOCycleInfo*);
OSStatus fx_io(AudioServerPlugInDriverRef,AudioObjectID,AudioObjectID,UInt32,UInt32,UInt32,const AudioServerPlugInIOCycleInfo*,void*,void*);
#endif

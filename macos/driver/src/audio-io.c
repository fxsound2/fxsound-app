#include "driver.h"
#include <mach/mach_time.h>
#include <math.h>
#include <pthread.h>

_Static_assert(ATOMIC_LLONG_LOCK_FREE==2 && ATOMIC_INT_LOCK_FREE==2,"Realtime atomics must be lock-free");
_Atomic UInt32 fx_running=0, fx_gain_bits=0x3f800000, fx_muted=0, fx_input_active=1, fx_output_active=1;
_Atomic UInt64 fx_anchor=0, fx_epoch=1;
double fx_ticks_per_frame=0;
typedef struct { _Atomic UInt64 stamp, epoch; _Atomic UInt32 left,right; } FxFrame;
static FxFrame ring[FX_CAPACITY];
static pthread_mutex_t lifecycle=PTHREAD_MUTEX_INITIALIZER;
static UInt32 clients[64];
static _Atomic UInt64 clock_sequence=0;

OSStatus fx_start(AudioServerPlugInDriverRef d,AudioObjectID o,UInt32 c) {
    if(d!=fx_driver || o!=FX_DEVICE) return kAudioHardwareBadObjectError;
    pthread_mutex_lock(&lifecycle);
    UInt32 count=atomic_load(&fx_running),slot=64;
    for(UInt32 i=0;i<count;i++) if(clients[i]==c) { pthread_mutex_unlock(&lifecycle); return kAudioHardwareIllegalOperationError; }
    if(count<64) slot=count;
    if(slot==64) { pthread_mutex_unlock(&lifecycle); return kAudioHardwareIllegalOperationError; }
    if(count==0) {
        atomic_fetch_add(&clock_sequence,1);
        atomic_fetch_add(&fx_epoch,1); atomic_store(&fx_anchor,mach_absolute_time());
        atomic_fetch_add(&clock_sequence,1);
    }
    clients[slot]=c; atomic_store(&fx_running,count+1);
    pthread_mutex_unlock(&lifecycle);
    if(count==0) fx_notify(FX_DEVICE,kAudioDevicePropertyDeviceIsRunning);
    return noErr;
}
OSStatus fx_stop(AudioServerPlugInDriverRef d,AudioObjectID o,UInt32 c) {
    if(d!=fx_driver || o!=FX_DEVICE) return kAudioHardwareBadObjectError;
    pthread_mutex_lock(&lifecycle);
    UInt32 count=atomic_load(&fx_running),slot=count;
    for(UInt32 i=0;i<count;i++) if(clients[i]==c) slot=i;
    if(slot==count) { pthread_mutex_unlock(&lifecycle); return kAudioHardwareIllegalOperationError; }
    clients[slot]=clients[count-1]; atomic_store(&fx_running,--count);
    if(count==0) {
        atomic_fetch_add(&clock_sequence,1); atomic_fetch_add(&fx_epoch,1);
        atomic_fetch_add(&clock_sequence,1);
    }
    pthread_mutex_unlock(&lifecycle);
    if(count==0) fx_notify(FX_DEVICE,kAudioDevicePropertyDeviceIsRunning);
    return noErr;
}
OSStatus fx_zero(AudioServerPlugInDriverRef d,AudioObjectID o,UInt32 c,Float64* sample,UInt64* host,UInt64* seed) {
    (void)c;
    if(d!=fx_driver || o!=FX_DEVICE) return kAudioHardwareBadObjectError;
    if(!sample || !host || !seed || fx_ticks_per_frame<=0) return kAudioHardwareIllegalOperationError;
    UInt64 anchor=0,generation=0; Boolean coherent=false;
    for(UInt32 attempt=0;attempt<3;attempt++) {
        UInt64 before=atomic_load(&clock_sequence);
        if(before&1) continue;
        anchor=atomic_load(&fx_anchor); generation=atomic_load(&fx_epoch);
        if(before==atomic_load(&clock_sequence)) { coherent=true; break; }
    }
    UInt64 now=mach_absolute_time();
    if(!coherent || now<anchor) return kAudioHardwareIllegalOperationError;
    UInt64 periods=(UInt64)((double)(now-anchor)/(fx_ticks_per_frame*FX_PERIOD));
    *sample=(Float64)periods*FX_PERIOD;
    *host=anchor+(UInt64)(*sample*fx_ticks_per_frame); *seed=generation;
    return noErr;
}
OSStatus fx_will(AudioServerPlugInDriverRef d,AudioObjectID o,UInt32 c,UInt32 op,Boolean* yes,Boolean* inplace) {
    (void)c;
    if(d!=fx_driver || o!=FX_DEVICE) return kAudioHardwareBadObjectError;
    if(!yes || !inplace) return kAudioHardwareIllegalOperationError;
    *yes=op==kAudioServerPlugInIOOperationReadInput || op==kAudioServerPlugInIOOperationWriteMix;
    *inplace=true; return noErr;
}
OSStatus fx_cycle(AudioServerPlugInDriverRef d,AudioObjectID o,UInt32 c,UInt32 op,UInt32 frames,const AudioServerPlugInIOCycleInfo* info) {
    (void)c;(void)op;(void)info;
    if(d!=fx_driver || o!=FX_DEVICE) return kAudioHardwareBadObjectError;
    return frames<=FX_CAPACITY?noErr:kAudioHardwareIllegalOperationError;
}
OSStatus fx_io(AudioServerPlugInDriverRef d,AudioObjectID o,AudioObjectID stream,UInt32 c,UInt32 op,
               UInt32 frames,const AudioServerPlugInIOCycleInfo* info,void* buffer,void* secondary) {
    (void)c;(void)secondary;
    if(d!=fx_driver || o!=FX_DEVICE) return kAudioHardwareBadObjectError;
    if(!info || !buffer || frames>FX_CAPACITY) return kAudioHardwareIllegalOperationError;
    Boolean write=op==kAudioServerPlugInIOOperationWriteMix;
    if((write && stream!=FX_OUTPUT) || (!write && stream!=FX_INPUT)) return kAudioHardwareBadObjectError;
    if(!write && op!=kAudioServerPlugInIOOperationReadInput) return kAudioHardwareUnsupportedOperationError;
    if(!atomic_load(&fx_running) || !atomic_load(write?&fx_output_active:&fx_input_active)) {
        if(!write) memset(buffer,0,frames*8);
        return noErr;
    }
    const AudioTimeStamp* time=write?&info->mOutputTime:&info->mInputTime;
    if(!(time->mFlags&kAudioTimeStampSampleTimeValid) || !isfinite(time->mSampleTime)
       || time->mSampleTime<0 || time->mSampleTime>4503599627370496.0) return kAudioHardwareIllegalOperationError;
    UInt64 start=(UInt64)time->mSampleTime,epoch=atomic_load(&fx_epoch);
    float* samples=buffer,gain; UInt32 bits=atomic_load(&fx_gain_bits); memcpy(&gain,&bits,4);
    if(atomic_load(&fx_muted)) gain=0;
    for(UInt32 i=0;i<frames;i++) {
        UInt64 frame=start+i;
        if(!write && frame<FX_DELAY) { samples[2*i]=samples[2*i+1]=0; continue; }
        if(!write) frame-=FX_DELAY;
        FxFrame* slot=&ring[frame%FX_CAPACITY]; UInt64 tag=frame*2+2;
        if(write) {
            atomic_store(&slot->stamp,tag-1);
            float left=samples[2*i]*gain,right=samples[2*i+1]*gain;
            UInt32 l,r; memcpy(&l,&left,4); memcpy(&r,&right,4);
            atomic_store(&slot->left,l); atomic_store(&slot->right,r); atomic_store(&slot->epoch,epoch);
            atomic_store(&slot->stamp,tag);
        } else {
            UInt64 before=atomic_load(&slot->stamp),generation=atomic_load(&slot->epoch);
            UInt32 l=atomic_load(&slot->left),r=atomic_load(&slot->right);
            UInt64 after=atomic_load(&slot->stamp);
            if(before!=tag || after!=tag || generation!=epoch || !atomic_load(&fx_running)) l=r=0;
            memcpy(samples+2*i,&l,4); memcpy(samples+2*i+1,&r,4);
        }
    }
    return noErr;
}

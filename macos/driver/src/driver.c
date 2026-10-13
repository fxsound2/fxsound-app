#include "driver.h"
#include <mach/mach_time.h>

AudioServerPlugInHostRef fx_host=NULL;
static _Atomic UInt32 refs=1;
static HRESULT query(void* d,REFIID uuid,LPVOID* out) {
    if(d!=fx_driver || !out) return E_INVALIDARG;
    *out=NULL; CFUUIDRef id=CFUUIDCreateFromUUIDBytes(NULL,uuid);
    Boolean supported=CFEqual(id,IUnknownUUID) || CFEqual(id,kAudioServerPlugInDriverInterfaceUUID);
    CFRelease(id);
    if(!supported) return E_NOINTERFACE;
    atomic_fetch_add(&refs,1); *out=fx_driver; return S_OK;
}
static ULONG add(void* d) { return d==fx_driver?atomic_fetch_add(&refs,1)+1:0; }
static ULONG release(void* d) {
    if(d!=fx_driver) return 0;
    UInt32 current=atomic_load(&refs);
    while(current && !atomic_compare_exchange_weak(&refs,&current,current-1)) {}
    return current?current-1:0;
}
static OSStatus initialize(AudioServerPlugInDriverRef d,AudioServerPlugInHostRef host) {
    if(d!=fx_driver || !host) return kAudioHardwareIllegalOperationError;
    mach_timebase_info_data_t time; if(mach_timebase_info(&time)!=KERN_SUCCESS) return kAudioHardwareIllegalOperationError;
    fx_ticks_per_frame=(1e9/48000)*time.denom/time.numer;
    atomic_store(&fx_anchor,mach_absolute_time()); fx_host=host; return noErr;
}
static OSStatus create(AudioServerPlugInDriverRef d,CFDictionaryRef desc,const AudioServerPlugInClientInfo* c,AudioObjectID* out) {
    (void)d;(void)desc;(void)c;(void)out; return kAudioHardwareUnsupportedOperationError;
}
static OSStatus destroy(AudioServerPlugInDriverRef d,AudioObjectID o) {
    (void)d;(void)o; return kAudioHardwareUnsupportedOperationError;
}
static OSStatus client(AudioServerPlugInDriverRef d,AudioObjectID o,const AudioServerPlugInClientInfo* c) {
    if(d!=fx_driver || o!=FX_DEVICE) return kAudioHardwareBadObjectError;
    return c?noErr:kAudioHardwareIllegalOperationError;
}
static OSStatus config(AudioServerPlugInDriverRef d,AudioObjectID o,UInt64 action,void* info) {
    (void)action;(void)info;
    if(d!=fx_driver || o!=FX_DEVICE) return kAudioHardwareBadObjectError;
    return kAudioHardwareUnsupportedOperationError;
}
static Boolean has(AudioServerPlugInDriverRef d,AudioObjectID o,pid_t pid,const AudioObjectPropertyAddress* a) {
    (void)pid; UInt32 size=0;
    if(d!=fx_driver) return false;
    return fx_value(o,a,0,NULL,&size,NULL)==noErr;
}
static OSStatus settable(AudioServerPlugInDriverRef d,AudioObjectID o,pid_t pid,const AudioObjectPropertyAddress* a,Boolean* out) {
    if(!out) return kAudioHardwareIllegalOperationError;
    if(!has(d,o,pid,a)) return fx_object(o)?kAudioHardwareUnknownPropertyError:kAudioHardwareBadObjectError;
    *out=fx_settable(o,a->mSelector); return noErr;
}
static OSStatus size(AudioServerPlugInDriverRef d,AudioObjectID o,pid_t pid,const AudioObjectPropertyAddress* a,
                     UInt32 qn,const void* q,UInt32* out) {
    (void)pid;
    if(d!=fx_driver) return kAudioHardwareBadObjectError;
    if(!out) return kAudioHardwareIllegalOperationError;
    return fx_value(o,a,qn,q,out,NULL);
}
static OSStatus get(AudioServerPlugInDriverRef d,AudioObjectID o,pid_t pid,const AudioObjectPropertyAddress* a,
                    UInt32 qn,const void* q,UInt32 capacity,UInt32* out,void* data) {
    (void)pid;
    if(d!=fx_driver) return kAudioHardwareBadObjectError;
    if(!out || (!data && capacity)) return kAudioHardwareIllegalOperationError;
    *out=capacity; return fx_value(o,a,qn,q,out,data);
}
static OSStatus set(AudioServerPlugInDriverRef d,AudioObjectID o,pid_t pid,const AudioObjectPropertyAddress* a,
                    UInt32 qn,const void* q,UInt32 n,const void* data) {
    (void)pid;(void)qn;(void)q;
    if(d!=fx_driver || !fx_object(o)) return kAudioHardwareBadObjectError;
    if(!a || a->mScope!=kAudioObjectPropertyScopeGlobal || a->mElement!=kAudioObjectPropertyElementMain)
        return kAudioHardwareUnknownPropertyError;
    return fx_set(o,a,n,data);
}
static AudioServerPlugInDriverInterface interface={NULL,query,add,release,initialize,create,destroy,
    client,client,config,config,has,settable,size,get,set,fx_start,fx_stop,fx_zero,fx_will,fx_cycle,fx_io,fx_cycle};
static AudioServerPlugInDriverInterface* pointer=&interface;
AudioServerPlugInDriverRef fx_driver=&pointer;
__attribute__((visibility("default"))) void* FxSound_Create(CFAllocatorRef allocator,CFUUIDRef type) {
    (void)allocator;
    return type && CFEqual(type,kAudioServerPlugInTypeUUID)?fx_driver:NULL;
}

// PTNOTE - can be downloaded from http://code.google.com/p/soundprison/source/browse/trunk/ipolicyconfig.h?spec=svn4&r=4
// ----------------------------------------------------------------------------
// PolicyConfig.h
// Undocumented COM-interface IPolicyConfig.
// Use for set default audio render endpoint
// @author EreTIk
// ----------------------------------------------------------------------------
/* Some additional notes from a post by EreTIk
MmSys.cpl use PolicyConfig interface for disable/enable playback device. Call SetEndpointVisibility method.
First parameter is device ID (obtained by calling IMMDevice::GetId), second parameter is bool: 0 - disable, 1 - enable
*** add: method IPolicyConfigVista::SetEndpointVisibility on Win7 return error (see remark in PolicyConfig.h). For Win7 need call IPolicyConfig::SetEndpointVisibility.
In the general case it is necessary to do so:
    try get IPolicyConfig interface    if error - get and use IPolicyConfigVista */

// Updated notes, 11/23/15 - for the functionality needed through DFX12, ie setting device samp freq. and format and setting default device,
// the functions in CPolicyConfigVistaClient are used as they work in Vista through Win10. However the function to set a devices ENABLED/DISABLED status
// requires a different implementation for Win7 and later, as noted in the Vista version function SetEndpointVisibility().
// See the Win7 and Win10 versions below, not sure which version functions with Win8/8.1, it made need a separate declaration also.

#pragma once

// PTNOTE - these are the original declarations that were Win7 specific, for use with ENABLE/DISABLE functions.
interface DECLSPEC_UUID("f8679f50-850a-41cf-9c72-430f290290c8")
IPolicyConfig;
class DECLSPEC_UUID("870af99c-171d-4f9e-af0d-e63df40c2bc9")
CPolicyConfigClient;

// ----------------------------------------------------------------------------
// class CPolicyConfigClient
// {870af99c-171d-4f9e-af0d-e63df40c2bc9}
//  
// interface IPolicyConfig
// {f8679f50-850a-41cf-9c72-430f290290c8}
//
// Query interface:
// CComPtr<IPolicyConfig> PolicyConfig;
// PolicyConfig.CoCreateInstance(__uuidof(CPolicyConfigClient));
// 
// @compatible: Windows 7 and Later (but not Win10, see versions below, not sure about Win8/8.1
// ----------------------------------------------------------------------------
interface IPolicyConfig : public IUnknown
{
public:

    virtual HRESULT GetMixFormat(
        PCWSTR,
        WAVEFORMATEX **
    );

    virtual HRESULT STDMETHODCALLTYPE GetDeviceFormat(
        PCWSTR,
        INT,	// BOOL, TRUE -> get default OEM format, FALSE -> get current format.
        WAVEFORMATEXTENSIBLE **
    );

    virtual HRESULT STDMETHODCALLTYPE ResetDeviceFormat(
        PCWSTR
    );

    virtual HRESULT STDMETHODCALLTYPE SetDeviceFormat(
        PCWSTR,
        WAVEFORMATEXTENSIBLE *,	// The format to set
        WAVEFORMATEXTENSIBLE *	// Unknown, set to NULL
    );

    virtual HRESULT STDMETHODCALLTYPE GetProcessingPeriod(
        PCWSTR,
        INT,
        PINT64,
        PINT64
    );

    virtual HRESULT STDMETHODCALLTYPE SetProcessingPeriod(
        PCWSTR,
        PINT64
    );

    virtual HRESULT STDMETHODCALLTYPE GetShareMode(
        PCWSTR,
        struct DeviceShareMode *
    );

    virtual HRESULT STDMETHODCALLTYPE SetShareMode(
        PCWSTR,
        struct DeviceShareMode *
    );

    virtual HRESULT STDMETHODCALLTYPE GetPropertyValue(
        PCWSTR,
        const PROPERTYKEY &,
        PROPVARIANT *
    );

    virtual HRESULT STDMETHODCALLTYPE SetPropertyValue(
        PCWSTR,
        const PROPERTYKEY &,
        PROPVARIANT *
    );

    virtual HRESULT STDMETHODCALLTYPE SetDefaultEndpoint(
        __in PCWSTR wszDeviceId,
        __in ERole eRole 
    );

    virtual HRESULT STDMETHODCALLTYPE SetEndpointVisibility(
        PCWSTR,
        INT
    );
};

interface DECLSPEC_UUID("568b9108-44bf-40b4-9006-86afe5b5a620")
IPolicyConfigVista;
class DECLSPEC_UUID("294935CE-F637-4E7C-A41B-AB255460B862")
CPolicyConfigVistaClient;
// ----------------------------------------------------------------------------
// class CPolicyConfigVistaClient
// {294935CE-F637-4E7C-A41B-AB255460B862}
//  
// interface IPolicyConfigVista
// {568b9108-44bf-40b4-9006-86afe5b5a620}
//
// Query interface:
// CComPtr<IPolicyConfigVista> PolicyConfig;
// PolicyConfig.CoCreateInstance(__uuidof(CPolicyConfigVistaClient));
// 
// @compatible: Windows Vista and Later
// ----------------------------------------------------------------------------
interface IPolicyConfigVista : public IUnknown
{
public:

    virtual HRESULT GetMixFormat(
        PCWSTR,
        WAVEFORMATEX **
    );  // not available on Windows 7, use method from IPolicyConfig

    virtual HRESULT STDMETHODCALLTYPE GetDeviceFormat(
        PCWSTR,
        INT,		// BOOL, TRUE -> get default format, FALSE -> get current format.
        WAVEFORMATEXTENSIBLE **
    );

	 virtual HRESULT STDMETHODCALLTYPE SetDeviceFormat(
        PCWSTR,
        WAVEFORMATEXTENSIBLE *,	// The format to set
        WAVEFORMATEXTENSIBLE *	// Unknown, set to NULL
    );
    virtual HRESULT STDMETHODCALLTYPE GetProcessingPeriod(
        PCWSTR,
        INT,
        PINT64,
        PINT64
    );  // not available on Windows 7, use method from IPolicyConfig

    virtual HRESULT STDMETHODCALLTYPE SetProcessingPeriod(
        PCWSTR,
        PINT64
    );  // not available on Windows 7, use method from IPolicyConfig

    virtual HRESULT STDMETHODCALLTYPE GetShareMode(
        PCWSTR,
        struct DeviceShareMode *
    );  // not available on Windows 7, use method from IPolicyConfig

    virtual HRESULT STDMETHODCALLTYPE SetShareMode(
        PCWSTR,
        struct DeviceShareMode *
    );  // not available on Windows 7, use method from IPolicyConfig

    virtual HRESULT STDMETHODCALLTYPE GetPropertyValue(
        PCWSTR,
        const PROPERTYKEY &,
        PROPVARIANT *
    );

    virtual HRESULT STDMETHODCALLTYPE SetPropertyValue(
        PCWSTR,
        const PROPERTYKEY &,
        PROPVARIANT *
    );

    virtual HRESULT STDMETHODCALLTYPE SetDefaultEndpoint(
        __in PCWSTR wszDeviceId,
        __in ERole eRole 
    );

    virtual HRESULT STDMETHODCALLTYPE SetEndpointVisibility(
        PCWSTR,
        INT
    );  // not available on Windows 7, use method from IPolicyConfig
};


// Suggested declarations for Win10 from PolicyConfig.h author (changes only IPolicyConfig), would need to make interface vars OS dependent.
// See PtDriverNotes.txt for more on this.
interface DECLSPEC_UUID("ca286fc3-91fd-42c3-8e9b-caafa66242e3") // changed from Win7 version, apparently .dll verison dependent, see Aleksey emails.
IPolicyConfigWin10;
//class DECLSPEC_UUID("870af99c-171d-4f9e-af0d-e63df40c2bc9")	// This was not changed from Win7 version and proto worked, Aleksey said that was ok, need to confirm that, Vista version is different.
class DECLSPEC_UUID("7f419449-07f6-4654-9b05-c0a2db1abf12") // A newly generated GUID to avoid any potential conflict.
CPolicyConfigWin10Client;

// ----------------------------------------------------------------------------
// class CPolicyConfigWin10Client
// {870af99c-171d-4f9e-af0d-e63df40c2bc9}
//  
// interface IPolicyConfigWin10
// {ca286fc3-91fd-42c3-8e9b-caafa66242e3}
//
// Query interface:
// CComPtr<IPolicyConfigWin10> PolicyConfig;
// PolicyConfig.CoCreateInstance(__uuidof(CPolicyConfigWin10Client));
// 
// @compatible: Windows 10 and Later
// ----------------------------------------------------------------------------
interface IPolicyConfigWin10 : public IUnknown
{
public:

    virtual HRESULT GetMixFormat(
        PCWSTR,
        WAVEFORMATEX **
    );

    virtual HRESULT STDMETHODCALLTYPE GetDeviceFormat(
        PCWSTR,
        INT,	// BOOL, TRUE -> get default OEM format, FALSE -> get current format.
        WAVEFORMATEXTENSIBLE **
    );

    virtual HRESULT STDMETHODCALLTYPE ResetDeviceFormat(
        PCWSTR
    );

    virtual HRESULT STDMETHODCALLTYPE SetDeviceFormat(
        PCWSTR,
        WAVEFORMATEXTENSIBLE *,	// The format to set
        WAVEFORMATEXTENSIBLE *	// Unknown, set to NULL
    );

    virtual HRESULT STDMETHODCALLTYPE GetProcessingPeriod(
        PCWSTR,
        INT,
        PINT64,
        PINT64
    );

    virtual HRESULT STDMETHODCALLTYPE SetProcessingPeriod(
        PCWSTR,
        PINT64
    );

    virtual HRESULT STDMETHODCALLTYPE GetShareMode(
        PCWSTR,
        struct DeviceShareMode *
    );

    virtual HRESULT STDMETHODCALLTYPE SetShareMode(
        PCWSTR,
        struct DeviceShareMode *
    );

    virtual HRESULT STDMETHODCALLTYPE GetPropertyValue(
        PCWSTR,
        const PROPERTYKEY &,
        PROPVARIANT *
    );

    virtual HRESULT STDMETHODCALLTYPE SetPropertyValue(
        PCWSTR,
        const PROPERTYKEY &,
        PROPVARIANT *
    );

    virtual HRESULT STDMETHODCALLTYPE SetDefaultEndpoint(
        __in PCWSTR wszDeviceId,
        __in ERole eRole 
    );

    virtual HRESULT STDMETHODCALLTYPE SetEndpointVisibility(
        PCWSTR,
        INT
    );
};


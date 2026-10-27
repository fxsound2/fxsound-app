/*
FxSound
Copyright (C) 2025  FxSound LLC

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "AudioPassthru.h"
#include "u_AudioPassthru.h"
#include "sndDevices.h"



AudioPassthru::AudioPassthru()
{
	data_ = new AudioPassthruPrivate();
}

AudioPassthru::~AudioPassthru()
{
	delete data_;
}

int AudioPassthru::init()
{
    try 
    {
        return data_->init();
    }
    catch (...)
    {
        return(NOT_OKAY_NO_BREAK);
    }
}

void AudioPassthru::setDspProcessingModule(DfxDsp* p_dfx_dsp)
{
	data_->setDspProcessingModule(p_dfx_dsp);
}


std::vector<SoundDevice> AudioPassthru::getSoundDevices(bool active_devices)
{
	return data_->getSoundDevices(active_devices);
}

void AudioPassthru::setAsPlaybackDevice(const SoundDevice sound_device)
{
	data_->setTargetedRealPlaybackDevice(sound_device.pwszID);
}

/*
* FUNCTION: setBufferLength()
* DESCRIPTION:
*
*  Sets the buffer length.
*
*/
int AudioPassthru::setBufferLength(int i_buffer_length_msecs)
{
	return data_->setBufferLength(i_buffer_length_msecs);
}

/*
* FUNCTION: processTimer()
* DESCRIPTION:
*
*  Process the snd server timer.
*/
int AudioPassthru::processTimer()
{
	auto ret = data_->processTimer();
    if (!data_->isPlaybackDeviceAvailable())
    {
        return(NOT_OKAY_NO_BREAK);
    }
    return ret;
}


void AudioPassthru::mute(bool mute)
{
	data_->mute(mute);
}

void AudioPassthru::registerCallback(AudioPassthruCallback* callback)
{
	data_->registerCallback(callback);
}

bool AudioPassthru::isPlaybackDeviceAvailable()
{
    return data_->isPlaybackDeviceAvailable();
}

bool AudioPassthru::checkDeviceChanges()
{
    return data_->checkDeviceChanges();
}

void AudioPassthru::restoreDefaultPlaybackDevice()
{
    data_->restoreDefaultPlaybackDevice();
}

std::wstring AudioPassthru::getInputDeviceId()
{
	return data_->getRoutingSetting(SND_DEVICES_REGISTRY_USER_SELECTED_INPUT_WIDE);
}

int AudioPassthru::setInputDeviceId(const std::wstring& device_id)
{
	return data_->setRoutingSetting(SND_DEVICES_REGISTRY_USER_SELECTED_INPUT_WIDE, device_id);
}

bool AudioPassthru::isAutomaticDefaultEnabled()
{
	return data_->isAutomaticDefaultEnabled();
}

int AudioPassthru::setAutomaticDefaultEnabled(bool enabled)
{
	return data_->setAutomaticDefaultEnabled(enabled);
}

std::wstring AudioPassthru::getPinnedOutputDeviceId()
{
	return data_->getRoutingSetting(SND_DEVICES_REGISTRY_USER_SELECTED_PLAYBACK_WIDE);
}

int AudioPassthru::setPinnedOutputDeviceId(const std::wstring& device_id)
{
	return data_->setRoutingSetting(SND_DEVICES_REGISTRY_USER_SELECTED_PLAYBACK_WIDE, device_id);
}

int AudioPassthru::getRoutingStatus()
{
	return data_->getRoutingStatus();
}

int AudioPassthru::stopProcessing()
{
	// The controller stops its timer before calling this; capture stays stopped until power-on.
	return data_->restartRouting();
}

int AudioPassthru::refreshRouting()
{
	return data_->refreshRouting();
}

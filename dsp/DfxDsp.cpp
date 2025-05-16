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
#include "DfxDsp.h"
#include "u_DfxDsp.h"

DfxDsp::DfxDsp()
{
	data_ = new DfxDspPrivate();
}


DfxDsp::~DfxDsp()
{
	delete data_;
}

void DfxDsp::powerOn(bool on)
{
	data_->powerOn(on);
}

bool DfxDsp::isPowerOn()
{
	return data_->isPowerOn();
}

float DfxDsp::getEffectValue(Effect effect)
{
	return data_->getEffectValue(effect);
}

void DfxDsp::setEffectValue(Effect effect, float value)
{
	data_->setEffectValue(effect, value);
}

int DfxDsp::loadPreset(std::wstring preset_file_full_path)
{
	return data_->loadPreset(preset_file_full_path);
}

int DfxDsp::savePreset(std::wstring preset_name, std::wstring preset_file_full_path)
{
	return data_->savePreset(preset_name, preset_file_full_path);
}

int DfxDsp::exportPreset(std::wstring preset_source_file_full_path, std::wstring preset_name, std::wstring preset_export_path)
{
	return data_->exportPreset(preset_source_file_full_path, preset_name, preset_export_path);
}

void DfxDsp::eqOn(bool on)
{
	data_->eqOn(on);
}

int DfxDsp::getNumEqBands()
{
	return data_->getNumEqBands();
}

DfxPreset DfxDsp::getPresetInfo(std::wstring preset_file_full_path)
{
	return data_->getPresetInfo(preset_file_full_path);
}

unsigned long DfxDsp::getTotalAudioProcessedTime()
{
	return data_->getTotalAudioProcessedTime();
}

void DfxDsp::resetTotalAudioProcessedTime()
{
	data_->resetTotalAudioProcessedTime();
}

int DfxDsp::setSignalFormat(int i_bps, int i_nch, int i_srate, int i_valid_bits)
{
	if (data_->being_destroyed_)
	{
		return OKAY;
	} else
	{
		return data_->setSignalFormat(i_bps, i_nch, i_srate, i_valid_bits);
	}
}

int DfxDsp::processAudio(short int *si_input_samples, short int *si_output_samples, int i_num_sample_sets, int i_check_for_duplicate_buffers)
{
	if (data_->being_destroyed_)
	{
		return OKAY;
	}
	else
	{
		return data_->processAudio(si_input_samples, si_output_samples, i_num_sample_sets, i_check_for_duplicate_buffers);
	}
}

float DfxDsp::getEqBandFrequency(int band_num)
{
	return data_->getEqBandFrequency(band_num);
}

void DfxDsp::setEqBandFrequency(int band_num, float freq)
{
    data_->setEqBandFrequency(band_num, freq);
}

void DfxDsp::getEqBandFrequencyRange(int band_num, float* min_freq, float* max_freq)
{
    data_->getEqBandFrequencyRange(band_num, min_freq, max_freq);
}

float DfxDsp::getEqBandBoostCut(int band_num)
{
	return data_->getEqBandBoostCut(band_num);
}

void DfxDsp::setEqBandBoostCut(int band_num, float boost)
{
	data_->setEqBandBoostCut(band_num, boost);
}

void DfxDsp::getSpectrumBandValues(float* rp_band_values, int i_array_size)
{
    data_->getSpectrumBandValues(rp_band_values, i_array_size);
}

void DfxDsp::setVolumeNormalization(float target_rms)
{
	data_->setVolumeNormalization(target_rms);
}
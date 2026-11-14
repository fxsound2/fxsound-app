/*
FxSound
Copyright (C) 2025  FxSound LLC

Contributors:
	www.theremino.com (2025)
	
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
#pragma once
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include "AudioPassthru.h"
#include "codedefs.h"
#include "DfxDsp.h"
#include "pt_defs.h"
#include "slout.h"
#include "FiltLowCut.h"
#include "FiltChain.h"
#include "FilterEngine.h"

struct dfxg_section_type {
	realtype value;
	int fader_x_center;
	int bypass;
	PT_HANDLE *fader_loc_to_rval_qnt;
	PT_HANDLE *rval_to_fader_loc_qnt;
};

/*
* Information which is product specific.  In other words,
* this info is different for DFX products vs. MP3 Remix products.
*/
struct dfxg_product_specific_info_type {
	realtype full_version; 	                            /* Full Product version (7.2003) */
	int major_version; 	                               /* Major version number (ex. 7) */
	wchar_t wcp_registry_product_name[PT_MAX_GENERIC_STRLEN];  /* Product Name as used in the registry */
	wchar_t wcp_displayed_product_name[PT_MAX_GENERIC_STRLEN]; /* Product Name as displayed to the user */
};



/*
* Information specific to the particular vendor type.  Most of these settings come
* in as parameters to dfxgInit() however some are inferred by those parameters.
*/
struct dfxg_vendor_specific_info_type {
	int vendor_code;
	int subvendor_code;
	int allow_close;
	int i_freemium_version;
	int display_close_dialog;
	long splash_display_frequency_secs;
	int remix_capabilities;
	int standalone_mode;
	int allow_recording;
	int oem_build;
};

class CDerivedSlout1 : public CSlout {
public:
	int Display(int, char *);
	int Error(int, char *);

	int Display_Wide(int, wchar_t *);
	int Error_Wide(int, wchar_t *);

	PT_HANDLE *hp_dfxg;
};

class DfxDspPrivate
{
public:
	DfxDspPrivate();
	~DfxDspPrivate();
	int processAudio(short int *si_input_samples, short int *si_output_samples, int i_num_sample_sets, int i_check_for_duplicate_buffers);
	int setSignalFormat(int i_bps, int i_nch, int i_srate, int i_valid_bits);
	int loadPreset(std::wstring preset_file_full_path);
	int savePreset(std::wstring preset_name, std::wstring preset_file_full_path);
	int exportPreset(std::wstring preset_source_file_full_path, std::wstring preset_name, std::wstring preset_export_path);
	int resetEQ();
	void eqOn(bool on);
	int getNumEqBands();
	void filtersOn(bool on);
	bool isFiltersOn();
	void setCutSlope(int db_per_octave);
	int getCutSlope();
	void lowCutOn(bool on);
	bool isLowCutOn();
	void setLowCutCutoff(float hz);
	float getLowCutCutoff();
	void highCutOn(bool on);
	bool isHighCutOn();
	void setHighCutCutoff(float hz);
	float getHighCutCutoff();
	void monoBassOn(bool on);
	bool isMonoBassOn();
	void setMonoBassCutoff(float hz);
	float getMonoBassCutoff();
	bool isFilterPreviewAvailable(int preview_flags);
	void setFilterPreview(int preview_flags);
	int getFilterPreview();
	float getBalance();
	void setBalance(float gain_db);
	float getNormalization();
	void setNormalization(float gain_db);
	float getVolumeLeveling();
	void setVolumeLeveling(float gain_db);
	float getMasterGain();
	void setMasterGain(float gain_db);
	float getFilterQ();
	void setFilterQ(float q_multiplier);
	void setNumBands(int num_bands);
    float getEqBandFrequency(int band_num);
    void setEqBandFrequency(int band_num, float freq);
    void getEqBandFrequencyRange(int band_num, float* min_freq, float* max_freq);
    bool getDefaultEqBandFrequency(int band_num, float* freq);
	float getEqBandBoostCut(int band_num);
	void setEqBandBoostCut(int band_num, float boost);
	void powerOn(bool on);
	bool isPowerOn();
	float getEffectValue(DfxDsp::Effect effect);
	void setEffectValue(DfxDsp::Effect effect, float value);
	DfxPreset getPresetInfo(std::wstring preset_file_full_path);
	unsigned long getTotalAudioProcessedTime();
	void resetTotalAudioProcessedTime();
    void getSpectrumBandValues(float* rp_band_values, int i_array_size);

	std::atomic<bool> being_destroyed_{ false };
private:
	void processTimer();
	int eqUpdateFromRegistry(int *ip_eq_changed);
	int getStateInfoFromVals(PT_HANDLE *hp_vals, bool b_include_eq = true);
	int getGraphicEqInfoFromVals(PT_HANDLE *hp_vals);
	int createValsFromStateInfo(wchar_t *preset_name, PT_HANDLE **hpp_vals);

	// DfxDspPrivate.cpp
	int dfxpFreeAll();

	// DfxDspRegistry.cpp
	int writeRegistrySessionLongValue(long l_value, wchar_t *wcp_key_name);

	// DfxDspEq.cpp
	int eqSetProcessingOn(int i_storage_type, int i_on);
	int eqGetProcessingOn(int i_storage_type, int *ip_on);

	void applyFilters(float *audio_buffer, int num_sample_sets);
	void requestFilterDesign();
	void filterDesignerThreadMain();
	static HANDLE createFilterDesignerEvent();
	int filterDesiredSource(bool filter_on, unsigned int reset_serial);
	FilterSettings filterSettings();
	bool beginFilterChange(const FilterSettings &next, int requested_preview);
	bool beginFilterChange(const FilterSettings &next);
	void setStageOn(int stage_bit, std::atomic<bool> &stage_on, bool on);
	void setStageCutoff(int stage_bit, std::atomic<float> &stage_cutoff, float clamped_hz);

	// Handles
	int *dfxp_handle_;
	int *preset_list_handle_;
	int *midi_to_rval_qnt_handle_; // Midi to Real Value
	int *rval_to_midi_qnt_handle_; // and visa versa QNT handles

	CDerivedSlout1 *slout1_;

	// Section specific information
	struct dfxg_section_type fidelity_;
	struct dfxg_section_type ambience_;
	struct dfxg_section_type surround_;
	struct dfxg_section_type dynamic_boost_;
	struct dfxg_section_type bass_boost_;

	bool update_from_registry_ = true;
	int headphone_on_;
	int music_mode_;     /* DFXP_MUSIC_MODE_MUSIC1, DFXP_MUSIC_MODE_MUSIC2, DFXP_MUSIC_MODE_SPEECH */

	struct dfxg_vendor_specific_info_type vendor_specific_;
	struct dfxg_product_specific_info_type product_specific_;

	int eq_processing_on_;

	std::atomic<bool> filters_on_{ false };
	std::atomic<bool> low_cut_on_{ true };
	std::atomic<bool> high_cut_on_{ true };
	std::atomic<bool> mono_bass_on_{ false };
	std::atomic<bool> filter_power_on_{ true };
	std::atomic<int> cut_slope_db_{ DfxDsp::CUT_DEFAULT_SLOPE_DB };
	std::atomic<float> low_cut_cutoff_hz_{ (float)DfxDsp::LOW_CUT_DEFAULT_CUTOFF_HZ };
	std::atomic<float> high_cut_cutoff_hz_{ (float)DfxDsp::HIGH_CUT_DEFAULT_CUTOFF_HZ };
	std::atomic<float> mono_bass_cutoff_hz_{ (float)DfxDsp::MONO_BASS_DEFAULT_CUTOFF_HZ };
	std::atomic<int> filter_preview_{ DfxDsp::PreviewNone };
	int filter_cached_sample_rate_ = 44100;
	int filter_cached_num_channels_ = 2;
	int filter_cached_bps_ = 32;
	unsigned int filter_format_serial_ = 0;

	FilterIirExchange filter_iir_exchange_;
	FilterAudioState filter_audio_;

	std::atomic<int> filter_request_sample_rate_{ 44100 };
	std::atomic<unsigned int> filter_request_generation_{ 1 };
	std::atomic<unsigned int> filter_reset_serial_{ 0 };
	std::atomic<bool> filter_designer_stop_{ false };
	// Auto-reset event that wakes the designer. SetEvent never blocks and a signal set before the designer waits is
	// kept, so any thread (the audio thread included) can wake it without a lock and no wakeup is lost. Atomic because
	// the designer recreates it if creation failed in the constructor (see filterDesignerThreadMain).
	std::atomic<HANDLE> filter_designer_event_{ NULL };
	// How often the designer checks for requests while it has no event.
	static constexpr DWORD FILTER_DESIGNER_POLL_MS = 50;
	// Held by the GUI-thread filter setters around a reset-serial bump and the store it guards, and by the designer
	// while it reads the serial and settings, so a design never pairs a new serial with the settings from before it.
	// Never taken on the audio thread.
	std::mutex filter_settings_mutex_;
	std::thread filter_designer_thread_;
};


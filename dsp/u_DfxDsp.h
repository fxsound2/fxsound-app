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
#include <condition_variable>
#include "AudioPassthru.h"
#include "codedefs.h"
#include "DfxDsp.h"
#include "pt_defs.h"
#include "slout.h"
#include "FiltLowCut.h"

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

struct LowCutPreviewLevelCoeffs {
	float rms;
	float rise;
	float fall;
};

struct LowCutIirParams {
	FiltLowCutSection hp_sections[FILT_LOW_CUT_MAX_SECTIONS];
	FiltLowCutSection lp_sections[FILT_LOW_CUT_MAX_SECTIONS];
	int num_hp_sections;
	int num_lp_sections;
	int sample_rate;
	unsigned int reset_serial;
};

#define LOW_CUT_IIR_NUM_SLOTS   3
#define LOW_CUT_IIR_SLOT_DIRTY  0x4
#define LOW_CUT_IIR_SLOT_MASK   0x3

struct LowCutIirExchange {
	LowCutIirParams slots[LOW_CUT_IIR_NUM_SLOTS];
	int front_slot;
	int back_slot;
	std::atomic<int> middle_slot;
};

#define LOW_CUT_SOURCE_DRY  0
#define LOW_CUT_SOURCE_IIR  1
#define LOW_CUT_NUM_SOURCES 2

#define LOW_CUT_CROSSFADE_MS  20.0
#define LOW_CUT_FADE_MS       10.0

struct LowCutAudioState {
	int ramp_rate;
	int crossfade_frames;
	int fade_frames;

	int source;
	int crossfade_from;
	int crossfade_pos;
	int gain_pos;
	int gain_dir;

	int preview_pos;
	bool preview_was_on;
	float preview_mean_square;
	float preview_gain;
	LowCutPreviewLevelCoeffs preview_level_coeffs;

	LowCutIirParams iir_params[2];
	FiltLowCutChannelState iir_states[2][FILT_LOW_CUT_MAX_CHANNELS];
	int iir_cur;
	bool iir_crossfading;
	int iir_crossfade_pos;
	bool iir_running;
	int iir_channels;
	unsigned int format_serial;
	bool iir_keep_blocked;
	LowCutIirParams iir_taken;
	bool iir_taken_pending;
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
	void lowCutOn(bool on);
	bool isLowCutOn();
	void setLowCutSlope(int db_per_octave);
	int getLowCutSlope();
	void setLowCutQ(float q);
	float getLowCutQ();
	void lowCutPreviewOn(bool on);
	bool isLowCutPreviewOn();
	void setLowCutCutoff(float cutoff_hz);
	float getLowCutCutoff();
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

	void applyLowCut(float *audio_buffer, int num_sample_sets);
	void requestLowCutDesign(bool wake_designer);
	void lowCutDesignerThreadMain();
	int lowCutDesiredSource(bool filter_on, unsigned int reset_serial);

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

	std::atomic<bool> low_cut_on_{ false };
	std::atomic<bool> low_cut_preview_on_{ false };
	std::atomic<bool> low_cut_power_on_{ true };
	std::atomic<int> low_cut_iir_slope_db_{ DfxDsp::LOW_CUT_DEFAULT_SLOPE_DB };
	std::atomic<float> low_cut_iir_q_{ DfxDsp::LOW_CUT_DEFAULT_Q };
	std::atomic<float> low_cut_cutoff_hz_{ (float)DfxDsp::LOW_CUT_DEFAULT_CUTOFF_HZ };
	int low_cut_cached_sample_rate_ = 44100;
	int low_cut_cached_num_channels_ = 2;
	int low_cut_cached_bps_ = 32;
	unsigned int low_cut_format_serial_ = 0;

	LowCutIirExchange low_cut_iir_exchange_;
	LowCutAudioState low_cut_audio_;

	std::atomic<int> low_cut_request_sample_rate_{ 44100 };
	std::atomic<unsigned int> low_cut_request_generation_{ 1 };
	std::atomic<unsigned int> low_cut_reset_serial_{ 0 };
	std::atomic<bool> low_cut_designer_stop_{ false };
	std::mutex low_cut_designer_mutex_;
	std::condition_variable low_cut_designer_wake_;
	std::thread low_cut_designer_thread_;
};


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

/* Standard includes */
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <cmath>

#include "codedefs.h"
#include "slout.h"
#include "vals.h"
#include "mry.h"
#include "filt.h"
#include "sos.h"
#include "u_sos.h"

namespace
{
    constexpr realtype kVolumeLevelingCeiling = 1.0f;
    constexpr realtype kVolumeLevelingAttackAlpha = 0.10f;
    constexpr realtype kVolumeLevelingReleaseAlphaFast = 0.05f;
    constexpr realtype kVolumeLevelingReleaseAlphaSlow = 0.02f;
    constexpr realtype kVolumeLevelingReleaseGapThreshold = 0.15f;
    constexpr realtype kVolumeLevelingPredictionStrength = 0.35f;
    constexpr realtype kVolumeLevelingPredictionClamp = 0.15f;
    constexpr realtype kVolumeLevelingPredictionMissRatio = 0.35f;
    constexpr realtype kVolumeLevelingSidechainHpfHz = 120.0f;
    constexpr realtype kVolumeLevelingToneLowHz = 180.0f;
    constexpr realtype kVolumeLevelingToneBodyHz = 1200.0f;
    constexpr realtype kVolumeLevelingTonePresenceHz = 4500.0f;
    constexpr realtype kVolumeLevelingMinRatioPerBuffer = 0.8912509381337456f; // 10 ^ (-1 dB / 20)
    constexpr realtype kVolumeLevelingTonalityDbRange = 7.0f;
    constexpr realtype kVolumeLevelingTonalitySmoothing = 0.08f;
    constexpr realtype kVolumeLevelingMuffledTargetBoost = 0.12f;
    constexpr realtype kVolumeLevelingClearTargetReduction = 0.18f;
    constexpr realtype kVolumeLevelingClearCeilingReduction = 0.08f;
    constexpr realtype kVolumeLevelingHeadroomTimeSeconds = 60.0f;
    constexpr realtype kVolumeLevelingHeadroomTargetBoost = 0.08f;
    constexpr realtype kVolumeLevelingHeadroomTargetReduction = 0.14f;
    constexpr realtype kVolumeLevelingHeadroomComfortThreshold = 0.18f;
    constexpr realtype kVolumeLevelingHeadroomNearCeilingThreshold = 0.985f;
    constexpr realtype kVolumeLevelingHeadroomHitThreshold = 0.002f;
    constexpr realtype kVolumeLevelingVeryQuietRmsThreshold = 0.035f;
    constexpr realtype kVolumeLevelingQuietAudiblePeakThreshold = 0.0035f;
    constexpr realtype kVolumeLevelingQuietFullBoostPeak = 0.02f;
    constexpr realtype kVolumeLevelingQuietMaxGain = 10.0f;
    constexpr realtype kVolumeLevelingQuietReleaseAlpha = 0.18f;
    constexpr realtype kVolumeLevelingQuietActivationSeconds = 10.0f;
    constexpr realtype kVolumeLevelingQuietActivationRampSeconds = 2.0f;
    constexpr realtype kVolumeLevelingQuietFloorReleaseRmsThreshold = 0.06f;
    constexpr realtype kVolumeLevelingQuietFloorReleaseAlpha = 0.02f;
    constexpr realtype kVolumeLevelingQuietFloorSilenceDecayAlpha = 0.08f;
    constexpr realtype kVolumeLevelingQuietPeakBucketSeconds = 1.0f;
    constexpr realtype kVolumeLevelingQuietPeakTargetRatio = 0.98f;
    constexpr realtype kVolumeLevelingQuietPeakFloorRaiseTimeSeconds = 6.0f;
    constexpr realtype kPi = 3.14159265358979323846f;

    static realtype clampReal(realtype value, realtype min_value, realtype max_value)
    {
        return std::fmax(min_value, std::fmin(value, max_value));
    }

    static realtype calcOnePoleAlpha(realtype cutoff_hz, realtype sample_rate)
    {
        realtype dt = 1.0f / sample_rate;
        realtype rc = 1.0f / ((realtype)2.0f * kPi * cutoff_hz);
        return dt / (rc + dt);
    }

    static void updateVolumeLevelingCoefficients(struct sosHdlType* cast_handle, realtype sample_rate)
    {
        if (cast_handle->volume_leveling_alpha_sample_rate == sample_rate)
            return;

        realtype dt = 1.0f / sample_rate;
        realtype rc = 1.0f / ((realtype)2.0f * kPi * kVolumeLevelingSidechainHpfHz);
        cast_handle->volume_leveling_sc_hpf_alpha = rc / (rc + dt);
        cast_handle->volume_leveling_tone_low_alpha = calcOnePoleAlpha(kVolumeLevelingToneLowHz, sample_rate);
        cast_handle->volume_leveling_tone_body_alpha = calcOnePoleAlpha(kVolumeLevelingToneBodyHz, sample_rate);
        cast_handle->volume_leveling_tone_presence_alpha = calcOnePoleAlpha(kVolumeLevelingTonePresenceHz, sample_rate);
        cast_handle->volume_leveling_alpha_sample_rate = sample_rate;
    }

    static void updateQuietPeakWindow(struct sosHdlType* cast_handle,
                                      realtype post_gain_peak_abs,
                                      realtype buffer_duration_seconds)
    {
        cast_handle->volume_leveling_quiet_peak_bucket_max =
            std::fmax(cast_handle->volume_leveling_quiet_peak_bucket_max, post_gain_peak_abs);
        cast_handle->volume_leveling_quiet_peak_bucket_seconds += buffer_duration_seconds;

        while (cast_handle->volume_leveling_quiet_peak_bucket_seconds >= kVolumeLevelingQuietPeakBucketSeconds)
        {
            cast_handle->volume_leveling_quiet_peak_history[
                cast_handle->volume_leveling_quiet_peak_history_index] =
                cast_handle->volume_leveling_quiet_peak_bucket_max;

            cast_handle->volume_leveling_quiet_peak_history_index =
                (cast_handle->volume_leveling_quiet_peak_history_index + 1) % SOS_VOLUME_LEVELING_PEAK_WINDOW_SIZE;

            if (cast_handle->volume_leveling_quiet_peak_history_count < SOS_VOLUME_LEVELING_PEAK_WINDOW_SIZE)
                cast_handle->volume_leveling_quiet_peak_history_count++;

            cast_handle->volume_leveling_quiet_peak_bucket_seconds -= kVolumeLevelingQuietPeakBucketSeconds;
            cast_handle->volume_leveling_quiet_peak_bucket_max = (realtype)0.0f;
        }
    }

    static realtype getQuietPeakWindowMax(const struct sosHdlType* cast_handle)
    {
        realtype rolling_peak_max = cast_handle->volume_leveling_quiet_peak_bucket_max;
        for (int i = 0; i < cast_handle->volume_leveling_quiet_peak_history_count; i++)
        {
            rolling_peak_max = std::fmax(rolling_peak_max, cast_handle->volume_leveling_quiet_peak_history[i]);
        }
        return rolling_peak_max;
    }

    static void applyVolumeLeveling(struct sosHdlType* cast_handle,
                                    realtype* rp_out_buf,
                                    int i_num_sample_sets,
                                    int i_num_channels,
                                    realtype r_samp_freq,
                                    int excluded_channel)
    {
        if ((cast_handle->volume_leveling_target_rms <= 0.0f) || (i_num_sample_sets <= 0) || (i_num_channels <= 0))
        {
            cast_handle->volume_leveling_gain = 1.0f;
            return;
        }

        int analyzed_channels = (excluded_channel >= 0) ? (i_num_channels - 1) : i_num_channels;
        if (analyzed_channels <= 0)
            return;

        realtype sum_squares = 0.0f;
        realtype peak = 0.0f;
        realtype low_energy = 0.0f;
        realtype body_energy = 0.0f;
        realtype presence_energy = 0.0f;
        realtype air_energy = 0.0f;
        int index = 0;
        realtype effective_sample_rate = (r_samp_freq > 1000.0f) ? r_samp_freq : 48000.0f;
        updateVolumeLevelingCoefficients(cast_handle, effective_sample_rate);

        for (int sample = 0; sample < i_num_sample_sets; sample++)
        {
            for (int channel = 0; channel < i_num_channels; channel++)
            {
                if (channel == excluded_channel)
                    continue;

                realtype value = rp_out_buf[index + channel];
                realtype sc_prev_in = cast_handle->volume_leveling_sc_prev_in[channel];
                realtype sc_prev_out = cast_handle->volume_leveling_sc_prev_out[channel];
                realtype sc_value = cast_handle->volume_leveling_sc_hpf_alpha * (sc_prev_out + value - sc_prev_in);
                cast_handle->volume_leveling_sc_prev_in[channel] = value;
                cast_handle->volume_leveling_sc_prev_out[channel] = sc_value;

                sum_squares += sc_value * sc_value;

                realtype abs_value = std::fabs(sc_value);
                if (abs_value > peak)
                    peak = abs_value;

                realtype* tone_state = cast_handle->volume_leveling_tone_lp_state[channel];
                tone_state[0] += cast_handle->volume_leveling_tone_low_alpha * (value - tone_state[0]);
                tone_state[1] += cast_handle->volume_leveling_tone_body_alpha * (value - tone_state[1]);
                tone_state[2] += cast_handle->volume_leveling_tone_presence_alpha * (value - tone_state[2]);

                realtype low_band = tone_state[0];
                realtype body_band = tone_state[1] - tone_state[0];
                realtype presence_band = tone_state[2] - tone_state[1];
                realtype air_band = value - tone_state[2];

                low_energy += low_band * low_band;
                body_energy += body_band * body_band;
                presence_energy += presence_band * presence_band;
                air_energy += air_band * air_band;
            }

            index += i_num_channels;
        }

        realtype current_power = sum_squares / (i_num_sample_sets * analyzed_channels);
        realtype current_rms = sqrtf(current_power);
        realtype gain_start = cast_handle->volume_leveling_gain;
        realtype gain_end = gain_start;
        realtype headroom_reduce_score = std::fmax(cast_handle->volume_leveling_headroom_score, (realtype)0.0f);
        realtype headroom_boost_score = std::fmax(-cast_handle->volume_leveling_headroom_score, (realtype)0.0f);
        realtype quiet_gain_floor = std::fmax(cast_handle->volume_leveling_quiet_gain_floor, (realtype)1.0f);
        realtype quiet_duration_before = cast_handle->volume_leveling_quiet_duration_seconds;
        realtype tonality_db =
            (realtype)(10.0f * log10((double)((presence_energy + air_energy * 0.75f + 1e-12f) /
                                              (body_energy * 1.15f + low_energy * 0.85f + 1e-12f))));
        realtype target_tonality_score = clampReal(tonality_db / kVolumeLevelingTonalityDbRange, -1.0f, 1.0f);
        cast_handle->volume_leveling_tonality_score =
            cast_handle->volume_leveling_tonality_score * (1.0f - kVolumeLevelingTonalitySmoothing) +
            target_tonality_score * kVolumeLevelingTonalitySmoothing;

        realtype clear_score = std::fmax(cast_handle->volume_leveling_tonality_score, (realtype)0.0f);
        realtype muffled_score = std::fmax(-cast_handle->volume_leveling_tonality_score, (realtype)0.0f);
        realtype buffer_duration_seconds = (realtype)i_num_sample_sets / effective_sample_rate;
        realtype tonal_upward_authority = clampReal(
            1.0f - headroom_reduce_score * 1.5f + headroom_boost_score * 0.25f,
            0.0f,
            1.0f);
        realtype tonal_downward_authority = clampReal(
            0.35f + tonal_upward_authority * 0.65f,
            0.35f,
            1.0f);
        realtype guarded_muffled_score = muffled_score * tonal_upward_authority;
        realtype guarded_clear_score = clear_score * tonal_downward_authority;
        realtype effective_target_rms = cast_handle->volume_leveling_target_rms *
            (1.0f
                + guarded_muffled_score * kVolumeLevelingMuffledTargetBoost
                - guarded_clear_score * kVolumeLevelingClearTargetReduction
                + headroom_boost_score * kVolumeLevelingHeadroomTargetBoost
                - headroom_reduce_score * kVolumeLevelingHeadroomTargetReduction);
        realtype effective_ceiling = clampReal(
            kVolumeLevelingCeiling * (1.0f - guarded_clear_score * kVolumeLevelingClearCeilingReduction),
            0.92f,
            kVolumeLevelingCeiling);
        realtype max_gain_cap = std::fmax(effective_target_rms / 0.125f, quiet_gain_floor);

        if (cast_handle->volume_leveling_power_count == SOS_VOLUME_LEVELING_HISTORY_SIZE)
        {
            cast_handle->volume_leveling_power_sum -=
                cast_handle->volume_leveling_power_history[cast_handle->volume_leveling_power_index];
        }
        else
        {
            cast_handle->volume_leveling_power_count++;
        }

        cast_handle->volume_leveling_power_history[cast_handle->volume_leveling_power_index] = current_power;
        cast_handle->volume_leveling_power_sum += current_power;
        cast_handle->volume_leveling_power_index =
            (cast_handle->volume_leveling_power_index + 1) % SOS_VOLUME_LEVELING_HISTORY_SIZE;

        realtype averaged_rms = current_rms;
        if (cast_handle->volume_leveling_power_count > 0)
        {
            averaged_rms = sqrtf(cast_handle->volume_leveling_power_sum / cast_handle->volume_leveling_power_count);
        }

        realtype predicted_rms = averaged_rms;
        bool prediction_missed = false;
        if (cast_handle->volume_leveling_previous_average_rms > 1e-6f)
        {
            realtype previous_average_rms = cast_handle->volume_leveling_previous_average_rms;
            realtype previous_predicted_rms = cast_handle->volume_leveling_previous_predicted_rms;
            realtype predicted_delta = previous_predicted_rms - previous_average_rms;
            realtype actual_delta = averaged_rms - previous_average_rms;
            realtype miss_threshold = std::fmax(previous_average_rms * 0.01f, 1e-5f);

            if (std::fabs(predicted_delta) > miss_threshold)
            {
                bool wrong_direction = (predicted_delta * actual_delta) < 0.0f;
                bool overshot = std::fabs(actual_delta) < (std::fabs(predicted_delta) * kVolumeLevelingPredictionMissRatio);
                if (wrong_direction || overshot)
                {
                    prediction_missed = true;
                }
            }
        }

        if (cast_handle->volume_leveling_previous_average_rms > 1e-6f)
        {
            realtype gradient = averaged_rms - cast_handle->volume_leveling_previous_average_rms;
            predicted_rms += gradient * kVolumeLevelingPredictionStrength;

            realtype min_predicted_rms = averaged_rms * (1.0f - kVolumeLevelingPredictionClamp);
            realtype max_predicted_rms = averaged_rms * (1.0f + kVolumeLevelingPredictionClamp);
            predicted_rms = clampReal(predicted_rms, min_predicted_rms, max_predicted_rms);
        }

        if (prediction_missed)
        {
            predicted_rms = averaged_rms;
        }

        predicted_rms = std::fmax(predicted_rms, 1e-6f);
        cast_handle->volume_leveling_previous_average_rms = averaged_rms;
        cast_handle->volume_leveling_previous_predicted_rms = predicted_rms;

        if (current_rms > 1e-6f)
        {
            realtype quiet_activation_score = clampReal(
                (cast_handle->volume_leveling_quiet_duration_seconds - kVolumeLevelingQuietActivationSeconds) /
                    kVolumeLevelingQuietActivationRampSeconds,
                0.0f,
                1.0f);
            realtype quiet_rms_score = clampReal(
                (kVolumeLevelingVeryQuietRmsThreshold - predicted_rms) / kVolumeLevelingVeryQuietRmsThreshold,
                0.0f,
                1.0f);
            realtype audible_peak_score = clampReal(
                (peak - kVolumeLevelingQuietAudiblePeakThreshold) /
                    (kVolumeLevelingQuietFullBoostPeak - kVolumeLevelingQuietAudiblePeakThreshold),
                0.0f,
                1.0f);
            realtype quiet_detected_score = quiet_rms_score * audible_peak_score;
            realtype quiet_boost_score = quiet_detected_score * quiet_activation_score;
            if (quiet_boost_score > 0.0f)
            {
                realtype quiet_gain_cap = std::fmax(max_gain_cap, kVolumeLevelingQuietMaxGain);
                max_gain_cap = max_gain_cap + (quiet_gain_cap - max_gain_cap) * quiet_boost_score;
            }

            realtype desired_gain = effective_target_rms / predicted_rms;
            desired_gain = std::fmin(desired_gain, max_gain_cap);
            desired_gain = std::fmax(desired_gain, quiet_gain_floor);

            realtype alpha = kVolumeLevelingAttackAlpha;
            if (desired_gain >= cast_handle->volume_leveling_gain)
            {
                realtype release_gap = desired_gain - cast_handle->volume_leveling_gain;
                realtype release_gap_ratio = release_gap / std::fmax(cast_handle->volume_leveling_gain, (realtype)1e-6f);
                alpha = (release_gap_ratio > kVolumeLevelingReleaseGapThreshold)
                    ? kVolumeLevelingReleaseAlphaSlow
                    : kVolumeLevelingReleaseAlphaFast;

                alpha *= clampReal(1.0f + guarded_muffled_score * 0.20f - guarded_clear_score * 0.35f, 0.55f, 1.20f);
                alpha = std::fmax(alpha, kVolumeLevelingQuietReleaseAlpha * quiet_boost_score);
            }
            gain_end = cast_handle->volume_leveling_gain * (1.0f - alpha) + desired_gain * alpha;
        }

        // Avoid abrupt "crushed" sound: limit how much gain can drop within one buffer.
        if (gain_end < gain_start)
        {
            const realtype min_allowed_gain_end = gain_start * kVolumeLevelingMinRatioPerBuffer;
            if (gain_end < min_allowed_gain_end)
                gain_end = min_allowed_gain_end;
        }

        if (peak > 1e-6f)
        {
            realtype peak_safe_gain = effective_ceiling / peak;
            if (gain_end > peak_safe_gain)
                gain_end = peak_safe_gain;
            if (gain_start > peak_safe_gain)
                gain_start = peak_safe_gain;
        }

        gain_start = clampReal(gain_start, 0.0f, max_gain_cap);
        gain_end = clampReal(gain_end, 0.0f, max_gain_cap);
        cast_handle->volume_leveling_gain = gain_end;

        index = 0;
        realtype post_gain_sum_squares = 0.0f;
        realtype post_gain_peak_abs = 0.0f;
        int ceiling_hit_count = 0;
        for (int sample = 0; sample < i_num_sample_sets; sample++)
        {
            realtype t = (realtype)sample / (realtype)i_num_sample_sets;
            realtype gain = gain_start + t * (gain_end - gain_start);

            for (int channel = 0; channel < i_num_channels; channel++)
            {
                if (channel == excluded_channel)
                    continue;

                rp_out_buf[index + channel] *= gain;
                realtype post_gain_abs = std::fabs(rp_out_buf[index + channel]);
                post_gain_sum_squares += rp_out_buf[index + channel] * rp_out_buf[index + channel];
                if (post_gain_abs > post_gain_peak_abs)
                    post_gain_peak_abs = post_gain_abs;
                if (post_gain_abs >= (effective_ceiling * kVolumeLevelingHeadroomNearCeilingThreshold))
                    ceiling_hit_count++;

                if (rp_out_buf[index + channel] > effective_ceiling)
                    rp_out_buf[index + channel] = effective_ceiling;
                else if (rp_out_buf[index + channel] < -effective_ceiling)
                    rp_out_buf[index + channel] = -effective_ceiling;
            }

            index += i_num_channels;
        }

        realtype post_gain_rms = sqrtf(post_gain_sum_squares / (i_num_sample_sets * analyzed_channels));
        updateQuietPeakWindow(cast_handle, post_gain_peak_abs, buffer_duration_seconds);
        realtype rolling_peak_max = getQuietPeakWindowMax(cast_handle);
        bool post_gain_still_quiet =
            peak > kVolumeLevelingQuietAudiblePeakThreshold &&
            post_gain_rms < kVolumeLevelingVeryQuietRmsThreshold;
        if (post_gain_still_quiet)
        {
            cast_handle->volume_leveling_quiet_duration_seconds += buffer_duration_seconds;
        }
        else
        {
            cast_handle->volume_leveling_quiet_duration_seconds = 0.0f;
        }

        bool quiet_boost_had_authority =
            quiet_duration_before >= kVolumeLevelingQuietActivationSeconds &&
            gain_end > (effective_target_rms / 0.125f);
        if (quiet_boost_had_authority && gain_end > quiet_gain_floor)
        {
            quiet_gain_floor = gain_end;
        }

        bool quiet_peak_window_ready =
            cast_handle->volume_leveling_quiet_peak_history_count == SOS_VOLUME_LEVELING_PEAK_WINDOW_SIZE;
        bool quiet_floor_is_active =
            quiet_duration_before >= kVolumeLevelingQuietActivationSeconds || quiet_gain_floor > 1.0f;
        realtype quiet_peak_target = effective_ceiling * kVolumeLevelingQuietPeakTargetRatio;
        bool sustained_headroom_available =
            rolling_peak_max > kVolumeLevelingQuietAudiblePeakThreshold &&
            rolling_peak_max < quiet_peak_target &&
            headroom_reduce_score < 0.25f;
        if (quiet_peak_window_ready && quiet_floor_is_active && sustained_headroom_available)
        {
            realtype desired_quiet_floor =
                quiet_gain_floor * (quiet_peak_target / std::fmax(rolling_peak_max, (realtype)1e-6f));
            desired_quiet_floor = clampReal(desired_quiet_floor, quiet_gain_floor, kVolumeLevelingQuietMaxGain);

            realtype quiet_floor_raise_alpha = clampReal(
                buffer_duration_seconds / kVolumeLevelingQuietPeakFloorRaiseTimeSeconds,
                0.0005f,
                0.05f);
            quiet_gain_floor =
                quiet_gain_floor * (1.0f - quiet_floor_raise_alpha) +
                desired_quiet_floor * quiet_floor_raise_alpha;
        }

        if (peak <= kVolumeLevelingQuietAudiblePeakThreshold)
        {
            quiet_gain_floor += (1.0f - quiet_gain_floor) * kVolumeLevelingQuietFloorSilenceDecayAlpha;
        }
        else if (post_gain_rms > kVolumeLevelingQuietFloorReleaseRmsThreshold)
        {
            quiet_gain_floor += (1.0f - quiet_gain_floor) * kVolumeLevelingQuietFloorReleaseAlpha;
        }

        if (quiet_gain_floor < 1.0001f)
        {
            quiet_gain_floor = 1.0f;
        }
        cast_handle->volume_leveling_quiet_gain_floor = quiet_gain_floor;

        realtype ceiling_hit_ratio = (realtype)ceiling_hit_count / (realtype)(i_num_sample_sets * analyzed_channels);
        realtype headroom_ratio = clampReal((effective_ceiling - post_gain_peak_abs) / effective_ceiling, 0.0f, 1.0f);
        realtype target_headroom_score = 0.0f;

        if (ceiling_hit_ratio > kVolumeLevelingHeadroomHitThreshold)
        {
            target_headroom_score = clampReal(ceiling_hit_ratio / 0.02f, 0.0f, 1.0f);
        }
        else if (post_gain_peak_abs >= (effective_ceiling * kVolumeLevelingHeadroomNearCeilingThreshold))
        {
            target_headroom_score = clampReal(
                (post_gain_peak_abs / effective_ceiling - kVolumeLevelingHeadroomNearCeilingThreshold) /
                (1.0f - kVolumeLevelingHeadroomNearCeilingThreshold),
                0.0f,
                1.0f);
        }
        else if (headroom_ratio > kVolumeLevelingHeadroomComfortThreshold)
        {
            target_headroom_score = -clampReal(
                (headroom_ratio - kVolumeLevelingHeadroomComfortThreshold) / 0.25f,
                0.0f,
                1.0f);
        }

        realtype headroom_alpha = clampReal(buffer_duration_seconds / kVolumeLevelingHeadroomTimeSeconds, 0.0005f, 0.05f);
        cast_handle->volume_leveling_headroom_score =
            cast_handle->volume_leveling_headroom_score * (1.0f - headroom_alpha) +
            target_headroom_score * headroom_alpha;
    }
}


/*
 * FUNCTION: sosProcessBuffer_MasterGainOnly()
 * DESCRIPTION:
 *   Processes the passed in buffer with MASTER_GAIN only.
 */
int PT_DECLSPEC sosProcessBuffer_MasterGainOnly(PT_HANDLE* hp_sos, realtype* rp_in_buf, realtype* rp_out_buf, int i_num_sample_sets, int i_num_channels)
{
    struct sosHdlType* cast_handle;
    int total_samples, i;

    cast_handle = (struct sosHdlType*)(hp_sos);
    if (cast_handle == NULL)
        return(NOT_OKAY);

    total_samples = i_num_sample_sets * i_num_channels;

    for (i = 0; i < total_samples; i++)
    {
        rp_out_buf[i] = rp_in_buf[i] * cast_handle->master_gain;
    }
    return(OKAY);
}

/*
 * FUNCTION: sosProcessBuffer()
 * DESCRIPTION:
 *   Processes the passed in buffer using the current sos handle settings.
 *   This function is for mono or stereo signals only.
 *   This version uses a DC bias component to avoid underflow problems.
 *   PTNOTE- this appears to have a serious problem with shelf functions, the processing
 *   method uses a form specific to the coeff symmetry that occurs with parametric filters.
 */
int PT_DECLSPEC sosProcessBuffer(PT_HANDLE* hp_sos, realtype* rp_in_buf, realtype* rp_out_buf, int i_num_sample_sets, int i_num_channels, realtype r_samp_freq)
{
    struct sosHdlType* cast_handle;

    struct sosSectionType* s;
    int i, j, k;

    realtype sum_squares = 0.0f;

    cast_handle = (struct sosHdlType*)(hp_sos);

    if (cast_handle == NULL)
        return(NOT_OKAY);

    /* Currently only handles mono or stereo cases */
    if ((i_num_channels != 1) && (i_num_channels != 2))
        return(NOT_OKAY);

    k = 0;
    for (j = 0; j < i_num_sample_sets; j++)
    {
        realtype in1, in2, out1, out2;
        int active_flag;

        if (i_num_channels == 1)
        {
#ifdef SOS_DO_DC_BLOCKING
            realtype in_tmp;
            // Do DC blocking filtering.
            in_tmp = rp_in_buf[k];
            in1 = in_tmp - cast_handle->in1_old + (realtype)SOS_DCBLOCK_ALPHA * cast_handle->outDC1_old;
            cast_handle->in1_old = in_tmp;
            cast_handle->outDC1_old = in1;
#else
            in1 = rp_in_buf[k];
#endif //SOS_DO_DC_BLOCKING

            /* Outputs are also set to handle the case where all sections are off */
            out1 = in1;
            for (i = 0; i < cast_handle->num_active_sections; i++)
            {
                active_flag = cast_handle->section_on_flag[i];

                if (active_flag)
                {
                    s = &((cast_handle->sections)[i]);

                    /* Processing derived from macro kerSosFiltDirectForm2TransParaExtState */
                    out1 = s->state1 + s->b0 * in1 + (realtype)SOS_FLOAT_BIAS;
                    s->state1 = (in1 - out1) * s->b1 + s->state2;
                    s->state2 = s->b2 * in1 - s->a2 * out1;

                    in1 = out1;
                }
            }
            rp_out_buf[k] = out1 * cast_handle->master_gain;
        }
        else /* Stereo case */
        {
#ifdef SOS_DO_DC_BLOCKING
            // Do DC blocking filtering.
            realtype in_tmp;
            in_tmp = rp_in_buf[k];
            in1 = in_tmp - cast_handle->in1_old + (realtype)SOS_DCBLOCK_ALPHA * cast_handle->outDC1_old;
            cast_handle->in1_old = in_tmp;
            cast_handle->outDC1_old = in1;

            in_tmp = rp_in_buf[k + 1];
            in2 = in_tmp - cast_handle->in2_old + (realtype)SOS_DCBLOCK_ALPHA * cast_handle->outDC2_old;
            cast_handle->in2_old = in_tmp;
            cast_handle->outDC2_old = in2;
#else
            in1 = rp_in_buf[k];
            in2 = rp_in_buf[k + 1];
#endif //SOS_DO_DC_BLOCKING

            /* Outputs are also set to handle the case where all sections are off */
            out1 = in1;
            out2 = in2;

            for (i = 0; i < cast_handle->num_active_sections; i++)
            {
                active_flag = cast_handle->section_on_flag[i];

                if (active_flag)
                {
                    s = &((cast_handle->sections)[i]);

                    /* Processing derived from macro kerSosFiltDirectForm2TransParaExtState */
                    out1 = s->state1 + s->b0 * in1 + (realtype)SOS_FLOAT_BIAS;
                    s->state1 = (in1 - out1) * s->b1 + s->state2;
                    s->state2 = s->b2 * in1 - s->a2 * out1;

                    in1 = out1;

                    out2 = s->state3 + s->b0 * in2 + (realtype)SOS_FLOAT_BIAS;
                    s->state3 = (in2 - out2) * s->b1 + s->state4;
                    s->state4 = s->b2 * in2 - s->a2 * out2;

                    in2 = out2;
                }
            }
            rp_out_buf[k] = out1 * cast_handle->master_gain * cast_handle->balance_left;
            rp_out_buf[k + 1] = out2 * cast_handle->master_gain * cast_handle->balance_right;

            if (cast_handle->target_rms != 1.0f)
            {
                sum_squares += (rp_out_buf[k] * rp_out_buf[k]) + (rp_out_buf[k + 1] * rp_out_buf[k + 1]);
            }
        }

        k += i_num_channels;
    }


    //// --------------------------------------------------------------- NORMALIZATION ORIGINAL
    //if (cast_handle->target_rms != 1.0f && i_num_channels == 2)
    //{
    //	realtype current_rms = sqrtf(sum_squares / (i_num_sample_sets * 2));
    // 
    //	realtype rms_gain = std::fmin(cast_handle->target_rms / current_rms, 1.0f);
    // 
    //  cast_handle->normalization_gain = 1.0f;
    //	
    //	if (rms_gain > 0.0f)
    //	{
    //		if (cast_handle->normalization_gain == 1.0f)
    //		{
    //			cast_handle->normalization_gain = rms_gain;
    //		}
    //		else
    //		{
    //			if (fabs(cast_handle->normalization_gain - rms_gain) <= 0.01f )
    //			{
    //				cast_handle->normalization_gain = rms_gain;
    //			}
    //		}
    //	}

    //	k = 0;
    //	for (j = 0; j < i_num_sample_sets; j++)
    //	{
    //		rp_out_buf[k] *= cast_handle->normalization_gain;
    //		rp_out_buf[k + 1] *= cast_handle->normalization_gain;
    //		k += i_num_channels;
    //	}
    //}


    // --------------------------------------------------------------- NORMALIZATION NEW  (See also ---> GraphicEqSetNormalization)
    if (cast_handle->target_rms != 1.0f && i_num_channels == 2)
    {
        realtype current_rms = sqrtf(sum_squares / (i_num_sample_sets * 2));

        // ------------------------------------------------------ Must not be zero
        if (current_rms < 1e-6f) current_rms = 1e-6f;

        realtype target_gain = cast_handle->target_rms / current_rms;

        // ------------------------------------------------------ Gain security limits
        const realtype max_gain = 1.0f;  // Max +0dB
        const realtype min_gain = 0.01f; // Min -40dB

        target_gain = std::fmax(min_gain, std::fmin(target_gain, max_gain));

        // ------------------------------------------------------ Gain difference
        realtype gain_diff = fabs(target_gain - cast_handle->normalization_gain);

        // -------------------------------------------------- Adaptive smoothing factor
        realtype smoothing_factor;

        if (target_gain > cast_handle->normalization_gain)
        {
            smoothing_factor = 0.0005f + (gain_diff * 0.001f); // Slow attack
        }
        else
        {
            //smoothing_factor = 0.005f + (gain_diff * 0.01f);   // Fast release
            smoothing_factor = 1;  // Fast release
        }

        // -------------------------------------------------- Limit the smoothing factor
        smoothing_factor = std::fmin(smoothing_factor, 0.5f);

        // -------------------------------------------------- Apply the smoothing
        cast_handle->normalization_gain += (target_gain - cast_handle->normalization_gain) * smoothing_factor;

        // ------------------------------------------------------ Apply gain to samples
        k = 0;
        for (j = 0; j < i_num_sample_sets; j++)
        {
            rp_out_buf[k] *= cast_handle->normalization_gain;
            rp_out_buf[k + 1] *= cast_handle->normalization_gain;
            k += i_num_channels;
        }
    }

    applyVolumeLeveling(cast_handle, rp_out_buf, i_num_sample_sets, i_num_channels, r_samp_freq, -1);

    return(OKAY);
}

/*
 * FUNCTION: sosProcessBufferNoBias()  -  UNUSED FUNCTION
 * DESCRIPTION:
 *   Processes the passed in buffer using the current sos handle settings.
 *   This function is for mono or stereo signals only.
 *   This is a special version that does not include any DC bias component for accuracy critical usage.
 */
 //int PT_DECLSPEC sosProcessBufferNoBias(PT_HANDLE *hp_sos, realtype *rp_in_buf, realtype *rp_out_buf, int i_num_sample_sets, int i_num_channels)
 //{
 //	struct sosHdlType *cast_handle;
 //
 //	struct sosSectionType *s;
 //	int i, j, k;
 //
 //	realtype sum_squares = 0.0f;
 //    
 //	cast_handle = (struct sosHdlType *)(hp_sos);  
 //	
 //	if (cast_handle == NULL)
 //       return(NOT_OKAY);
 //
 //	/* Currently only handles mono or stereo cases */
 //	if( (i_num_channels != 1) && (i_num_channels != 2) )
 //		return(NOT_OKAY);
 //
 //	k = 0;
 //	for(j=0; j<i_num_sample_sets; j++)
 //	{
 //		realtype in1, in2, out1, out2;
 //		int active_flag;
 //
 //		if( i_num_channels == 1)
 //		{
 //			/* Outputs are also set to handle the case where all sections are off */
 //			in1 = rp_in_buf[k];
 //			out1 = in1;
 //			for(i=0; i<cast_handle->num_active_sections; i++)
 //			{
 //				active_flag = cast_handle->section_on_flag[i];
 //
 //				if( active_flag )
 //				{
 //					s = &((cast_handle->sections)[i]);
 //
 //					// For now to avoid parametric assumption problems with shelfs, straight ahead processing is used
 //					out1 = s->b0 * in1 + s->b1 * s->state_1[0] + s->b2 * s->state_1[1]
 //								- s->a1 * s->state_1[2] - s->a2 * s->state_1[3];
 //
 //					s->state_1[1] = s->state_1[0];
 //					s->state_1[0] = in1;
 //					s->state_1[3] = s->state_1[2];
 //					s->state_1[2] = out1;
 //
 //					in1 = out1; // Pass output on to next filter
 //				}
 //			}
 //			rp_out_buf[k] = out1 * cast_handle->master_gain;
 //		}
 //		else /* Stereo case */
 //		{
 //			in1 = rp_in_buf[k];
 //			out1 = in1;
 //			in2 = rp_in_buf[k+1];
 //			out2 = in2;
 //			for(i=0; i<cast_handle->num_active_sections; i++)
 //			{
 //				active_flag = cast_handle->section_on_flag[i];
 //
 //				if( active_flag )
 //				{
 //					s = &((cast_handle->sections)[i]);
 //
 //					// For now to avoid parametric assumption problems with shelfs, straight ahead processing is used
 //					out1 = s->b0 * in1 + s->b1 * s->state_1[0] + s->b2 * s->state_1[1]
 //								- s->a1 * s->state_1[2] - s->a2 * s->state_1[3];
 //
 //					s->state_1[1] = s->state_1[0];
 //					s->state_1[0] = in1;
 //					s->state_1[3] = s->state_1[2];
 //					s->state_1[2] = out1;
 //
 //					in1 = out1; // Pass output on to next filter
 //
 //					// For now to avoid parametric assumption problems with shelfs, straight ahead processing is used
 //					out2 = s->b0 * in2 + s->b1 * s->state_2[0] + s->b2 * s->state_2[1]
 //								- s->a1 * s->state_2[2] - s->a2 * s->state_2[3];
 //
 //					s->state_2[1] = s->state_2[0];
 //					s->state_2[0] = in2;
 //					s->state_2[3] = s->state_2[2];
 //					s->state_2[2] = out2;
 //
 //					in2 = out2; // Pass output on to next filter
 //				}
 //			}
 //			rp_out_buf[k] = out1 * cast_handle->master_gain * cast_handle->balance_left;
 //			rp_out_buf[k+1] = out2 * cast_handle->master_gain * cast_handle->balance_right;
 //
 //			if (cast_handle->target_rms != 0.0f)
 //			{
 //				sum_squares += (rp_out_buf[k] * rp_out_buf[k]) + (rp_out_buf[k + 1] * rp_out_buf[k + 1]);
 //			}
 //		}
 //
 //		k += i_num_channels;
 //	}
 //	
 //	return(OKAY);
 //}

 /*
  * FUNCTION: sosProcessSurroundBuffer()
  * DESCRIPTION:
  *   Processes the passed in buffer using the current sos handle settings.
  *   This function is for Surround Sound signals. Only puts bass boost on bass channel, no other eq on bass channel.
  */
int PT_DECLSPEC sosProcessSurroundBuffer(PT_HANDLE* hp_sos, realtype* rp_in_buf, realtype* rp_out_buf, int i_num_sample_sets, int i_num_channels, realtype r_samp_freq)
{
    struct sosHdlType* cast_handle;

    struct sosSectionType* s;
    int i, j, k;

    cast_handle = (struct sosHdlType*)(hp_sos);

    if (cast_handle == NULL)
        return(NOT_OKAY);

    /* Currently only handles 6 or 8 channel surround */
    if ((i_num_channels != 6) && (i_num_channels != 8))
        return(NOT_OKAY);

    //Ordering for 5.1 is: Front Left, Front Right, Front Center, Low Frequency, Back Left, Back Right
    //Ordering for 7.1 is: Front Left, Front Right, Front Center, Low Frequency, Back Left, Back Right, Side Left, Side Right
    //Note - LFE channel only gets bands 0,1 others only get bands 2->max
    for (j = 0; j < (i_num_sample_sets * i_num_channels); j += i_num_channels)
    {
        for (k = 0; k < i_num_channels; k++)
        {
            realtype in, out;
            int start, end;
            int active_flag;

            // LFE channel only gets bottom two bands, others only get upper bands
            if (k == 3)
            {
                start = 0;
                end = 2;
            }
            else
            {
                start = 2;
                end = cast_handle->num_active_sections;
            }

            /* Outputs are also set to handle the case where all sections are off */
            in = rp_in_buf[j + k];
            out = in;
            for (i = start; i < end; i++)
            {
                active_flag = cast_handle->section_on_flag[i];

                if (active_flag)
                {
                    s = &((cast_handle->sections)[i]);

                    /* Processing derived from macro kerSosFiltDirectForm2TransParaExtState */
                    out = s->state_1[k] + s->b0 * in + (realtype)SOS_FLOAT_BIAS;
                    s->state_1[k] = (in - out) * s->b1 + s->state_2[k];
                    s->state_2[k] = s->b2 * in - s->a2 * out;

                    in = out;
                }
            }
            rp_out_buf[j + k] = out * cast_handle->master_gain;
        }
    }

    applyVolumeLeveling(cast_handle, rp_out_buf, i_num_sample_sets, i_num_channels, r_samp_freq, 3);

    return(OKAY);
}

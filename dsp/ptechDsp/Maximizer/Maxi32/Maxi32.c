/*
FxSound
Copyright (C) 2023  FxSound LLC

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * FILE: MaxiX.c
 * DATE: 2025-03-25 (Updated)
 * AUTHOR: Paul F. Titchener (Original)
 * DESCRIPTION: Auto Maximizer with dynamic gain control and envelope-based peak limiting
 *
 * This is an audio maximizer that intelligently increases perceived loudness while
 * preventing digital clipping. It works by:
 * 1. Boosting overall signal gain
 * 2. Predicting potential peaks that would clip
 * 3. Dynamically adjusting gain to maintain a target output level
 * 4. Applying dithering and quantization for improved audio quality
 */

#ifndef DSPSOFT_TARGET
#define DSP_TARGET  /* Target is DSP hardware if not DSPSOFT */
#endif
#include "Maxi_num.h"  /* Sets bit precision */

/* Standard includes */
#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include "codedefs.h" 
#include "platform.h"    /* Platform-specific configurations */
#include "pt_defs.h"     /* Common definitions */
#include "product_type.h" /* Defines if we're building DSPFX or DFX */
#include "boardrv1.h"    /* Hardware-specific defines */

#ifdef DSP_TARGET
#include "dma32.h"       /* DMA handling for DSP target */
#endif

#include "dsp_mem1.h"    /* Memory configuration */
#include "c_dsps.h"      /* DSP software interface */
#include "c_max.h"       /* Maximizer parameter mappings */
#include "dutio.h"       /* I/O macros */
#include "dutcom.h"      /* Communication routines */
#include "dutinit.h"     /* Initialization routines */
#include "kerdma.h"      /* DMA handling macros */
#include "kerdelay.h"    /* Delay and wet/dry mixing */
#include "kernoise.h"    /* Noise and dithering functions */

/* Function prototypes */
static void DSPS_MAXI_RUN(void);
static void processEnvelope(struct dspMaxiStructType *s, float dly_out, float new_abs, 
                           float *env, float *max_abs, float *delta, int *ramp_count);
static float calculateGainBoost(struct dspMaxiStructType *s, float sqrt_level);
static void applyDitherAndQuantize(float *out1, float *out2, struct dspMaxiStructType *s);

#ifdef DSP_TARGET
void main(int argc, char *argv[])
{
    DSPS_MAXI_INIT();  /* Initialize the maximizer */
    while(1)           /* Continuous processing loop */
        DSPS_MAXI_RUN();
}
#endif

/* Only build init function in 32 bit files (same for both) */
#if defined(DSPSOFT_32_BIT)
DSP_FUNC_DEF int DSPS_MAXI_INIT(float *fp_params, float *fp_memory, long l_memsize, float *fp_state, int i_init_flag, float r_samp_freq)
{
    float *COMM_MEM_OFFSET = fp_params;    /* Communication parameter memory */
    float *MEMBANK0_START = fp_memory;     /* Main memory bank */
    long i;
    struct dspMaxiStructType *s = (struct dspMaxiStructType *)(COMM_MEM_OFFSET);
 
    if (i_init_flag & DSPS_INIT_PARAMS)
    {
        /* Initialize default parameter settings */
        s->wet_gain = 0.0f;                /* Initial processed signal level */
        s->dry_gain = 0.0f;                /* Initial original signal level */
        s->gain_boost = 1.99526f;          /* Initial amount of gain boost (approx. 6dB) */
        s->max_output = 0.966051f;         /* Maximum output level (just below 0dB) */
        s->max_delay = 33;                 /* Lookahead delay in samples for peak prediction */
        s->release_time_beta = 0.997776f;  /* Controls how quickly limiter releases (higher = slower) */
        s->num_quant_bits = KERNOISE_QUANTIZE_16;  /* 16-bit output quantization */
        s->dither_type = KERNOISE_DITHER_SHAPED;   /* Use noise-shaped dithering */

        /* Initialize state variables */
        s->ramp_count_l = s->ramp_count_r = 0;     /* No active envelope ramping */
        s->max_abs_l = s->max_abs_r = 0.0f;        /* No peaks detected yet */
        s->env_l = s->env_r = 0.0f;                /* Initialize envelope trackers */
        s->noise1_old = s->noise2_old = 0.0f;      /* Initialize dither memory */

        /* Auto mode parameters */
        s->target_level = MAXIMIZE_TARGET_LEVEL_SETTING;  /* Target output level for auto mode */
        s->level = 0.0f;                           /* Initial signal level estimate */

        /* Design the single pole level estimation lowpass filter */
        {
            /* This filter extracts the average energy level over time for auto-adjustment */
            double r_omega = 6.283185 * MAXIMIZE_LEVEL_FILT_CUTOFF / r_samp_freq;  /* Angular frequency */
            double cos_om = cos(r_omega);
            double root_calc = sqrt(cos_om * cos_om - 4.0 * cos_om + 3.0);
            double d_tmp = (2.0 - cos_om - root_calc);
            
            s->a0 = (realtype)d_tmp;                /* Feedback coefficient (close to 1.0) */
            s->filt_gain = (realtype)(1.0 - d_tmp); /* Input gain coefficient */
        }
    }

    if (i_init_flag & DSPS_INIT_MEMORY)
    {
        /* Initialize memory for DSP operation */
        dutilGetMemSizes();  /* Get memory allocation sizes */
        dutilInitMem();      /* Initialize memory segments */

        /* Initialize memory pointers for delay lines */
        s->ptr_l = s->dly_start_l;  /* Left channel delay line pointer */
        s->ptr_r = s->dly_start_r;  /* Right channel delay line pointer */
    }

    /* Zero internal signal memory spaces to eliminate glitches on restarts */
    if ((i_init_flag & DSPS_INIT_MEMORY) || (i_init_flag & DSPS_ZERO_MEMORY))
    {
        for(i = 0; i < MAXI_MAX_DELAY_LEN; i++)
            s->dly_start_l[i] = s->dly_start_r[i] = 0.0f;  /* Clear delay buffers */
    }

    /* Initialize A/D and D/A converters, and set Samp Rate */
    dutilInitAIO();        /* Initialize audio I/O */
    kerdmainit_MACRO();    /* Initialize DMA system */

    return OKAY;
}
#endif /* DSPSOFT_32_BIT */

#ifdef DSPSOFT_TARGET
DSP_FUNC_DEF void DSPS_MAXI_PROCESS(long *lp_data, int l_length,
                                   float *fp_params, float *fp_memory, float *fp_state,
                                   struct hardwareMeterValType *sp_meters,
                                   int DSP_data_type)
{
    float *COMM_MEM_OFFSET = fp_params;    /* Communication parameter memory */
    float *MEMBANK0_START = fp_memory;     /* Main memory bank */
    
    long transfer_state = 0;  /* For sending out meter values */
    long status = 0;          /* For sending run time status to PC */

    /* DMA related variables */
    unsigned data_index = 0;  /* Current position in buffer */
    float in_meter1_dma = 0.0f;   /* Input level meter for left channel */
    float in_meter2_dma = 0.0f;   /* Input level meter for right channel */
    float out_meter1_dma = 0.0f;  /* Output level meter for left channel */
    float out_meter2_dma = 0.0f;  /* Output level meter for right channel */
    long *read_in_buf = lp_data;   /* Input buffer pointer */
    long *read_out_buf = lp_data;  /* Output buffer pointer (same as input for in-place processing) */

    int i;
    struct dspMaxiStructType *s = (struct dspMaxiStructType *)(COMM_MEM_OFFSET);

    /* For peak data visualization */
    float peak_in1 = 1.0e-20f;  /* Peak input level tracker (left) - very small non-zero value */
    float peak_in2 = 1.0e-20f;  /* Peak input level tracker (right) */
    float peak_out1 = 1.0e-20f; /* Peak output level tracker (left) */
    float peak_out2 = 1.0e-20f; /* Peak output level tracker (right) */

    /* Process a buffer of audio data, sample by sample */
    for(i = 0; i < l_length; i++)
    {
        float out1, out2;           /* Output samples */
        float in1, in2;             /* Input samples */
        volatile long in_count = 0;  /* DMA input counter */
        volatile long out_count = 0; /* DMA output counter */
        float dly_l_out, dly_r_out; /* Delayed outputs from buffer */
        float new_abs_l, new_abs_r; /* Absolute value of new samples */
        float in_sqr;               /* Squared input value for level detection */
        float sqrt_level;           /* Square root of level (RMS-like value) */
        float gain_boost;           /* Calculated dynamic gain boost */

        load_parameter(); /* Load parameter if sent from host */

        /* Next line includes kerdmaru.h run loop macro (DMA handling) */
        #include "kerdmaru.h"

        out1 = out2 = 0.0f;  /* Initialize output samples */
         
        dutilGetInputsAndMeter(in1, in2, status);  /* Get input samples and update meters */

        /* Calculate level estimate using left channel input */
        in_sqr = in1 * in1;  /* Square the input sample (energy) */
        
        /* Update the lowpass filter for average level tracking */
        s->level = s->level * s->a0 + in_sqr * s->filt_gain;  /* Single-pole IIR filter */
        
        sqrt_level = sqrtf(s->level);  /* Convert energy to amplitude domain */
        
        /* Calculate dynamic gain boost based on signal level */
        gain_boost = calculateGainBoost(s, sqrt_level);  /* Apply auto-limiting if needed */
        
        /* --- Process left channel --- */
        dly_l_out = *(s->ptr_l);  /* Read current sample from delay buffer */
        
        /* Store boosted input sample in delay buffer */
        *(s->ptr_l) = gain_boost * s->max_output * in1;  /* Apply gain boost and scaling */
        
        new_abs_l = fabsf(*(s->ptr_l));  /* Get absolute value for peak detection */
        
        /* Advance delay pointer with circular buffer wrapping */
        (s->ptr_l)++;
        if (s->ptr_l >= (s->dly_start_l + s->max_delay))
            s->ptr_l = s->dly_start_l;  /* Wrap back to start of buffer */

        /* Update envelope tracking for left channel */
        processEnvelope(s, dly_l_out, new_abs_l, &s->env_l, &s->max_abs_l, 
                       &s->delta_l, &s->ramp_count_l);

        /* Apply envelope-based limiting to left channel output */
        if (s->env_l > s->max_output) {
            /* Normalize peak by applying gain reduction to prevent clipping */
            out1 = dly_l_out * s->max_output / s->env_l;  /* Proportional gain reduction */
            
            /* Track maximum peak values for visualization */
            if (s->env_l > peak_in1) {
                peak_in1 = s->env_l;           /* Store input peak level */
                peak_out1 = s->max_output;     /* Store limited output level */
            }
        } else {
            /* No limiting needed, output is below maximum */
            out1 = dly_l_out;
            
            /* Still track peaks for visualization */
            if (s->env_l > peak_in1) {
                peak_in1 = s->env_l;    /* Input peak */
                peak_out1 = s->env_l;   /* Output peak (same as input when no limiting) */
            }
        }

        /* --- Process right channel if stereo input enabled --- */
        if (s->stereo_in_flag) {
            dly_r_out = *(s->ptr_r);  /* Read current sample from delay buffer */
            
            /* Store boosted input sample in delay buffer */
            *(s->ptr_r) = gain_boost * s->max_output * in2;
            
            new_abs_r = fabsf(*(s->ptr_r));  /* Get absolute value for peak detection */
            
            /* Advance delay pointer with circular buffer wrapping */
            (s->ptr_r)++;
            if (s->ptr_r >= (s->dly_start_r + s->max_delay))
                s->ptr_r = s->dly_start_r;

            /* Update envelope tracking for right channel */
            processEnvelope(s, dly_r_out, new_abs_r, &s->env_r, &s->max_abs_r, 
                           &s->delta_r, &s->ramp_count_r);

            /* Apply envelope-based limiting to right channel output */
            if (s->env_r > s->max_output) {
                /* Apply gain reduction to prevent clipping */
                out2 = dly_r_out * s->max_output / s->env_r;
                
                if (s->env_r > peak_in2) {
                    peak_in2 = s->env_r;
                    peak_out2 = s->max_output;
                }
            } else {
                /* No limiting needed */
                out2 = dly_r_out;
                
                if (s->env_r > peak_in2) {
                    peak_in2 = s->env_r;
                    peak_out2 = s->env_r;
                }
            }
        } else {
            /* Mono mode - set right channel to silence */
            in2 = 0.0f;
            out2 = 0.0f;
        }

        /* Apply dithering and quantization if enabled */
        if (s->quantize_on_flag) {
            applyDitherAndQuantize(&out1, &out2, s);  /* Add dither and quantize to target bit depth */
        }

        /* Apply wet/dry mix (balance between processed and original signal) */
        kerWetDry(in1, in2, &(s->wet_gain), &(s->dry_gain), out1, out2);

        /* Check for clipping and update meters */
        dutilSetClipStatus(in1, in2, out1, out2, status);  /* Set clip status flags if needed */
        dutilPutOutputsAndMeter(out1, out2, status);       /* Send output samples and update meters */
        
        /* Update level meters and status information */
        write_meters_and_status(in_meter1, in_meter2, out_meter1, out_meter2, status, transfer_state);
    }

    /* Write averaged meter data after processing the entire buffer */
    write_meter_average();

#if defined(DSPSOFT_TARGET) & (PT_DSP_BUILD == PT_DSP_DSPFX)
    /* Write extra graphic data for visualizations (in decibels) */
    
    /* Calculate input peak levels in dB, removing the gain boost to show true input */
    sp_meters->aux_vals[0] = 20.0f * log10f(peak_in1 / (s->gain_boost * s->max_output));
    if (sp_meters->aux_vals[0] > 0.0f)
        sp_meters->aux_vals[0] = 0.0f;  /* Cap at 0dB maximum */

    sp_meters->aux_vals[2] = 20.0f * log10f(peak_in2 / (s->gain_boost * s->max_output));
    if (sp_meters->aux_vals[2] > 0.0f)
        sp_meters->aux_vals[2] = 0.0f;  /* Cap at 0dB maximum */

    /* Calculate output peak levels in dB */
    sp_meters->aux_vals[1] = 20.0f * log10f(peak_out1);  /* Left output peak in dB */
    sp_meters->aux_vals[3] = 20.0f * log10f(peak_out2);  /* Right output peak in dB */
#endif
}

/**
 * Calculate dynamic gain boost based on signal level and target
 * 
 * This function determines how much gain to apply based on the current
 * signal level. If applying the full gain boost would exceed the target level,
 * it reduces the gain to prevent excessive limiting.
 * 
 * @param s Pointer to dspMaxiStructType
 * @param sqrt_level Square root of the current signal level (RMS-like value)
 * @return The appropriate gain boost value
 */
static float calculateGainBoost(struct dspMaxiStructType *s, float sqrt_level)
{
    float result = s->gain_boost * sqrt_level;  /* Projected output level with full boost */
    float gain_boost;

    if (result > s->target_level) {
        /* Reduce gain to hit target level instead of full boost */
        gain_boost = s->target_level / sqrt_level;  /* Calculate needed gain to reach target */
        
        /* Prevent extreme gain reduction which can cause volume pumping */
        if (gain_boost < 1.06f)
            gain_boost = 1.06f;  /* Minimum 0.5dB of gain to prevent pumping */
    } else {
        /* Signal is low enough that we can apply full gain boost */
        gain_boost = s->gain_boost;
    }
    
    return gain_boost;
}

/**
 * Process the envelope tracking and update for a single channel
 * 
 * This function implements a peak prediction algorithm that:
 * 1. Tracks the envelope (peak level) of the signal
 * 2. Uses lookahead to anticipate upcoming peaks
 * 3. Smoothly ramps the envelope up to prevent sudden gain changes
 * 4. Gradually releases the envelope when signal level decreases
 * 
 * @param s Pointer to dspMaxiStructType
 * @param dly_out Current delayed output sample
 * @param new_abs Absolute value of new input sample
 * @param env Pointer to envelope value
 * @param max_abs Pointer to maximum absolute value
 * @param delta Pointer to delta value for envelope ramping
 * @param ramp_count Pointer to ramp counter
 */
static void processEnvelope(struct dspMaxiStructType *s, float dly_out, float new_abs, 
                           float *env, float *max_abs, float *delta, int *ramp_count)
{
    /* If we're in ramping mode to meet a peak */
    if (*ramp_count) {
        float tmp_delta;
        float abs_out = fabsf(dly_out);  /* Current output sample magnitude */
        
        /* Update envelope with current output if needed (instantaneous peak tracking) */
        if (abs_out > *env)
            *env = abs_out;  /* Use current sample as envelope if higher */

        /* Check if new input is higher than the ramp target we're already moving toward */
        if (new_abs > *max_abs) {
            /* Reset for new higher target level */
            *max_abs = new_abs;  /* New peak target */
            *ramp_count = s->max_delay;  /* Reset ramp counter to full delay length */
            
            /* Calculate new delta for smooth ramping to reach peak exactly when it exits delay */
            tmp_delta = (new_abs - *env) / (float)(s->max_delay + 1);
            
            /* Only increase ramp speed, never decrease it */
            if (tmp_delta > *delta)
                *delta = tmp_delta;
        } else {
            /* Continue counting down current ramp */
            (*ramp_count)--;
        }

        /* Update envelope by adding the calculated delta (smooth ramp up) */
        *env += *delta;
    } else {
        /* Not in ramp mode - first decay the envelope with release time constant */
        *env = *env * s->release_time_beta + MAXI_ENVELOPE_BIAS;
            
        /* Update envelope with current output if needed */
        float abs_out = fabsf(dly_out);
        if (abs_out > *env)
            *env = abs_out;  /* Use current sample as envelope if higher */

        /* Check if we need to start ramp mode (new peak in input is higher than current envelope) */
        if (new_abs > *env) {
            *max_abs = new_abs;  /* Store new peak target */
            
            /* Calculate delta for smooth ramping to the new peak */
            *delta = (new_abs - *env) / (float)(s->max_delay + 1);
            
            /* Start ramping immediately */
            *env += *delta;
            *ramp_count = s->max_delay;
        }
    }
}

/**
 * Apply dithering and quantization to output signals
 * 
 * This function adds carefully controlled noise (dither) to the signal
 * before quantizing to the target bit depth. This process improves the 
 * perceived audio quality when reducing bit depth by:
 * 1. Eliminating quantization distortion
 * 2. Converting non-linear quantization errors to noise
 * 3. Shaping the noise to be less audible (for shaped dither)
 * 
 * @param out1 Pointer to left channel output
 * @param out2 Pointer to right channel output
 * @param s Pointer to dspMaxiStructType
 */
static void applyDitherAndQuantize(float *out1, float *out2, struct dspMaxiStructType *s)
{
    static unsigned long seed = MAXIMIZE_NOISE_SEED;  /* Seed for noise generation */
    float dither1 = 0.0f, dither2 = 0.0f;  /* Dither values to add before quantization */

    /* Generate appropriate dither values based on selected type */
    switch (s->dither_type) {
    case KERNOISE_DITHER_NONE:
        /* No dither, values remain 0 (not recommended for final output) */
        break;
        
    case KERNOISE_DITHER_UNIFORM:
        /* Uniform white noise with amplitude of +/- 0.5 quantization steps */
        kerUniformWhiteNoise(seed, 0.5f, dither1);
        kerUniformWhiteNoise(seed, 0.5f, dither2);
        break;
        
    case KERNOISE_DITHER_TRIANGULAR:
        /* Triangular distribution (sum of two uniform) with +/- 1.0 peak */
        /* This provides better linearization of quiet signals */
        kerTriangularWhiteNoise(seed, 1.0f, dither1);
        kerTriangularWhiteNoise(seed, 1.0f, dither2);
        break;
        
    case KERNOISE_DITHER_SHAPED:
        {
            /* High-pass shaped dither moves noise energy to higher frequencies */
            /* where human hearing is less sensitive (first-order noise shaping) */
            float noise_tmp;
            
            kerUniformWhiteNoise(seed, 0.325f, noise_tmp);  /* Generate noise */
            dither1 = noise_tmp - s->noise1_old;  /* High-pass filter (differentiate) */
            s->noise1_old = noise_tmp;  /* Store for next sample */

            kerUniformWhiteNoise(seed, 0.325f, noise_tmp);
            dither2 = noise_tmp - s->noise2_old;
            s->noise2_old = noise_tmp;
        }
        break;
    }

    /* Apply quantization with dither to the output signals */
    switch (s->num_quant_bits) {
    case KERNOISE_QUANTIZE_24:
        /* 24-bit quantization (high quality, minimal audible effect) */
        break;
        
    case KERNOISE_QUANTIZE_20:
        /* 20-bit quantization with dither */
        kerQuantizeAndDither(*out1, KERNOISE_PEAK_LEVEL_20, dither1);
        kerQuantizeAndDither(*out2, KERNOISE_PEAK_LEVEL_20, dither2);
        break;
        
    case KERNOISE_QUANTIZE_16:
        /* 16-bit quantization (CD quality) with dither */
        kerQuantizeAndDither(*out1, KERNOISE_PEAK_LEVEL_16, dither1);
        kerQuantizeAndDither(*out2, KERNOISE_PEAK_LEVEL_16, dither2);
        break;
        
    case KERNOISE_QUANTIZE_12:
        /* 12-bit quantization with dither */
        kerQuantizeAndDither(*out1, KERNOISE_PEAK_LEVEL_12, dither1);
        kerQuantizeAndDither(*out2, KERNOISE_PEAK_LEVEL_12, dither2);
        break;
        
    case KERNOISE_QUANTIZE_8:
        /* 8-bit quantization with dither (considerable loss of quality) */
        kerQuantizeAndDither(*out1, KERNOISE_PEAK_LEVEL_8, dither1);
        kerQuantizeAndDither(*out2, KERNOISE_PEAK_LEVEL_8, dither2);
        break;
    }
}
#endif /* DSPSOFT_TARGET */

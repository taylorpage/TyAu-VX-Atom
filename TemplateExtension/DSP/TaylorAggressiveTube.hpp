//
//  TaylorAggressiveTube.hpp
//  Extreme Tube Saturation DSP Module
//
//  The most aggressive tube saturation variant for heavy distortion
//  and creative tone shaping.
//

#pragma once

#include <cmath>
#include <algorithm>

/**
 TaylorAggressiveTube

 Extreme tube saturation processor with heavy distortion character.
 This is the "special effects" version - for aggressive tone shaping
 and creative processing.

 Features:
 - Extreme asymmetric soft-clipping
 - Heavy harmonic distortion
 - Adjustable drive and output gain
 - DC blocking filter to prevent offset buildup
 - Zero external dependencies

 Usage (Plug-and-play with aggressive defaults):
   TaylorAggressiveTube tube;
   tube.setSampleRate(44100.0);

   // In your process loop:
   float output = tube.processSample(input);

   // Defaults are tuned for extreme saturation and distortion.
 */
class TaylorAggressiveTube {
public:
    TaylorAggressiveTube() {
        reset();
    }

    // MARK: - Configuration

    /**
     Set the sample rate (call this during initialization)
     */
    void setSampleRate(double sampleRate) {
        mSampleRate = sampleRate;
        updateDCBlockerCoefficients();
    }

    /**
     Set drive amount (saturation intensity)
     @param drive 1.0 = unity/clean, 8.0-15.0 = extreme to insane saturation
     */
    void setDrive(float drive) {
        mDrive = std::max(0.1f, drive);
    }

    /**
     Get current drive setting
     */
    float getDrive() const {
        return mDrive;
    }

    /**
     Set output gain (typically used to compensate for drive boost)
     @param gain 0.0-1.0+ output level multiplier
     */
    void setOutputGain(float gain) {
        mOutputGain = std::max(0.0f, gain);
    }

    /**
     Get current output gain
     */
    float getOutputGain() const {
        return mOutputGain;
    }

    /**
     Enable/disable the tube saturation processing
     */
    void setEnabled(bool enabled) {
        mEnabled = enabled;
    }

    /**
     Check if processing is enabled
     */
    bool isEnabled() const {
        return mEnabled;
    }

    // MARK: - Processing

    /**
     Process a single sample through the tube saturation
     @param input Input sample value
     @return Processed output sample
     */
    float processSample(float input) {
        if (!mEnabled) {
            return input;
        }

        // Apply input drive
        float driven = input * mDrive;

        // Asymmetric tube-like saturation curve
        float saturated = tubeSaturationCurve(driven);

        // DC blocking filter (removes DC offset that saturation can introduce)
        float dcBlocked = processDCBlocker(saturated);

        // Apply output gain compensation
        return dcBlocked * mOutputGain;
    }

    /**
     Process a buffer of samples
     @param buffer Pointer to audio buffer (in-place processing)
     @param numSamples Number of samples to process
     */
    void processBuffer(float* buffer, int numSamples) {
        for (int i = 0; i < numSamples; ++i) {
            buffer[i] = processSample(buffer[i]);
        }
    }

    /**
     Reset internal state (call when audio stream stops/starts)
     */
    void reset() {
        mDCBlockerX1 = 0.0f;
        mDCBlockerY1 = 0.0f;
    }

private:
    // MARK: - Saturation Algorithms

    /**
     Extreme asymmetric tube saturation transfer function

     Very aggressive clipping for heavy distortion and creative effects.
     Produces dense harmonic content with strong tube-like compression.
     */
    float tubeSaturationCurve(float x) {
        // Very low thresholds = extreme early saturation
        const float positiveThreshold = 0.2f;   // Clip very early
        const float negativeThreshold = 0.35f;  // Clip very early

        if (x > positiveThreshold) {
            // Extreme soft clip on positive peaks
            float excess = x - positiveThreshold;
            return positiveThreshold + std::tanh(excess * 3.0f) * 0.5f;
        }
        else if (x < -negativeThreshold) {
            // Heavy clip on negative peaks (asymmetry maintained)
            float excess = x + negativeThreshold;
            return -negativeThreshold + std::tanh(excess * 2.5f) * 0.55f;
        }
        else {
            // Linear region with strong harmonic distortion
            return x + (x * x * x) * 0.3f;  // Heavy 3rd harmonic content
        }
    }

    // MARK: - DC Blocker

    /**
     Update DC blocker filter coefficients based on sample rate
     */
    void updateDCBlockerCoefficients() {
        // High-pass filter at ~5Hz to remove DC offset
        float cutoffFreq = 5.0f;
        float w0 = 2.0f * M_PI * cutoffFreq / mSampleRate;
        mDCBlockerCoeff = 1.0f - w0;
    }

    /**
     DC blocking filter (1st order high-pass)
     Removes DC offset that can accumulate from asymmetric saturation
     */
    float processDCBlocker(float input) {
        float output = input - mDCBlockerX1 + mDCBlockerCoeff * mDCBlockerY1;
        mDCBlockerX1 = input;
        mDCBlockerY1 = output;
        return output;
    }

    // MARK: - Member Variables

    // Configuration (defaults tuned for extreme saturation and distortion)
    double mSampleRate = 44100.0;
    float mDrive = 9.0f;         // Extreme saturation - heavy distortion
    float mOutputGain = 0.45f;   // Maximum compensation for massive drive boost
    bool mEnabled = true;

    // DC Blocker state
    float mDCBlockerX1 = 0.0f;
    float mDCBlockerY1 = 0.0f;
    float mDCBlockerCoeff = 0.99f;
};

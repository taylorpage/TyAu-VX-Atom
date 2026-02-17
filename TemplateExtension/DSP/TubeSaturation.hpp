//
//  TubeSaturation.hpp
//  Standalone Tube Saturation DSP Module
//
//  A self-contained, reusable tube-style saturation processor
//  that can be dropped into any audio plugin or application.
//

#pragma once

#include <cmath>
#include <algorithm>

/**
 TubeSaturation

 A lightweight, CPU-efficient tube saturation processor that adds
 warmth and harmonic richness to audio signals.

 Features:
 - Asymmetric soft-clipping for tube-like character
 - Even-order harmonic generation
 - Adjustable drive and output gain
 - DC blocking filter to prevent offset buildup
 - Zero external dependencies

 Usage (Plug-and-play with subtle defaults):
   TubeSaturation tube;
   tube.setSampleRate(44100.0);

   // In your process loop:
   float output = tube.processSample(input);

   // That's it! Defaults are tuned for transparent analog warmth.
   // Optional: Adjust parameters if you want more/less saturation.
 */
class TubeSaturation {
public:
    TubeSaturation() {
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
     @param drive 1.0 = unity/clean, 2.0-10.0 = mild to heavy saturation
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
     Asymmetric tube saturation transfer function

     This curve mimics the asymmetric clipping characteristics of a
     triode tube, producing pleasing even-order harmonics.
     */
    float tubeSaturationCurve(float x) {
        // Asymmetric parameters (tubes clip positive and negative differently)
        const float positiveThreshold = 0.7f;
        const float negativeThreshold = 0.9f;

        if (x > positiveThreshold) {
            // Soft clip positive peaks (more aggressive)
            float excess = x - positiveThreshold;
            return positiveThreshold + std::tanh(excess * 2.0f) * 0.3f;
        }
        else if (x < -negativeThreshold) {
            // Soft clip negative peaks (less aggressive - tube asymmetry)
            float excess = x + negativeThreshold;
            return -negativeThreshold + std::tanh(excess * 1.5f) * 0.35f;
        }
        else {
            // Linear region with subtle soft knee
            return x + (x * x * x) * 0.05f;  // Very subtle 3rd harmonic for warmth
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

    // Configuration (defaults tuned for subtle, transparent warmth)
    double mSampleRate = 44100.0;
    float mDrive = 1.5f;         // Subtle saturation - adds warmth without obvious distortion
    float mOutputGain = 0.92f;   // Slight compensation to maintain unity gain
    bool mEnabled = true;

    // DC Blocker state
    float mDCBlockerX1 = 0.0f;
    float mDCBlockerY1 = 0.0f;
    float mDCBlockerCoeff = 0.99f;
};

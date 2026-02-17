//
//  TaylorWarmTube.hpp
//  Heavy Tube Saturation DSP Module
//
//  A more aggressive version of TubeSaturation for obvious warmth and character.
//  Drop-in replacement with heavier default settings.
//

#pragma once

#include <cmath>
#include <algorithm>

/**
 TaylorWarmTube

 Heavy-duty tube saturation processor with strong harmonic character.
 This is the "you'll definitely hear it" version - great for mixing
 and adding obvious analog character.

 Features:
 - Aggressive asymmetric soft-clipping
 - Rich harmonic generation
 - Adjustable drive and output gain
 - DC blocking filter to prevent offset buildup
 - Zero external dependencies

 Usage (Plug-and-play with warm defaults):
   TaylorWarmTube tube;
   tube.setSampleRate(44100.0);

   // In your process loop:
   float output = tube.processSample(input);

   // Defaults are tuned for obvious warmth and saturation.
 */
class TaylorWarmTube {
public:
    TaylorWarmTube() {
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
     @param drive 1.0 = unity/clean, 4.0-10.0 = heavy to extreme saturation
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
     Heavy asymmetric tube saturation transfer function

     More aggressive clipping for obvious warmth and character.
     Produces rich even-order harmonics with strong tube coloration.
     */
    float tubeSaturationCurve(float x) {
        // Lower thresholds = earlier saturation = more harmonic content
        const float positiveThreshold = 0.4f;   // Clip earlier (was 0.7)
        const float negativeThreshold = 0.6f;   // Clip earlier (was 0.9)

        if (x > positiveThreshold) {
            // Aggressive soft clip on positive peaks
            float excess = x - positiveThreshold;
            return positiveThreshold + std::tanh(excess * 2.5f) * 0.4f;
        }
        else if (x < -negativeThreshold) {
            // Moderate clip on negative peaks (tube asymmetry preserved)
            float excess = x + negativeThreshold;
            return -negativeThreshold + std::tanh(excess * 2.0f) * 0.45f;
        }
        else {
            // Linear region with noticeable harmonic content
            return x + (x * x * x) * 0.15f;  // More pronounced 3rd harmonic
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

    // Configuration (defaults tuned for obvious warmth and saturation)
    double mSampleRate = 44100.0;
    float mDrive = 5.0f;         // Heavy saturation - obvious tube character
    float mOutputGain = 0.65f;   // Stronger compensation for drive boost
    bool mEnabled = true;

    // DC Blocker state
    float mDCBlockerX1 = 0.0f;
    float mDCBlockerY1 = 0.0f;
    float mDCBlockerCoeff = 0.99f;
};

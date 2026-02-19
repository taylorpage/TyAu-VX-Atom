//
//  VXAtomExtensionDSPKernel.hpp
//  VXAtomExtension
//
//  VX-ATOM Character Compressor DSP Engine
//  JJP-inspired hard/aggressive compression with harmonic saturation
//

#pragma once

#include <AudioToolbox/AudioToolbox.h>
#include <algorithm>
#include <cmath>
#include <span>
#include <array>

#include "VX-AtomExtensionParameterAddresses.h"

/*
 VXAtomExtensionDSPKernel
 As a non-ObjC class, this is safe to use from render thread.

 Signal chain (per sample, per channel):
   Input → Envelope Follower → Gain Computer → VCA → Saturation → Auto Makeup → Parallel Mix → Output Trim
*/
class VXAtomExtensionDSPKernel {
public:

    // MARK: - Lifecycle

    void initialize(int inputChannelCount, int outputChannelCount, double inSampleRate) {
        mSampleRate = inSampleRate;
        mChannelCount = std::min(inputChannelCount, kMaxChannels);
        for (int ch = 0; ch < kMaxChannels; ++ch) {
            mEnvelope[ch] = 0.0f;
        }
        updateCoefficients();
    }

    void deInitialize() {
        for (int ch = 0; ch < kMaxChannels; ++ch) {
            mEnvelope[ch] = 0.0f;
        }
    }

    // MARK: - Bypass

    bool isBypassed() {
        return mBypassed;
    }

    void setBypass(bool shouldBypass) {
        mBypassed = shouldBypass;
    }

    // MARK: - Parameter Getter / Setter

    void setParameter(AUParameterAddress address, AUValue value) {
        switch (address) {
            case VXAtomExtensionParameterAddress::squeeze:
                mSqueeze = std::max(0.0f, std::min(10.0f, value));
                break;
            case VXAtomExtensionParameterAddress::speed:
                mSpeed = std::max(0.0f, std::min(10.0f, value));
                updateCoefficients();
                break;
            case VXAtomExtensionParameterAddress::tone:
                mTone = std::max(0.0f, std::min(10.0f, value));
                break;
            case VXAtomExtensionParameterAddress::outputGain:
                mOutputGainDB = std::max(-12.0f, std::min(12.0f, value));
                mOutputGainLinear = dBToLinear(mOutputGainDB);
                break;
            case VXAtomExtensionParameterAddress::mix:
                mMix = std::max(0.0f, std::min(1.0f, value));
                break;
            case VXAtomExtensionParameterAddress::bypass:
                mBypassed = (value >= 0.5f);
                break;
        }
    }

    AUValue getParameter(AUParameterAddress address) {
        switch (address) {
            case VXAtomExtensionParameterAddress::squeeze:    return mSqueeze;
            case VXAtomExtensionParameterAddress::speed:      return mSpeed;
            case VXAtomExtensionParameterAddress::tone:       return mTone;
            case VXAtomExtensionParameterAddress::outputGain: return mOutputGainDB;
            case VXAtomExtensionParameterAddress::mix:        return mMix;
            case VXAtomExtensionParameterAddress::bypass:     return mBypassed ? 1.0f : 0.0f;
            default:                                          return 0.0f;
        }
    }

    // MARK: - Max Frames

    AUAudioFrameCount maximumFramesToRender() const {
        return mMaxFramesToRender;
    }

    void setMaximumFramesToRender(const AUAudioFrameCount &maxFrames) {
        mMaxFramesToRender = maxFrames;
    }

    // MARK: - Musical Context

    void setMusicalContextBlock(AUHostMusicalContextBlock contextBlock) {
        mMusicalContextBlock = contextBlock;
    }

    // MARK: - Gain Reduction Metering
    // Written on the render thread, read on the main/UI thread.
    // A float read/write is practically safe for a meter display (worst case: one stale frame).
    // Do NOT use std::atomic here — atomic operations are not real-time safe.
    float getGainReductionDB() const {
        return mGainReductionDB;
    }

    // MARK: - Internal Process

    void process(std::span<float const*> inputBuffers, std::span<float *> outputBuffers, AUEventSampleTime bufferStartTime, AUAudioFrameCount frameCount) {
        assert(inputBuffers.size() == outputBuffers.size());

        if (mBypassed) {
            for (UInt32 ch = 0; ch < inputBuffers.size(); ++ch) {
                std::copy_n(inputBuffers[ch], frameCount, outputBuffers[ch]);
            }
            mGainReductionDB = 0.0f;
            return;
        }

        // Pre-compute squeeze-dependent values once per buffer (not per sample)
        const float squeezeNorm   = mSqueeze / 10.0f;
        const float thresholdDB   = lerp(-6.0f, -30.0f, squeezeNorm);
        const float ratio         = lerp(2.0f, 12.0f, squeezeNorm);
        const float kneeDB        = lerp(8.0f, 0.0f, squeezeNorm);
        // Auto makeup: conservative estimate of gain lost at threshold
        const float autoMakeupDB  = -thresholdDB * (1.0f - 1.0f / ratio) * 0.5f;
        // Saturation drive from TONE (0=linear, 10=heavy harmonic coloring)
        const float drive         = 1.0f + (mTone / 10.0f) * 2.5f;

        float sumGainReductionDB = 0.0f;

        for (UInt32 ch = 0; ch < inputBuffers.size(); ++ch) {
            const int envIdx = (ch < kMaxChannels) ? ch : kMaxChannels - 1;

            for (UInt32 i = 0; i < frameCount; ++i) {
                const float inputSample = inputBuffers[ch][i];

                // --- Envelope Follower (peak detector, first-order IIR leaky integrator) ---
                const float rectified = std::fabs(inputSample);
                if (rectified > mEnvelope[envIdx]) {
                    mEnvelope[envIdx] += mAttackCoeff * (rectified - mEnvelope[envIdx]);
                } else {
                    mEnvelope[envIdx] += mReleaseCoeff * (rectified - mEnvelope[envIdx]);
                }
                // Clamp to prevent denormal floats on silence
                mEnvelope[envIdx] = std::max(mEnvelope[envIdx], 1e-10f);

                // --- Gain Computer (log domain with soft knee) ---
                const float levelDB = 20.0f * std::log10(mEnvelope[envIdx]);
                const float grDB    = computeGainReduction(levelDB, thresholdDB, ratio, kneeDB);

                // Total gain: GR + auto makeup + output trim
                const float totalGainDB   = grDB + autoMakeupDB + mOutputGainDB;
                const float gainLinear    = dBToLinear(totalGainDB);

                // --- Apply compression ---
                const float compressed = inputSample * gainLinear;

                // --- Saturation (soft tanh waveshaper for harmonic character) ---
                // Drive in, divide back out to approximate level preservation
                const float saturated = softTanh(compressed * drive) / drive;

                // --- Parallel Mix (dry = pre-compression signal, for transient preservation) ---
                const float output = lerp(inputSample, saturated, mMix) * mOutputGainLinear;

                outputBuffers[ch][i] = output;

                // Accumulate gain reduction for metering (first channel only)
                if (ch == 0) {
                    sumGainReductionDB += -grDB; // GR is negative dB; meter shows positive reduction
                }
            }
        }

        // Update meter state (average across buffer)
        if (frameCount > 0 && !inputBuffers.empty()) {
            mGainReductionDB = sumGainReductionDB / static_cast<float>(frameCount);
        }
    }

    // MARK: - Event Handling

    void handleOneEvent(AUEventSampleTime now, AURenderEvent const *event) {
        switch (event->head.eventType) {
            case AURenderEventParameter: {
                handleParameterEvent(now, event->parameter);
                break;
            }
            default:
                break;
        }
    }

    void handleParameterEvent(AUEventSampleTime now, AUParameterEvent const& parameterEvent) {
        setParameter(parameterEvent.parameterAddress, parameterEvent.value);
    }

private:

    // MARK: - DSP Helpers

    static float dBToLinear(float dB) {
        return std::pow(10.0f, dB / 20.0f);
    }

    static float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }

    // Soft tanh approximation: x*(27 + x²) / (27 + 9*x²)
    // Cheap, numerically stable, bounded to ≈ [-1, 1]
    static float softTanh(float x) {
        const float x2 = x * x;
        return x * (27.0f + x2) / (27.0f + 9.0f * x2);
    }

    // Piecewise gain computer in log domain.
    // Returns gain reduction in dB (negative = reduction, 0 = no reduction).
    float computeGainReduction(float levelDB, float thresholdDB, float ratio, float kneeDB) const {
        const float halfKnee     = kneeDB * 0.5f;
        const float overThreshold = levelDB - thresholdDB;

        if (overThreshold < -halfKnee) {
            // Below knee — no compression
            return 0.0f;
        } else if (kneeDB > 0.001f && overThreshold < halfKnee) {
            // In the soft knee — quadratic interpolation for smooth onset
            const float kneeInput = overThreshold + halfKnee;
            const float grSlope   = (1.0f / ratio - 1.0f);
            return grSlope * (kneeInput * kneeInput) / (2.0f * kneeDB);
        } else {
            // Above knee — full ratio compression
            return (1.0f / ratio - 1.0f) * overThreshold;
        }
    }

    // Recompute attack/release IIR coefficients from SPEED and sample rate.
    // SPEED 0 = slow optical warmth, SPEED 10 = fast FET aggression.
    void updateCoefficients() {
        const float speedNorm  = mSpeed / 10.0f;
        const double attackMs  = static_cast<double>(lerp(50.0f, 3.0f,   speedNorm));
        const double releaseMs = static_cast<double>(lerp(400.0f, 75.0f, speedNorm));
        mAttackCoeff  = computeIIRCoeff(attackMs  * 0.001, mSampleRate);
        mReleaseCoeff = computeIIRCoeff(releaseMs * 0.001, mSampleRate);
    }

    // First-order IIR pull coefficient from time constant in seconds.
    // Result is used as: envelope += coeff * (target - envelope)
    static float computeIIRCoeff(double timeSeconds, double sampleRate) {
        if (sampleRate <= 0.0 || timeSeconds <= 0.0) return 0.0f;
        return static_cast<float>(1.0 - std::exp(-1.0 / (timeSeconds * sampleRate)));
    }

    // MARK: - Member Variables

    static constexpr int kMaxChannels = 2;

    AUHostMusicalContextBlock mMusicalContextBlock;

    double mSampleRate    = 44100.0;
    int    mChannelCount  = 2;

    // Parameters (written from main thread via AU event system, read from render thread)
    float  mSqueeze       = 5.0f;
    float  mSpeed         = 3.0f;
    float  mTone          = 4.0f;
    float  mOutputGainDB  = 0.0f;
    float  mOutputGainLinear = 1.0f;
    float  mMix           = 1.0f;
    bool   mBypassed      = false;

    // Envelope follower state (per channel)
    float  mEnvelope[kMaxChannels] = { 0.0f, 0.0f };

    // Time-domain coefficients (derived from SPEED, recomputed on change)
    float  mAttackCoeff   = 0.0f;
    float  mReleaseCoeff  = 0.0f;

    // Gain reduction metering (written on render thread, read on UI thread — float read is tolerable)
    float  mGainReductionDB = 0.0f;

    AUAudioFrameCount mMaxFramesToRender = 1024;
};

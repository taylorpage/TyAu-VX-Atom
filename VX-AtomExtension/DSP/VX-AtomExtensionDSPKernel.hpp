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
   Input → Gate → Env1 → GC1 → VCA1 → Env2 → GC2 → VCA2 → Env3 → GC3 (Limiter) → VCA3 → Parallel Mix → Output Trim

   Gate:    Pre-compression noise gate. Threshold 0=-80dB (off) to 10=-30dB. Fixed 2ms open / 100ms close.
   Stage 1: Heavy VCA-style compressor. Threshold -12 to -60 dB, ratio 4:1 to 200:1, fast attack.
   Stage 2: Independent aggressive compressor. Threshold -10 to -25 dB, ratio 4:1 to 8:1, 2x slower attack.
   Stage 3: Ceiling limiter. Threshold -2 to -8 dB, ratio 80:1, very fast. No makeup — ceiling stays down.
   Three different personalities at three different timescales = "pressed against the wall" stacked sound.
*/
class VXAtomExtensionDSPKernel {
public:

    // MARK: - Lifecycle

    void initialize(int inputChannelCount, int outputChannelCount, double inSampleRate) {
        mSampleRate = inSampleRate;
        mChannelCount = std::min(inputChannelCount, kMaxChannels);
        for (int ch = 0; ch < kMaxChannels; ++ch) {
            mEnvelope[ch]  = 0.0f;
            mEnvelope2[ch] = 0.0f;
            mEnvelope3[ch] = 0.0f;
            mGateEnvelope[ch] = 0.0f;
            mGateGain[ch]     = 1.0f;
        }
        // Gate: fixed time constants (not parameter-dependent)
        mGateAttackCoeff  = computeIIRCoeff(0.002, mSampleRate);  // 2ms open
        mGateReleaseCoeff = computeIIRCoeff(0.100, mSampleRate);  // 100ms close
        updateCoefficients();
    }

    void deInitialize() {
        for (int ch = 0; ch < kMaxChannels; ++ch) {
            mEnvelope[ch]  = 0.0f;
            mEnvelope2[ch] = 0.0f;
            mEnvelope3[ch] = 0.0f;
            mGateEnvelope[ch] = 0.0f;
            mGateGain[ch]     = 1.0f;
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
            case VXAtomExtensionParameterAddress::compress:
                mCompress = std::max(0.0f, std::min(10.0f, value));
                break;
            case VXAtomExtensionParameterAddress::speed:
                mSpeed = std::max(0.0f, std::min(10.0f, value));
                updateCoefficients();
                break;
            case VXAtomExtensionParameterAddress::gate:
                mGate = std::max(0.0f, std::min(10.0f, value));
                break;
            case VXAtomExtensionParameterAddress::outputGain:
                mOutputGainDB = std::max(-24.0f, std::min(24.0f, value));
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
            case VXAtomExtensionParameterAddress::compress:    return mCompress;
            case VXAtomExtensionParameterAddress::speed:      return mSpeed;
            case VXAtomExtensionParameterAddress::gate:       return mGate;
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

    // MARK: - Gain Reduction Metering
    // Written on the render thread, read on the main/UI thread.
    // A float read/write is practically safe for a meter display (worst case: one stale frame).
    // Do NOT use std::atomic here — atomic operations are not real-time safe.
    //
    // When the host stops calling the render block (transport stopped), mRenderGeneration stops
    // advancing. Each UI poll that sees no advancement applies a release-style decay so the needle
    // returns to rest rather than freezing at the last reading.
    float getGainReductionDB() {
        const uint64_t gen = mRenderGeneration;
        if (gen == mLastUIGeneration && mMeterSmoothed > 0.0f) {
            // Render thread has been idle since our last read — decay toward zero.
            // ~0.87 per poll ≈ 200 ms time-constant at 30 fps.
            mMeterSmoothed *= 0.87f;
            if (mMeterSmoothed < 0.001f) mMeterSmoothed = 0.0f;
            mGainReductionDB = mMeterSmoothed;
        }
        mLastUIGeneration = gen;
        return mGainReductionDB;
    }

    // MARK: - Internal Process

    void process(std::span<float const*> inputBuffers, std::span<float *> outputBuffers, AUEventSampleTime bufferStartTime, AUAudioFrameCount frameCount) {
        assert(inputBuffers.size() == outputBuffers.size());
        ++mRenderGeneration;  // Signals the UI thread that the render block is still being called

        if (mBypassed) {
            for (UInt32 ch = 0; ch < inputBuffers.size(); ++ch) {
                std::copy_n(inputBuffers[ch], frameCount, outputBuffers[ch]);
            }
            mGainReductionDB = 0.0f;
            return;
        }

        // Pre-compute squeeze-dependent values once per buffer (not per sample)
        // Piecewise mapping: 0-8 hits hard from the start;
        // 8-10 extends into nuclear territory (200:1 / -60 dB threshold).
        const float compressNorm = mCompress / 10.0f;
        float thresholdDB, ratio, kneeDB;
        if (compressNorm <= 0.8f) {
            // Normal zone (SQUEEZE 0-8): aggressive from the start, solid at ~30% of knob
            thresholdDB = lerp(-12.0f, -45.0f, compressNorm);
            ratio       = lerp(4.0f,   25.0f,  compressNorm);
            kneeDB      = lerp(4.0f,    0.0f,  compressNorm);
        } else {
            // Nuclear zone (SQUEEZE 8-10): extreme compression / hard limiting territory
            // Breakpoint values at compressNorm=0.8: threshold=-38.4dB, ratio=20.8:1, knee=0.8dB
            const float t = (compressNorm - 0.8f) / 0.2f;
            thresholdDB = lerp(-38.4f, -60.0f,  t);
            ratio       = lerp(20.8f,  200.0f,  t);
            kneeDB      = lerp(0.8f,    0.0f,   t);
        }
        // Auto makeup: conservative estimate of gain lost at threshold
        const float autoMakeupDB  = -thresholdDB * (1.0f - 1.0f / ratio) * 0.5f;

        // Stage 2: independent aggressive compressor — genuinely different personality from Stage 1.
        // Own threshold range (much higher than Stage 1's nuclear range), own ratio (heavy but not extreme),
        // and a fixed narrow-ish knee. Combined with the different time constants above, this creates
        // real stacked-compressor interaction rather than math-doubling the same settings.
        const float threshold2DB = lerp(-10.0f, -25.0f, compressNorm);
        const float ratio2       = lerp(4.0f,    8.0f,  compressNorm);
        const float knee2        = 3.0f;
        const float autoMakeup2  = -threshold2DB * (1.0f - 1.0f / ratio2) * 0.5f;

        // Stage 3: ceiling limiter — "pressed against the wall" brick-wall character.
        // Threshold scales with SQUEEZE so it engages harder as you push.
        // No auto makeup: the ceiling clamps and stays down — that squash is the sound.
        const float threshold3DB = lerp(-2.0f, -8.0f, compressNorm);
        const float ratio3       = 80.0f;
        const float knee3        = 0.5f;

        // Gate threshold: GATE=0 → -80 dB (effectively off), GATE=10 → -30 dB
        const float gateThresholdDB = lerp(-80.0f, -30.0f, mGate / 10.0f);
        const float gateThreshold   = dBToLinear(gateThresholdDB);

        float sumGainReductionDB = 0.0f;

        for (UInt32 ch = 0; ch < inputBuffers.size(); ++ch) {
            const int envIdx = (ch < kMaxChannels) ? ch : kMaxChannels - 1;

            for (UInt32 i = 0; i < frameCount; ++i) {
                const float inputSample = inputBuffers[ch][i];

                // --- Noise Gate (pre-compression) ---
                // Envelope follower detects signal level; gain smoothly opens/closes.
                const float absIn = std::fabs(inputSample);
                if (absIn > mGateEnvelope[envIdx]) {
                    mGateEnvelope[envIdx] += mGateAttackCoeff * (absIn - mGateEnvelope[envIdx]);
                } else {
                    mGateEnvelope[envIdx] += mGateReleaseCoeff * (absIn - mGateEnvelope[envIdx]);
                }
                mGateEnvelope[envIdx] = std::max(mGateEnvelope[envIdx], 1e-10f);
                const float targetGateGain = (mGateEnvelope[envIdx] >= gateThreshold) ? 1.0f : 0.0f;
                if (targetGateGain > mGateGain[envIdx]) {
                    mGateGain[envIdx] += mGateAttackCoeff * (targetGateGain - mGateGain[envIdx]);
                } else {
                    mGateGain[envIdx] += mGateReleaseCoeff * (targetGateGain - mGateGain[envIdx]);
                }
                const float gatedSample = inputSample * mGateGain[envIdx];

                // --- Envelope Follower (peak detector, first-order IIR leaky integrator) ---
                const float rectified = std::fabs(gatedSample);
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

                // --- Apply compression (Stage 1) ---
                const float compressed = gatedSample * gainLinear;

                // --- Stage 2: second envelope follower on post-stage-1 signal ---
                // Stage 2's detector sees the already-compressed signal, so it reacts to stage 1's
                // artifacts (pumping, breathing) — this is what creates the stacked-compressor character.
                const float rectified2 = std::fabs(compressed);
                if (rectified2 > mEnvelope2[envIdx]) {
                    mEnvelope2[envIdx] += mAttackCoeff2 * (rectified2 - mEnvelope2[envIdx]);
                } else {
                    mEnvelope2[envIdx] += mReleaseCoeff2 * (rectified2 - mEnvelope2[envIdx]);
                }
                mEnvelope2[envIdx] = std::max(mEnvelope2[envIdx], 1e-10f);

                const float levelDB2    = 20.0f * std::log10(mEnvelope2[envIdx]);
                const float grDB2       = computeGainReduction(levelDB2, threshold2DB, ratio2, knee2);
                const float compressed2 = compressed * dBToLinear(grDB2 + autoMakeup2);

                // --- Stage 3: ceiling limiter on post-stage-2 signal ---
                // No makeup gain — the ceiling stays down, that's the "pressed against the wall" feel.
                const float rectified3 = std::fabs(compressed2);
                if (rectified3 > mEnvelope3[envIdx]) {
                    mEnvelope3[envIdx] += mAttackCoeff3 * (rectified3 - mEnvelope3[envIdx]);
                } else {
                    mEnvelope3[envIdx] += mReleaseCoeff3 * (rectified3 - mEnvelope3[envIdx]);
                }
                mEnvelope3[envIdx] = std::max(mEnvelope3[envIdx], 1e-10f);

                const float levelDB3    = 20.0f * std::log10(mEnvelope3[envIdx]);
                const float grDB3       = computeGainReduction(levelDB3, threshold3DB, ratio3, knee3);
                const float compressed3 = compressed2 * dBToLinear(grDB3);

                // --- Parallel Mix (dry = gated pre-compression signal, for transient preservation) ---
                const float output = lerp(gatedSample, compressed3, mMix) * mOutputGainLinear;

                outputBuffers[ch][i] = output;

                // Accumulate gain reduction for metering (first channel only, all three stages combined)
                if (ch == 0) {
                    sumGainReductionDB += -(grDB + grDB2 + grDB3); // GR is negative dB; meter shows positive reduction
                }
            }
        }

        // Update meter with VU-style ballistics.
        // Raw per-buffer average would peg instantly at high SQUEEZE settings.
        // Attack ~150ms (needle rises quickly) / release ~300ms (needle falls slowly, like real VU).
        if (frameCount > 0 && !inputBuffers.empty()) {
            const float instantaneous  = sumGainReductionDB / static_cast<float>(frameCount);
            const float bufferDuration = static_cast<float>(frameCount) / static_cast<float>(mSampleRate);
            const float attackCoeff    = 1.0f - std::exp(-bufferDuration / 0.150f);
            const float releaseCoeff   = 1.0f - std::exp(-bufferDuration / 0.300f);
            if (instantaneous > mMeterSmoothed) {
                mMeterSmoothed += attackCoeff  * (instantaneous - mMeterSmoothed);
            } else {
                mMeterSmoothed += releaseCoeff * (instantaneous - mMeterSmoothed);
            }
            mGainReductionDB = mMeterSmoothed;
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
    // SPEED 0 = slow optical warmth (50ms attack / 400ms release)
    // SPEED 10 = sub-millisecond FET aggression (0.5ms attack / 25ms release)
    void updateCoefficients() {
        const float speedNorm  = mSpeed / 10.0f;
        const double attackMs  = static_cast<double>(lerp(50.0f, 0.5f,   speedNorm));
        const double releaseMs = static_cast<double>(lerp(400.0f, 25.0f, speedNorm));
        mAttackCoeff  = computeIIRCoeff(attackMs  * 0.001, mSampleRate);
        mReleaseCoeff = computeIIRCoeff(releaseMs * 0.001, mSampleRate);
        // Stage 2: 2x slower attack and 1.5x longer release than Stage 1.
        // Different time constants are what create genuine stacked-compressor interaction —
        // Stage 2 reacts on a different timescale so the two stages don't simply double each other.
        mAttackCoeff2  = computeIIRCoeff(attackMs * 2.0 * 0.001, mSampleRate);
        mReleaseCoeff2 = computeIIRCoeff(releaseMs * 1.5 * 0.001, mSampleRate);
        // Stage 3: ceiling limiter — always fast, tight range regardless of SPEED.
        // Scales with SPEED only slightly (5ms → 0.5ms attack, 100ms → 20ms release)
        // so the ceiling stays responsive even at the slowest SPEED setting.
        mAttackCoeff3  = computeIIRCoeff(static_cast<double>(lerp(5.0f, 0.5f,   speedNorm)) * 0.001, mSampleRate);
        mReleaseCoeff3 = computeIIRCoeff(static_cast<double>(lerp(100.0f, 20.0f, speedNorm)) * 0.001, mSampleRate);
    }

    // First-order IIR pull coefficient from time constant in seconds.
    // Result is used as: envelope += coeff * (target - envelope)
    static float computeIIRCoeff(double timeSeconds, double sampleRate) {
        if (sampleRate <= 0.0 || timeSeconds <= 0.0) return 0.0f;
        return static_cast<float>(1.0 - std::exp(-1.0 / (timeSeconds * sampleRate)));
    }

    // MARK: - Member Variables

    static constexpr int kMaxChannels = 2;

    double mSampleRate    = 44100.0;
    int    mChannelCount  = 2;

    // Parameters (written from main thread via AU event system, read from render thread)
    float  mCompress       = 5.0f;
    float  mSpeed         = 3.0f;
    float  mGate          = 0.0f;
    float  mOutputGainDB  = 0.0f;
    float  mOutputGainLinear = 1.0f;
    float  mMix           = 1.0f;
    bool   mBypassed      = false;

    // Envelope follower state (per channel) — gate + stages 1, 2, and 3
    float  mGateEnvelope[kMaxChannels] = { 0.0f, 0.0f };
    float  mGateGain[kMaxChannels]     = { 1.0f, 1.0f };
    float  mEnvelope[kMaxChannels]  = { 0.0f, 0.0f };
    float  mEnvelope2[kMaxChannels] = { 0.0f, 0.0f };
    float  mEnvelope3[kMaxChannels] = { 0.0f, 0.0f };

    // Time-domain coefficients (derived from SPEED, recomputed on change)
    float  mAttackCoeff   = 0.0f;
    float  mReleaseCoeff  = 0.0f;
    // Stage 2: slower attack + longer release than Stage 1 (different timescale = real stacked interaction)
    float  mAttackCoeff2  = 0.0f;
    float  mReleaseCoeff2 = 0.0f;
    // Stage 3: ceiling limiter — always fast, tight range
    float  mAttackCoeff3  = 0.0f;
    float  mReleaseCoeff3 = 0.0f;
    // Gate: fixed time constants, computed once in initialize()
    float  mGateAttackCoeff  = 0.0f;
    float  mGateReleaseCoeff = 0.0f;

    // Gain reduction metering (written on render thread, read on UI thread — float read is tolerable)
    float    mGainReductionDB  = 0.0f;
    float    mMeterSmoothed    = 0.0f;  // ballistic-smoothed value exposed to VU needle
    uint64_t mRenderGeneration = 0;     // incremented each render call; UI checks for idle
    uint64_t mLastUIGeneration = 0;     // last generation seen by getGainReductionDB (UI thread only)

    AUAudioFrameCount mMaxFramesToRender = 1024;
};

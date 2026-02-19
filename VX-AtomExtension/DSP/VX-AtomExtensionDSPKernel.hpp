//
//  VXAtomExtensionDSPKernel.hpp
//  VXAtomExtension
//
//  Simple gain plugin for TyAu-VXAtom
//

#pragma once

#include <AudioToolbox/AudioToolbox.h>
#include <algorithm>
#include <span>

#include "VX-AtomExtensionParameterAddresses.h"

/*
 VXAtomExtensionDSPKernel
 As a non-ObjC class, this is safe to use from render thread.
 */
class VXAtomExtensionDSPKernel {
public:
    void initialize(int inputChannelCount, int outputChannelCount, double inSampleRate) {
        mSampleRate = inSampleRate;
    }

    void deInitialize() {
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
            case VXAtomExtensionParameterAddress::gain:
                mGain = value;
                break;
            case VXAtomExtensionParameterAddress::bypass:
                mBypassed = (value >= 0.5f);
                break;
        }
    }

    AUValue getParameter(AUParameterAddress address) {
        switch (address) {
            case VXAtomExtensionParameterAddress::gain:
                return (AUValue)mGain;
            case VXAtomExtensionParameterAddress::bypass:
                return (AUValue)(mBypassed ? 1.0f : 0.0f);
            default:
                return 0.f;
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

    /**
     MARK: - Internal Process

     This function does the core signal processing.
     Do your custom DSP here.
     */
    void process(std::span<float const*> inputBuffers, std::span<float *> outputBuffers, AUEventSampleTime bufferStartTime, AUAudioFrameCount frameCount) {
        assert(inputBuffers.size() == outputBuffers.size());

        if (mBypassed) {
            // Pass the samples through unmodified
            for (UInt32 channel = 0; channel < inputBuffers.size(); ++channel) {
                std::copy_n(inputBuffers[channel], frameCount, outputBuffers[channel]);
            }
        } else {
            // Apply gain to each sample
            for (UInt32 channel = 0; channel < inputBuffers.size(); ++channel) {
                for (UInt32 frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
                    float input = inputBuffers[channel][frameIndex];
                    float output = input * mGain;
                    outputBuffers[channel][frameIndex] = output;
                }
            }
        }
    }

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

    // MARK: Member Variables
    AUHostMusicalContextBlock mMusicalContextBlock;
    double mSampleRate = 44100.0;
    float mGain = 1.0f;  // 0.0 to 2.0
    bool mBypassed = false;
    AUAudioFrameCount mMaxFramesToRender = 1024;
};

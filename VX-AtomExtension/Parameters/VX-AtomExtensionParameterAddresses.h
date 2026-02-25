//
//  VXAtomExtensionParameterAddresses.h
//  VXAtomExtension
//
//  Created by Taylor Page on 1/22/26.
//

#pragma once

#include <AudioToolbox/AUParameters.h>

typedef NS_ENUM(AUParameterAddress, VXAtomExtensionParameterAddress) {
    squeeze    = 0,   // Main compression intensity (threshold + ratio + saturation)
    speed      = 1,   // Envelope character: 0=slow optical, 10=fast FET
    gate       = 2,   // Noise gate threshold (0=off, 10=aggressive)
    outputGain = 3,   // Output trim (-12 to +12 dB)
    mix        = 4,   // Parallel compression blend (0=dry, 1=full wet)
    bypass     = 5
};

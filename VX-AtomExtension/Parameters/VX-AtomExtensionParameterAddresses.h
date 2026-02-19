//
//  VXAtomExtensionParameterAddresses.h
//  VXAtomExtension
//
//  Created by Taylor Page on 1/22/26.
//

#pragma once

#include <AudioToolbox/AUParameters.h>

typedef NS_ENUM(AUParameterAddress, VXAtomExtensionParameterAddress) {
    gain = 0,
    bypass = 1
};

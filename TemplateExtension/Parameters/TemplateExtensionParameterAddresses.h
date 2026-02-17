//
//  TemplateExtensionParameterAddresses.h
//  TemplateExtension
//
//  Created by Taylor Page on 1/22/26.
//

#pragma once

#include <AudioToolbox/AUParameters.h>

typedef NS_ENUM(AUParameterAddress, TemplateExtensionParameterAddress) {
    gain = 0,
    bypass = 1
};

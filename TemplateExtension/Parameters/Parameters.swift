//
//  Parameters.swift
//  TemplateExtension
//
//  Created by Taylor Page on 1/22/26.
//

import Foundation
import AudioToolbox

let TemplateExtensionParameterSpecs = ParameterTreeSpec {
    ParameterGroupSpec(identifier: "global", name: "Global") {
        ParameterSpec(
            address: .gain,
            identifier: "gain",
            name: "Gain",
            units: .linearGain,
            valueRange: 0.0...2.0,
            defaultValue: 1.0
        )
        ParameterSpec(
            address: .bypass,
            identifier: "bypass",
            name: "Bypass",
            units: .boolean,
            valueRange: 0.0...1.0,
            defaultValue: 0.0
        )
    }
}

extension ParameterSpec {
    init(
        address: TemplateExtensionParameterAddress,
        identifier: String,
        name: String,
        units: AudioUnitParameterUnit,
        valueRange: ClosedRange<AUValue>,
        defaultValue: AUValue,
        unitName: String? = nil,
        flags: AudioUnitParameterOptions = [AudioUnitParameterOptions.flag_IsWritable, AudioUnitParameterOptions.flag_IsReadable],
        valueStrings: [String]? = nil,
        dependentParameters: [NSNumber]? = nil
    ) {
        self.init(address: address.rawValue,
                  identifier: identifier,
                  name: name,
                  units: units,
                  valueRange: valueRange,
                  defaultValue: defaultValue,
                  unitName: unitName,
                  flags: flags,
                  valueStrings: valueStrings,
                  dependentParameters: dependentParameters)
    }
}

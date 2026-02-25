//
//  Parameters.swift
//  VXAtomExtension
//
//  Created by Taylor Page on 1/22/26.
//

import Foundation
import AudioToolbox

let VXAtomExtensionParameterSpecs = ParameterTreeSpec {
    ParameterGroupSpec(identifier: "global", name: "Global") {
        ParameterSpec(
            address: .compress,
            identifier: "compress",
            name: "Compress",
            units: .generic,
            valueRange: 0.0...10.0,
            defaultValue: 5.0
        )
        ParameterSpec(
            address: .speed,
            identifier: "speed",
            name: "Speed",
            units: .generic,
            valueRange: 0.0...10.0,
            defaultValue: 3.0
        )
        ParameterSpec(
            address: .gate,
            identifier: "gate",
            name: "Gate",
            units: .generic,
            valueRange: 0.0...10.0,
            defaultValue: 0.0
        )
        ParameterSpec(
            address: .outputGain,
            identifier: "outputGain",
            name: "Output",
            units: .decibels,
            valueRange: -24.0...24.0,
            defaultValue: 0.0
        )
        ParameterSpec(
            address: .mix,
            identifier: "mix",
            name: "Mix",
            units: .generic,
            valueRange: 0.0...1.0,
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
        address: VXAtomExtensionParameterAddress,
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

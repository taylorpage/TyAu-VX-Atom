//
//  ParameterKnob.swift
//  TemplateExtension
//
//  A rotary knob control for AU parameters
//

import SwiftUI

struct ParameterKnob: View {
    @State var param: ObservableAUParameter
    let size: CGFloat

    @State private var isDragging = false
    @State private var lastDragValue: CGFloat = 0

    var knobSize: CGFloat { size }
    var scaleRadius: CGFloat { size * 0.464 }  // Proportional to size

    init(param: ObservableAUParameter, size: CGFloat = 140) {
        self.param = param
        self.size = size
    }

    var specifier: String {
        switch param.unit {
        case .midiNoteNumber:
            return "%.0f"
        default:
            return "%.2f"
        }
    }

    // Convert parameter value (0-1 normalized) to angle
    var angle: Angle {
        let normalizedValue = (param.value - param.min) / (param.max - param.min)
        let startAngle: Double = -135 // Start at bottom-left (7 o'clock)
        let endAngle: Double = 135    // End at bottom-right (5 o'clock - 270 degrees of rotation)
        let angleRange = endAngle - startAngle
        return Angle(degrees: startAngle + (Double(normalizedValue) * angleRange))
    }

    var body: some View {
        ZStack {
            // Scale markings (0-10) - Simple white tick marks with shadow
            ForEach(0..<11) { i in
                Rectangle()
                    .fill(Color.white)
                    .frame(width: 2, height: i % 2 == 0 ? 12 : 8)
                    .shadow(color: Color(red: 0.1, green: 0.15, blue: 0.25).opacity(0.8), radius: 1, x: -1, y: -1)
                    .shadow(color: Color(red: 0.1, green: 0.15, blue: 0.25).opacity(0.8), radius: 1, x: 1, y: -1)
                    .shadow(color: Color(red: 0.1, green: 0.15, blue: 0.25).opacity(0.8), radius: 1, x: -1, y: 1)
                    .shadow(color: Color(red: 0.1, green: 0.15, blue: 0.25).opacity(0.8), radius: 1, x: 1, y: 1)
                    .shadow(color: Color(red: 0.1, green: 0.15, blue: 0.25).opacity(0.5), radius: 2, x: 0, y: 0)
                    .offset(y: -scaleRadius)
                    .rotationEffect(Angle(degrees: -135 + (270.0 / 10.0) * Double(i)))
            }

            // Knob image with rotation
            if let knobImage = NSImage(named: "knob") {
                Image(nsImage: knobImage)
                    .resizable()
                    .aspectRatio(contentMode: .fit)
                    .frame(width: knobSize, height: knobSize)
                    .rotationEffect(angle)
                    .shadow(color: .black.opacity(0.5), radius: 6, x: 0, y: 3)
            } else {
                // Fallback if image not found
                Circle()
                    .fill(
                        RadialGradient(
                            gradient: Gradient(colors: [
                                Color(red: 0.3, green: 0.3, blue: 0.3),
                                Color(red: 0.15, green: 0.15, blue: 0.15),
                                Color(red: 0.25, green: 0.25, blue: 0.25)
                            ]),
                            center: .center,
                            startRadius: 0,
                            endRadius: knobSize / 2
                        )
                    )
                    .frame(width: knobSize, height: knobSize)
                    .overlay(
                        Circle()
                            .stroke(Color.black.opacity(0.6), lineWidth: 3)
                    )
                    .shadow(color: .black.opacity(0.5), radius: 6, x: 0, y: 3)
            }
        }
        .frame(width: size + scaleRadius * 2, height: size + scaleRadius * 2)
        .gesture(
            DragGesture(minimumDistance: 0)
                .onChanged { gesture in
                    if !isDragging {
                        isDragging = true
                        lastDragValue = gesture.translation.height
                        param.onEditingChanged(true)
                    }

                    // Vertical drag to control value
                    let delta = lastDragValue - gesture.translation.height
                    let sensitivity: CGFloat = 0.005
                    let valueChange = Float(delta * sensitivity) * (param.max - param.min)

                    let newValue = param.value + valueChange
                    param.value = min(max(newValue, param.min), param.max)

                    lastDragValue = gesture.translation.height
                }
                .onEnded { _ in
                    isDragging = false
                    param.onEditingChanged(false)
                }
        )
        .accessibility(identifier: param.displayName)
    }
}

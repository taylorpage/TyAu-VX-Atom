//
//  TemplateExtensionMainView.swift
//  TemplateExtension
//
//  Simple gain plugin UI for TyAu-Template
//

import SwiftUI

struct TemplateExtensionMainView: View {
    var parameterTree: ObservableAUParameterGroup

    var body: some View {
        ZStack {
            // Light grey gradient background
            LinearGradient(
                gradient: Gradient(colors: [
                    Color(white: 0.85),
                    Color(white: 0.75)
                ]),
                startPoint: .top,
                endPoint: .bottom
            )
            .cornerRadius(8)

            VStack(spacing: 20) {
                // Title
                Text("TEMPLATE")
                    .font(.system(size: 20, weight: .bold))
                    .foregroundColor(.black)
                    .padding(.top, 20)

                // LED indicator
                Circle()
                    .fill(bypassParam.boolValue ? Color.gray : Color.green)
                    .frame(width: 10, height: 10)
                    .shadow(color: bypassParam.boolValue ? .clear : .green.opacity(0.8), radius: 6)

                Spacer()

                // Gain knob
                VStack(spacing: 10) {
                    ParameterKnob(param: parameterTree.global.gain, size: 120)
                    Text("GAIN")
                        .font(.system(size: 14, weight: .bold))
                        .foregroundColor(.black)
                }

                Spacer()

                // Bypass button
                BypassButton(param: parameterTree.global.bypass)

                Spacer()

                // Logo
                Text("TaylorAudio")
                    .font(.caption)
                    .foregroundColor(.gray)
                    .padding(.bottom, 20)
            }
            .padding()
        }
        .frame(width: 300, height: 450)
    }

    // MARK: - Computed Properties

    var bypassParam: ObservableAUParameter {
        parameterTree.global.bypass
    }
}

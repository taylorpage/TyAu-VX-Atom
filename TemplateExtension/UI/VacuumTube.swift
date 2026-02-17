//
//  VacuumTube.swift
//  TemplateExtension
//
//  Created by Claude on 2/4/26.
//

import SwiftUI

/// A vacuum tube visual component that glows based on signal level
struct VacuumTube: View {
    /// Signal level from 0.0 to 1.0
    let signalLevel: Double

    /// Whether the effect is bypassed
    let isBypassed: Bool

    /// Size of the tube
    let size: CGSize

    init(signalLevel: Double = 0.0, isBypassed: Bool = false, width: CGFloat = 60, height: CGFloat = 120) {
        self.signalLevel = signalLevel
        self.isBypassed = isBypassed
        self.size = CGSize(width: width, height: height)
    }

    var body: some View {
        ZStack {
            // Metal base
            tubeBase

            // Glass envelope
            glassEnvelope

            // Internal components (plates and grids)
            internalComponents

            // Glow effect (orange/amber)
            if !isBypassed {
                glowEffect
            }

            // Glass reflection highlight
            glassHighlight
        }
        .frame(width: size.width, height: size.height)
    }

    // MARK: - Visual Components

    private var tubeBase: some View {
        VStack(spacing: 0) {
            Spacer()

            // Base cylinder
            RoundedRectangle(cornerRadius: 4)
                .fill(
                    LinearGradient(
                        colors: [
                            Color(white: 0.5),
                            Color(white: 0.35),
                            Color(white: 0.3),
                            Color(white: 0.4)
                        ],
                        startPoint: .leading,
                        endPoint: .trailing
                    )
                )
                .frame(height: size.height * 0.2)
                .overlay(
                    // Pin detail lines
                    HStack(spacing: size.width * 0.12) {
                        ForEach(0..<4) { _ in
                            Rectangle()
                                .fill(Color(white: 0.2))
                                .frame(width: 1)
                        }
                    }
                )
        }
    }

    private var glassEnvelope: some View {
        VStack(spacing: 0) {
            // Top dome
            Circle()
                .fill(
                    LinearGradient(
                        colors: [
                            Color.white.opacity(0.05),
                            Color.white.opacity(0.02)
                        ],
                        startPoint: .top,
                        endPoint: .bottom
                    )
                )
                .frame(height: size.width)
                .offset(y: size.width * 0.4)

            // Main cylinder
            Rectangle()
                .fill(Color.white.opacity(0.03))
                .overlay(
                    // Subtle vertical gradient for glass depth
                    LinearGradient(
                        colors: [
                            Color.white.opacity(0.05),
                            Color.clear,
                            Color.white.opacity(0.02)
                        ],
                        startPoint: .top,
                        endPoint: .bottom
                    )
                )

            Spacer()
                .frame(height: size.height * 0.2)
        }
        .overlay(
            // Glass edge highlighting
            RoundedRectangle(cornerRadius: size.width / 2)
                .stroke(Color.white.opacity(0.1), lineWidth: 0.5)
        )
    }

    private var internalComponents: some View {
        VStack(spacing: size.height * 0.05) {
            Spacer()
                .frame(height: size.height * 0.15)

            // Top plate (anode)
            Capsule()
                .fill(Color(white: 0.3))
                .frame(width: size.width * 0.4, height: size.height * 0.08)

            // Grid 1
            gridStructure
                .frame(height: size.height * 0.12)

            // Grid 2
            gridStructure
                .frame(height: size.height * 0.12)

            // Cathode (bottom element) — glows with signal, dark when silent/bypassed
            RoundedRectangle(cornerRadius: 2)
                .fill(
                    isBypassed ? Color(white: 0.25) : Color.orange.opacity(signalLevel > 0 ? 0.2 + pow(signalLevel, 0.5) * 0.5 : 0.0)
                )
                .frame(width: size.width * 0.5, height: size.height * 0.06)

            Spacer()
        }
    }

    private var gridStructure: some View {
        HStack(spacing: 2) {
            ForEach(0..<5) { _ in
                Rectangle()
                    .fill(Color(white: 0.35))
                    .frame(width: 1)
            }
        }
        .frame(width: size.width * 0.6)
    }

    private var glowEffect: some View {
        // Sqrt lifts quiet signals so they're visible, but stays 0 at true silence.
        let glowOpacity = signalLevel > 0 ? pow(signalLevel, 0.5) : 0.0

        return VStack(spacing: 0) {
            Spacer()
                .frame(height: size.height * 0.25)

            // Main glow area
            Ellipse()
                .fill(
                    RadialGradient(
                        colors: [
                            Color.orange.opacity(glowOpacity),
                            Color.orange.opacity(glowOpacity * 0.7),
                            Color.red.opacity(glowOpacity * 0.4),
                            Color.clear
                        ],
                        center: .center,
                        startRadius: 0,
                        endRadius: size.width * 0.5
                    )
                )
                .frame(width: size.width * 0.8, height: size.height * 0.4)
                .blur(radius: 8)

            Spacer()
                .frame(height: size.height * 0.2)
        }
    }

    private var glassHighlight: some View {
        VStack(spacing: 0) {
            // Top highlight reflection
            Circle()
                .fill(
                    LinearGradient(
                        colors: [
                            Color.white.opacity(0.3),
                            Color.white.opacity(0.1),
                            Color.clear
                        ],
                        startPoint: .topLeading,
                        endPoint: .bottomTrailing
                    )
                )
                .frame(width: size.width * 0.3, height: size.width * 0.3)
                .offset(x: -size.width * 0.15, y: size.height * 0.05)

            Spacer()
        }
    }
}

// MARK: - Preview

#Preview {
    VStack(spacing: 30) {
        HStack(spacing: 20) {
            VStack {
                VacuumTube(signalLevel: 0.0, isBypassed: false)
                Text("No Signal")
                    .font(.caption)
            }

            VStack {
                VacuumTube(signalLevel: 0.5, isBypassed: false)
                Text("Medium Signal")
                    .font(.caption)
            }

            VStack {
                VacuumTube(signalLevel: 1.0, isBypassed: false)
                Text("High Signal")
                    .font(.caption)
            }

            VStack {
                VacuumTube(signalLevel: 0.5, isBypassed: true)
                Text("Bypassed")
                    .font(.caption)
            }
        }
    }
    .padding()
    .background(Color(white: 0.85))
}

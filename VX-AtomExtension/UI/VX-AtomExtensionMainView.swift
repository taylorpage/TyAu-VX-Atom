//
//  VX-AtomExtensionMainView.swift
//  VXAtomExtension
//
//  VX-ATOM Character Compressor — Nuclear/Fallout Era UI
//

import SwiftUI

// MARK: - Color Palette

private extension Color {
    static let vxTealDark   = Color(red: 0.176, green: 0.420, blue: 0.353)
    static let vxTealMid    = Color(red: 0.290, green: 0.545, blue: 0.478)
    static let vxYellow     = Color(red: 1.000, green: 0.850, blue: 0.000)
    static let vxRed        = Color(red: 0.820, green: 0.118, blue: 0.118)
    static let vxIvory      = Color(red: 0.945, green: 0.929, blue: 0.855)
    static let vxTextLight  = Color(red: 0.878, green: 0.867, blue: 0.800)
    static let vxTextDim    = Color(red: 0.580, green: 0.615, blue: 0.545)
}

// MARK: - VU Meter (Gain Reduction Display)

private struct VUMeter: View {
    let gainReductionDB: Float

    var body: some View {
        Canvas { ctx, size in
            // Ivory background
            ctx.fill(Path(CGRect(origin: .zero, size: size)), with: .color(.vxIvory))

            // Scale marks and labels
            let marks: [(Float, String, Bool)] = [
                (0, "0", true), (3, "", false), (5, "5", true),
                (7, "", false), (10, "10", true), (14, "", false), (20, "20", true)
            ]
            let insetX: CGFloat = 12
            let usableW = size.width - insetX * 2

            for (db, label, isMajor) in marks {
                let x = CGFloat(db) / 20.0 * usableW + insetX
                let tickH: CGFloat = isMajor ? 10 : 6
                var tick = Path()
                tick.move(to: CGPoint(x: x, y: size.height - tickH - 4))
                tick.addLine(to: CGPoint(x: x, y: size.height - 4))
                ctx.stroke(tick, with: .color(.black.opacity(0.50)), lineWidth: isMajor ? 1.5 : 1)

                if isMajor && !label.isEmpty {
                    let resolved = ctx.resolve(
                        Text(label)
                            .font(.system(size: 7.5, weight: .regular, design: .monospaced))
                            .foregroundStyle(Color.black.opacity(0.55))
                    )
                    ctx.draw(resolved, at: CGPoint(x: x, y: size.height - tickH - 5), anchor: .bottom)
                }
            }

            // Needle (vertical line showing GR amount)
            let clampedGR = CGFloat(min(max(gainReductionDB, 0), 20))
            let needleX = clampedGR / 20.0 * usableW + insetX
            var needle = Path()
            needle.move(to: CGPoint(x: needleX, y: 4))
            needle.addLine(to: CGPoint(x: needleX, y: size.height - 16))
            ctx.stroke(needle, with: .color(.vxRed), lineWidth: 2)

            // Needle pivot dot
            let dotR: CGFloat = 3
            ctx.fill(
                Path(ellipseIn: CGRect(x: needleX - dotR, y: size.height - 16 - dotR,
                                       width: dotR * 2, height: dotR * 2)),
                with: .color(.vxRed)
            )

            // Glass glare
            ctx.fill(
                Path(roundedRect: CGRect(x: 2, y: 2,
                                          width: size.width - 4, height: size.height * 0.30),
                     cornerRadius: 2),
                with: .color(.white.opacity(0.18))
            )

            // Border
            ctx.stroke(
                Path(roundedRect: CGRect(origin: .zero, size: size).insetBy(dx: 0.5, dy: 0.5),
                     cornerRadius: 3),
                with: .color(.black.opacity(0.45)),
                lineWidth: 1
            )
        }
        .frame(height: 46)
        .clipShape(RoundedRectangle(cornerRadius: 3))
        .shadow(color: .black.opacity(0.45), radius: 3, x: 0, y: 2)
    }
}

// MARK: - Corner Bolt

private struct CornerBolt: View {
    var body: some View {
        ZStack {
            Circle()
                .fill(
                    RadialGradient(
                        colors: [Color(white: 0.58), Color(white: 0.22)],
                        center: .topLeading,
                        startRadius: 0,
                        endRadius: 9
                    )
                )
                .frame(width: 13, height: 13)
                .overlay(Circle().stroke(Color.black.opacity(0.55), lineWidth: 1))
            // Phillips head crosshair
            Rectangle()
                .fill(Color.black.opacity(0.40))
                .frame(width: 7, height: 1.5)
            Rectangle()
                .fill(Color.black.opacity(0.40))
                .frame(width: 1.5, height: 7)
        }
    }
}

// MARK: - Knob + Label composite

private struct LabeledKnob: View {
    let param: ObservableAUParameter
    let label: String
    let knobSize: CGFloat

    var body: some View {
        VStack(spacing: 3) {
            ParameterKnob(param: param, size: knobSize)
            Text(label)
                .font(.system(size: 10, weight: .bold, design: .monospaced))
                .foregroundColor(.vxTextLight)
                .tracking(1.5)
        }
    }
}

// MARK: - SQUEEZE Arc Labels

private struct SqueezeArcLabels: View {
    let knobRadius: CGFloat  // visual knob radius (knobSize / 2)

    // Angles follow clock convention: -135 = 7 o'clock (min), +135 = 5 o'clock (max)
    private let labels: [(angle: Double, text: String)] = [
        (-135, "LIGHT"),
        (-48,  "THHKY"),
        (48,   "RADIO"),
        (135,  "TOO\nMUCH")
    ]

    var body: some View {
        ZStack {
            ForEach(labels, id: \.angle) { item in
                // Convert clock-convention angle to SwiftUI screen coordinates
                // Subtract 90° so 0° maps to 12 o'clock; y increases downward in screen
                let screenRad = (item.angle - 90.0) * .pi / 180.0
                let r = knobRadius + 28.0
                Text(item.text)
                    .font(.system(size: 7.5, weight: .semibold, design: .monospaced))
                    .foregroundColor(.vxTextDim)
                    .multilineTextAlignment(.center)
                    .tracking(0.5)
                    .fixedSize()
                    .offset(
                        x: CGFloat(cos(screenRad)) * r,
                        y: CGFloat(sin(screenRad)) * r
                    )
            }
        }
    }
}

// MARK: - Main View

struct VXAtomExtensionMainView: View {
    var parameterTree: ObservableAUParameterGroup
    var gainReductionProvider: (() -> Float)?

    private let panelWidth:  CGFloat = 300
    private let panelHeight: CGFloat = 580

    var body: some View {
        ZStack {
            // MARK: Panel background
            backgroundPanel

            // MARK: Corner bolts
            CornerBolt()
                .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
                .padding(10)
            CornerBolt()
                .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topTrailing)
                .padding(10)
            CornerBolt()
                .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .bottomLeading)
                .padding(10)
            CornerBolt()
                .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .bottomTrailing)
                .padding(10)

            // MARK: Content stack
            VStack(spacing: 0) {

                // Title bar
                titleBar
                    .frame(height: 40)

                // VU Meter
                TimelineView(.animation(minimumInterval: 1.0 / 30.0)) { _ in
                    VUMeter(gainReductionDB: gainReductionProvider?() ?? 0.0)
                }
                .padding(.horizontal, 22)
                .padding(.bottom, 8)

                // SQUEEZE knob (large, center character)
                squeezeSection
                    .padding(.bottom, 10)

                // SPEED & TONE row
                HStack(spacing: 0) {
                    LabeledKnob(param: parameterTree.global.speed,
                                label: "SPEED", knobSize: 64)
                        .frame(maxWidth: .infinity)

                    // Radioactive decoration (center divider)
                    VStack(spacing: 3) {
                        Text("☢")
                            .font(.system(size: 22))
                            .foregroundColor(Color.vxYellow.opacity(0.70))
                        Text("ATOM")
                            .font(.system(size: 6, weight: .black, design: .monospaced))
                            .foregroundColor(Color.vxTextDim.opacity(0.65))
                            .tracking(2.5)
                    }
                    .frame(width: 48)

                    LabeledKnob(param: parameterTree.global.tone,
                                label: "TONE", knobSize: 64)
                        .frame(maxWidth: .infinity)
                }
                .padding(.horizontal, 12)
                .padding(.bottom, 8)

                // Thin separator
                Rectangle()
                    .fill(Color.white.opacity(0.07))
                    .frame(width: 200, height: 1)
                    .padding(.bottom, 8)

                // OUTPUT & MIX row
                HStack(spacing: 0) {
                    LabeledKnob(param: parameterTree.global.outputGain,
                                label: "OUTPUT", knobSize: 64)
                        .frame(maxWidth: .infinity)
                    LabeledKnob(param: parameterTree.global.mix,
                                label: "MIX", knobSize: 64)
                        .frame(maxWidth: .infinity)
                }
                .padding(.horizontal, 26)
                .padding(.bottom, 10)

                // Bypass stomp
                BypassButton(param: parameterTree.global.bypass)
                    .padding(.bottom, 8)

                // Footer
                Text("T A Y L O R A U D I O")
                    .font(.system(size: 7.5, weight: .medium, design: .monospaced))
                    .foregroundColor(Color.vxTextDim.opacity(0.55))
                    .tracking(3)
                    .padding(.bottom, 10)
            }
        }
        .frame(width: panelWidth, height: panelHeight)
    }

    // MARK: - Subviews

    private var backgroundPanel: some View {
        ZStack {
            LinearGradient(
                colors: [Color.vxTealMid, Color.vxTealDark],
                startPoint: .top,
                endPoint: .bottom
            )
            // Edge vignette
            RadialGradient(
                colors: [Color.clear, Color.black.opacity(0.28)],
                center: .center,
                startRadius: 90,
                endRadius: 220
            )
        }
        .cornerRadius(8)
        .overlay(
            RoundedRectangle(cornerRadius: 8)
                .stroke(Color.black.opacity(0.65), lineWidth: 2)
        )
    }

    private var titleBar: some View {
        ZStack {
            // Subtle top highlight stripe
            VStack {
                Rectangle()
                    .fill(Color.white.opacity(0.10))
                    .frame(height: 1)
                Spacer()
            }
            HStack(alignment: .center, spacing: 0) {
                // Left badge
                Text("MK-I")
                    .font(.system(size: 8, weight: .black, design: .monospaced))
                    .foregroundColor(Color.vxTextDim.opacity(0.65))
                    .tracking(1.5)
                    .padding(.leading, 24)

                Spacer()

                // Main title
                Text("VX-ATOM")
                    .font(.system(size: 22, weight: .black, design: .monospaced))
                    .foregroundColor(.vxTextLight)
                    .tracking(5)
                    .shadow(color: .black.opacity(0.55), radius: 1, x: 1, y: 1)

                Spacer()

                // LED indicator (yellow = active, dim = bypassed)
                Circle()
                    .fill(bypassParam.boolValue
                          ? Color(white: 0.25)
                          : Color.vxYellow)
                    .frame(width: 9, height: 9)
                    .shadow(color: bypassParam.boolValue
                            ? .clear
                            : Color.vxYellow.opacity(0.95),
                            radius: 6)
                    .padding(.trailing, 24)
            }
        }
    }

    private var squeezeSection: some View {
        VStack(spacing: 0) {
            ZStack {
                // Arc position labels around the knob
                SqueezeArcLabels(knobRadius: 43)  // half of size=86
                ParameterKnob(param: parameterTree.global.squeeze, size: 86)
            }
            .padding(.bottom, 4)

            // Engraved separator line
            Rectangle()
                .fill(Color.white.opacity(0.08))
                .frame(width: 76, height: 1)
                .padding(.bottom, 4)

            Text("SQUEEZE")
                .font(.system(size: 13, weight: .black, design: .monospaced))
                .foregroundColor(.vxTextLight)
                .tracking(3)

            Text("You sure?")
                .font(.system(size: 8.5, weight: .regular, design: .monospaced))
                .foregroundColor(.vxTextDim)
                .tracking(0.5)
                .padding(.top, 1)
        }
    }

    // MARK: - Helpers

    private var bypassParam: ObservableAUParameter {
        parameterTree.global.bypass
    }
}

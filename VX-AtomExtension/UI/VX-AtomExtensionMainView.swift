//
//  VX-AtomExtensionMainView.swift
//  VXAtomExtension
//
//  VX-ATOM Character Compressor — Nuclear/Fallout Era UI
//

import SwiftUI

// MARK: - Bundle

private class BundleToken {}
private let extensionBundle = Bundle(for: BundleToken.self)

// MARK: - Color Palette

private extension Color {
    static let vxTealDark   = Color(red: 0.176, green: 0.420, blue: 0.353)
    static let vxTealMid    = Color(red: 0.290, green: 0.545, blue: 0.478)
    static let vxYellow     = Color(red: 1.000, green: 0.850, blue: 0.000)
    static let vxRed        = Color(red: 0.640, green: 0.048, blue: 0.048)
    static let vxIvory      = Color(red: 0.945, green: 0.929, blue: 0.855)
    static let vxTextLight  = Color(red: 0.878, green: 0.867, blue: 0.800)
    static let vxTextDim    = Color(red: 0.580, green: 0.615, blue: 0.545)
}

// MARK: - VU Meter (Gain Reduction Display)

private struct VUMeter: View {
    let gainReductionDB: Float

    var body: some View {
        ZStack {
            // Meter face
            Canvas { ctx, size in
                // Rails flush with face edges; ticks span full width
                let insetX: CGFloat = 16   // left/right margin reserved for dots
                let usableW = size.width - insetX * 2
                let topRailY:    CGFloat = 2
                let bottomRailY: CGFloat = size.height - 2
                let majorH: CGFloat = 11   // tick length inward from each rail
                let minorH: CGFloat = 6
                let labelY = size.height / 2   // labels float in the vertical center

                // Indicator dots — vertically centered, in the left/right margins
                let dotSize: CGFloat = 8
                let dotY = labelY - dotSize / 2
                // Green (left) — always on
                ctx.fill(
                    Path(ellipseIn: CGRect(x: 4, y: dotY, width: dotSize, height: dotSize)),
                    with: .color(Color(red: 0.20, green: 0.85, blue: 0.22).opacity(0.92))
                )
                // Yellow (right) — dims when GR is light
                let heavyGR = gainReductionDB > 10
                ctx.fill(
                    Path(ellipseIn: CGRect(x: size.width - 4 - dotSize, y: dotY, width: dotSize, height: dotSize)),
                    with: .color(Color.vxYellow.opacity(heavyGR ? 0.95 : 0.30))
                )

                // 7 major ticks with 1 minor tick centered exactly between each pair
                let majorDBs = [1, 4, 7, 10, 13, 16, 19]

                // Minor ticks — computed as midpoints so they are perfectly centered
                for idx in 0..<majorDBs.count - 1 {
                    let midDB = CGFloat(majorDBs[idx] + majorDBs[idx + 1]) / 2.0
                    let x = midDB / 20.0 * usableW + insetX
                    let isRed = midDB >= 10
                    let color: GraphicsContext.Shading = .color(
                        isRed ? Color.vxRed.opacity(0.45) : .black.opacity(0.28)
                    )
                    var top = Path()
                    top.move(to: CGPoint(x: x, y: topRailY))
                    top.addLine(to: CGPoint(x: x, y: topRailY + minorH))
                    ctx.stroke(top, with: color, lineWidth: 0.8)
                    var bot = Path()
                    bot.move(to: CGPoint(x: x, y: bottomRailY))
                    bot.addLine(to: CGPoint(x: x, y: bottomRailY - minorH))
                    ctx.stroke(bot, with: color, lineWidth: 0.8)
                }

                // Major ticks + labels
                for db in majorDBs {
                    let x = CGFloat(db) / 20.0 * usableW + insetX
                    let isRed = db >= 10
                    let color: GraphicsContext.Shading = .color(
                        isRed ? Color.vxRed.opacity(0.85) : .black.opacity(0.70)
                    )
                    var top = Path()
                    top.move(to: CGPoint(x: x, y: topRailY))
                    top.addLine(to: CGPoint(x: x, y: topRailY + majorH))
                    ctx.stroke(top, with: color, lineWidth: 1.5)
                    var bot = Path()
                    bot.move(to: CGPoint(x: x, y: bottomRailY))
                    bot.addLine(to: CGPoint(x: x, y: bottomRailY - majorH))
                    ctx.stroke(bot, with: color, lineWidth: 1.5)

                    let t = ctx.resolve(
                        Text("\(db)")
                            .font(.system(size: 11, weight: .semibold, design: .monospaced))
                            .foregroundStyle(isRed ? Color.vxRed : Color.black.opacity(0.85))
                    )
                    ctx.draw(t, at: CGPoint(x: x, y: labelY), anchor: .center)
                }

                // Needle — full height, red
                let clampedGR = CGFloat(min(max(gainReductionDB, 0), 20))
                let needleX = clampedGR / 20.0 * usableW + insetX
                var needle = Path()
                needle.move(to: CGPoint(x: needleX, y: 2))
                needle.addLine(to: CGPoint(x: needleX, y: size.height - 2))
                ctx.stroke(needle, with: .color(.vxRed), lineWidth: 2.5)

            }
            .background {
                if let path = extensionBundle.path(forResource: "vuBackground", ofType: "png"),
                   let nsImage = NSImage(contentsOfFile: path) {
                    Image(nsImage: nsImage)
                        .resizable()
                        .scaledToFill()
                } else {
                    Color(red: 0.948, green: 0.930, blue: 0.852)
                }
            }
            .padding(.horizontal, 7)
            .padding(.vertical, 5)
            .clipShape(RoundedRectangle(cornerRadius: 2))
        }
        .frame(height: 80)
        .shadow(color: .black.opacity(0.65), radius: 5, x: 0, y: 3)
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
        VStack(spacing: 0) {
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

    private let panelWidth:  CGFloat = 240
    private let panelHeight: CGFloat = 608

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
                .padding(.top, 20)
                .padding(.horizontal, 22)
                .padding(.bottom, 42)

                // SPEED & TONE row
                HStack {
                    LabeledKnob(param: parameterTree.global.speed,
                                label: "SPEED", knobSize: 90)
                        .padding(.leading, 36)
                    Spacer()
                    LabeledKnob(param: parameterTree.global.tone,
                                label: "TONE", knobSize: 90)
                        .padding(.trailing, 36)
                }
                .padding(.bottom, -16)

                // SQUEEZE knob (large, center character)
                squeezeSection

                // OUTPUT & MIX row
                HStack {
                    LabeledKnob(param: parameterTree.global.outputGain,
                                label: "OUTPUT", knobSize: 90)
                        .padding(.leading, 36)
                    Spacer()
                    LabeledKnob(param: parameterTree.global.mix,
                                label: "MIX", knobSize: 90)
                        .padding(.trailing, 36)
                }
                .padding(.top, -16)
                .padding(.bottom, 16)

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
            if let path = extensionBundle.path(forResource: "background", ofType: "png"),
               let nsImage = NSImage(contentsOfFile: path) {
                Image(nsImage: nsImage)
                    .resizable()
                    .scaledToFill()
            }
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
            HStack(alignment: .center, spacing: 0) {
                // Left badge
                Text("MK-I")
                    .font(.system(size: 8, weight: .black, design: .monospaced))
                    .foregroundColor(Color.vxTextDim.opacity(0.65))
                    .tracking(1.5)
                    .padding(.leading, 36)

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
                    .padding(.trailing, 36)
            }
        }
    }

    private var squeezeSection: some View {
        VStack(spacing: 0) {
            ZStack {
                // Arc position labels around the knob
                SqueezeArcLabels(knobRadius: 90)  // half of size=180
                ParameterKnob(param: parameterTree.global.squeeze, size: 180)
            }

            HStack(spacing: 8) {
                Text("☢")
                    .font(.system(size: 14))
                    .foregroundColor(Color.vxYellow.opacity(0.70))
                Text("SQUEEZE")
                    .font(.system(size: 13, weight: .black, design: .monospaced))
                    .foregroundColor(.vxTextLight)
                    .tracking(3)
                Text("☢")
                    .font(.system(size: 14))
                    .foregroundColor(Color.vxYellow.opacity(0.70))
            }

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

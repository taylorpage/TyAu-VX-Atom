//
//  CustomJack.swift
//  TemplateExtension
//
//  Created by Taylor Page on 2/4/26.
//

import SwiftUI

struct CustomJack: View {
    enum Side {
        case left
        case right
    }

    let side: Side

    var body: some View {
        GeometryReader { geometry in
            Canvas { context, size in
                let centerY = size.height / 2

                // The jack consists of overlapping vertical rounded pill/capsule shapes
                // Each with different heights and shades to create 3D depth

                if side == .left {
                    // Segment 1: Leftmost thin dark edge
                    let seg1 = Path(roundedRect: CGRect(
                        x: 0,
                        y: centerY - size.height * 0.48,
                        width: size.width * 0.10,
                        height: size.height * 0.96
                    ), cornerRadius: size.width * 0.05)
                    context.fill(seg1, with: .color(Color(red: 70/255, green: 70/255, blue: 70/255)))

                    // Segment 2: Dark gray
                    let seg2 = Path(roundedRect: CGRect(
                        x: size.width * 0.06,
                        y: centerY - size.height * 0.42,
                        width: size.width * 0.14,
                        height: size.height * 0.84
                    ), cornerRadius: size.width * 0.07)
                    context.fill(seg2, with: .color(Color(red: 100/255, green: 100/255, blue: 100/255)))

                    // Segment 3: Medium gray
                    let seg3 = Path(roundedRect: CGRect(
                        x: size.width * 0.15,
                        y: centerY - size.height * 0.36,
                        width: size.width * 0.20,
                        height: size.height * 0.72
                    ), cornerRadius: size.width * 0.10)
                    context.fill(seg3, with: .color(Color(red: 145/255, green: 145/255, blue: 145/255)))

                    // Segment 4: Light gray (widest main body)
                    let seg4 = Path(roundedRect: CGRect(
                        x: size.width * 0.28,
                        y: centerY - size.height * 0.30,
                        width: size.width * 0.24,
                        height: size.height * 0.60
                    ), cornerRadius: size.width * 0.12)
                    context.fill(seg4, with: .color(Color(red: 190/255, green: 190/255, blue: 190/255)))

                    // Segment 5: Very light/white highlight (narrowest)
                    let seg5 = Path(roundedRect: CGRect(
                        x: size.width * 0.45,
                        y: centerY - size.height * 0.22,
                        width: size.width * 0.18,
                        height: size.height * 0.44
                    ), cornerRadius: size.width * 0.09)
                    context.fill(seg5, with: .color(Color(red: 225/255, green: 225/255, blue: 225/255)))

                    // Segment 6: Medium gray (getting darker again)
                    let seg6 = Path(roundedRect: CGRect(
                        x: size.width * 0.58,
                        y: centerY - size.height * 0.28,
                        width: size.width * 0.20,
                        height: size.height * 0.56
                    ), cornerRadius: size.width * 0.10)
                    context.fill(seg6, with: .color(Color(red: 135/255, green: 135/255, blue: 135/255)))

                    // Segment 7: Dark gray (rightmost)
                    let seg7 = Path(roundedRect: CGRect(
                        x: size.width * 0.73,
                        y: centerY - size.height * 0.38,
                        width: size.width * 0.27,
                        height: size.height * 0.76
                    ), cornerRadius: size.width * 0.10)
                    context.fill(seg7, with: .color(Color(red: 85/255, green: 85/255, blue: 85/255)))

                } else {
                    // Mirror for right side

                    // Segment 1: Rightmost thin dark edge
                    let seg1 = Path(roundedRect: CGRect(
                        x: size.width * 0.90,
                        y: centerY - size.height * 0.48,
                        width: size.width * 0.10,
                        height: size.height * 0.96
                    ), cornerRadius: size.width * 0.05)
                    context.fill(seg1, with: .color(Color(red: 70/255, green: 70/255, blue: 70/255)))

                    // Segment 2: Dark gray
                    let seg2 = Path(roundedRect: CGRect(
                        x: size.width * 0.80,
                        y: centerY - size.height * 0.42,
                        width: size.width * 0.14,
                        height: size.height * 0.84
                    ), cornerRadius: size.width * 0.07)
                    context.fill(seg2, with: .color(Color(red: 100/255, green: 100/255, blue: 100/255)))

                    // Segment 3: Medium gray
                    let seg3 = Path(roundedRect: CGRect(
                        x: size.width * 0.65,
                        y: centerY - size.height * 0.36,
                        width: size.width * 0.20,
                        height: size.height * 0.72
                    ), cornerRadius: size.width * 0.10)
                    context.fill(seg3, with: .color(Color(red: 145/255, green: 145/255, blue: 145/255)))

                    // Segment 4: Light gray (widest main body)
                    let seg4 = Path(roundedRect: CGRect(
                        x: size.width * 0.48,
                        y: centerY - size.height * 0.30,
                        width: size.width * 0.24,
                        height: size.height * 0.60
                    ), cornerRadius: size.width * 0.12)
                    context.fill(seg4, with: .color(Color(red: 190/255, green: 190/255, blue: 190/255)))

                    // Segment 5: Very light/white highlight (narrowest)
                    let seg5 = Path(roundedRect: CGRect(
                        x: size.width * 0.37,
                        y: centerY - size.height * 0.22,
                        width: size.width * 0.18,
                        height: size.height * 0.44
                    ), cornerRadius: size.width * 0.09)
                    context.fill(seg5, with: .color(Color(red: 225/255, green: 225/255, blue: 225/255)))

                    // Segment 6: Medium gray (getting darker again)
                    let seg6 = Path(roundedRect: CGRect(
                        x: size.width * 0.22,
                        y: centerY - size.height * 0.28,
                        width: size.width * 0.20,
                        height: size.height * 0.56
                    ), cornerRadius: size.width * 0.10)
                    context.fill(seg6, with: .color(Color(red: 135/255, green: 135/255, blue: 135/255)))

                    // Segment 7: Dark gray (leftmost)
                    let seg7 = Path(roundedRect: CGRect(
                        x: 0,
                        y: centerY - size.height * 0.38,
                        width: size.width * 0.27,
                        height: size.height * 0.76
                    ), cornerRadius: size.width * 0.10)
                    context.fill(seg7, with: .color(Color(red: 85/255, green: 85/255, blue: 85/255)))
                }
            }
        }
    }
}

#Preview {
    VStack(spacing: 20) {
        CustomJack(side: .left)
            .frame(width: 20, height: 60)
            .background(Color.black)

        CustomJack(side: .right)
            .frame(width: 20, height: 60)
            .background(Color.black)
    }
    .padding()
}

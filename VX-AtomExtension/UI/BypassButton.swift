//
//  BypassButton.swift
//  VXAtomExtension
//
//  A stomp switch button for audio effect bypass
//

import SwiftUI

struct BypassButton: View {
    @State var param: ObservableAUParameter

    let switchSize: CGFloat = 70

    var body: some View {
        // Simple stomp button without animation
        if let stompImage = NSImage(named: "stomp") {
            Image(nsImage: stompImage)
                .resizable()
                .aspectRatio(contentMode: .fit)
                .frame(width: switchSize, height: switchSize)
                .shadow(color: .black.opacity(0.5), radius: 4, x: 0, y: 2)
                .onTapGesture {
                    // Toggle bypass state
                    param.onEditingChanged(true)
                    param.boolValue.toggle()
                    param.onEditingChanged(false)
                }
                .accessibility(identifier: param.displayName)
        }
    }
}

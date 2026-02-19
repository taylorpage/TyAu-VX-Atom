#!/bin/bash

# TyAu-VX-Atom Build Script
# Builds the plugin in Debug configuration and registers it with the system

set -e  # Exit on error

echo "🎸 Building TyAu-VX-Atom plugin..."

# Build in Debug configuration
xcodebuild -project VX-Atom.xcodeproj \
    -scheme VX-Atom \
    -configuration Debug \
    build \
    -allowProvisioningUpdates

echo "✅ Build succeeded!"

# Register the Audio Unit extension
echo "📝 Registering Audio Unit extension..."
open /Users/taylorpage/Library/Developer/Xcode/DerivedData/VX-Atom-*/Build/Products/Debug/VX-Atom.app

echo "🎸 VX-Atom is ready! Load it in Logic Pro."

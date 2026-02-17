#!/bin/bash

# TyAu-Template Build Script
# Builds the plugin in Debug configuration and registers it with the system

set -e  # Exit on error

echo "🎸 Building TyAu-Template plugin..."

# Build in Debug configuration
xcodebuild -project Template.xcodeproj \
    -scheme Template \
    -configuration Debug \
    build \
    -allowProvisioningUpdates

echo "✅ Build succeeded!"

# Register the Audio Unit extension
echo "📝 Registering Audio Unit extension..."
open /Users/taylorpage/Library/Developer/Xcode/DerivedData/Template-*/Build/Products/Debug/Template.app

echo "🎸 Template is ready! Load it in Logic Pro."

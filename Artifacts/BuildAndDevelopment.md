# TyAu-TubeCity Build and Development Guide

This document contains essential information for building, testing, and developing the TyAu-TubeCity audio plugin.

## Project Location

**Repository Root:** `/Users/taylorpage/Repos/TyAu/TyAu-Pedals/TyAu-TubeCity/`

**Xcode Project:** `TubeCity.xcodeproj`

## Build Instructions

### Building from Command Line

The project must be built in **Debug** configuration for Logic Pro to pick up changes during development.

```bash
cd /Users/taylorpage/Repos/TyAu/TyAu-Pedals/TyAu-TubeCity
xcodebuild -project TubeCity.xcodeproj -scheme TubeCity -configuration Debug build
```

For Release builds:
```bash
xcodebuild -project TubeCity.xcodeproj -scheme TubeCity -configuration Release build
```

### Build Output Locations

**Debug Build:**
```
/Users/taylorpage/Library/Developer/Xcode/DerivedData/TubeCity-[hash]/Build/Products/Debug/TubeCity.app
```

**Audio Unit Extension (Debug):**
```
/Users/taylorpage/Library/Developer/Xcode/DerivedData/TubeCity-[hash]/Build/Products/Debug/TubeCity.app/Contents/PlugIns/TubeCityExtension.appex
```

**Release Build:**
```
/Users/taylorpage/Library/Developer/Xcode/DerivedData/TubeCity-[hash]/Build/Products/Release/TubeCity.app
```

## Development Workflow

### IMPORTANT: Always Rebuild After Making Changes

**Every time you make a code change, you MUST rebuild and re-register the plugin to test it.**

This is critical because:
- Audio Unit extensions are loaded from the built binary, not source code
- Logic Pro and other DAWs cache plugin binaries
- UI changes, parameter changes, and DSP changes all require rebuilding
- Without rebuilding, you will not see your changes reflected in the DAW

### Standard Development Cycle

After making any changes (UI, DSP, parameters, etc.), run:

```bash
cd /Users/taylorpage/Repos/TyAu/TyAu-Pedals/TyAu-TubeCity
xcodebuild -project TubeCity.xcodeproj -scheme TubeCity -configuration Debug build
open /Users/taylorpage/Library/Developer/Xcode/DerivedData/TubeCity-*/Build/Products/Debug/TubeCity.app
```

Opening the app after building ensures the Audio Unit extension is re-registered with the system.

### Iterative UI Development with Claude Code

**IMPORTANT:** When working with Claude Code on UI changes, Claude should rebuild after EACH code change so you can see the results immediately. This allows for rapid iteration and visual feedback.

**Recommended workflow:**
1. User requests a UI change (e.g., "move the logo closer to the stomp box")
2. Claude makes the code change
3. **Claude immediately rebuilds** using the build commands above
4. User provides feedback based on what they see
5. Repeat until satisfied

This iterative approach is much faster than batching multiple changes before rebuilding.

### Testing in Logic Pro

1. **Initial Setup:**
   - Build the Debug configuration
   - Launch `TubeCity.app` to register the Audio Unit extension
   - Open Logic Pro
   - The plugin will appear as "TubeCity" under Audio Units

2. **Making Changes:**
   - Edit source files (Swift, C++, etc.)
   - **ALWAYS rebuild in Debug configuration** (see Standard Development Cycle above)
   - **ALWAYS open the app** to re-register the plugin
   - Logic Pro will typically crash or reload when the plugin binary is overwritten
   - If Logic doesn't auto-reload, close and reopen the plugin window or restart Logic

3. **Plugin Registration:**
   The plugin is registered with the system via `pluginkit`. Check registration:
   ```bash
   pluginkit -m -v 2>&1 | grep -i tubecity
   ```

### Important Files and Directories

**UI Components:**
- `TubeCityExtension/UI/ParameterKnob.swift` - Rotary knob control
- `TubeCityExtension/UI/TubeCityExtensionMainView.swift` - Main plugin UI
- `TubeCityExtension/UI/ParameterSlider.swift` - Slider control

**Audio Processing:**
- `TubeCityExtension/Common/Audio Unit/TubeCityExtensionAudioUnit.swift` - Audio Unit wrapper
- `TubeCityExtension/DSP/TubeCityExtensionDSPKernel.hpp` - DSP implementation (C++)

**Parameters:**
- `TubeCityExtension/Parameters/Parameters.swift` - Plugin parameters definition

## Project Structure

```
TyAu-Pedals/TyAu-TubeCity/
├── Artifacts/                          # Documentation and design files
│   ├── SignalProcessing.md            # DSP implementation details
│   └── BuildAndDevelopment.md         # This file
├── TubeCity/                           # Host application
│   ├── TubeCityApp.swift
│   ├── ContentView.swift
│   ├── ValidationView.swift
│   └── Common/                         # Shared utilities
├── TubeCityExtension/                 # Audio Unit Extension (the plugin)
│   ├── UI/                            # SwiftUI interface
│   ├── DSP/                           # DSP kernel implementation
│   ├── Common/                        # Core AU implementation
│   ├── Parameters/                    # Parameter definitions
│   └── Resources/                     # Images and assets
└── TubeCity.xcodeproj                 # Xcode project
```

## Key Information

### Bundle Identifiers
- **App:** `com.taylor.audio.TubeCity`
- **Extension:** `com.taylor.audio.TubeCity.TubeCityExtension`

### Audio Unit Type
- **Type:** Effect (aufx)
- **Subtype:** Custom tube saturation
- **Manufacturer:** Taylor Audio

### Parameter Ranges
- **Drive:** 0.0 to 1.0 (0% to 100%)
  - Default: 0.5 (50%)
- **Output Volume:** 0.0 to 1.0 (0% to 100%)
  - Default: 0.8 (80%)
- **Tube Style:** 0, 1, 2
  - 0: Classic (symmetric clipping)
  - 1: Asymmetric (tube-like asymmetry)
  - 2: Warm (softer saturation)

### Code Signing
- **Developer:** jontaylorpage@gmail.com
- **Team:** SWA5UWWQY7
- **Certificate:** Apple Development

## Common Tasks

### Force Plugin Refresh in Logic
```bash
# Kill Logic Pro
killall "Logic Pro"

# Clear plugin cache (if needed)
killall -9 AudioComponentRegistrar

# Rebuild and restart
cd /Users/taylorpage/Repos/TyAu/TyAu-Pedals/TyAu-TubeCity
xcodebuild -project TubeCity.xcodeproj -scheme TubeCity -configuration Debug build
open /Users/taylorpage/Library/Developer/Xcode/DerivedData/TubeCity-*/Build/Products/Debug/TubeCity.app
```

### Check Build Logs
Build logs are saved when output is large. Look for:
```
~/.claude/projects/-Users-taylorpage-Repos-TyAu/*/tool-results/
```

### Clean Build
```bash
cd /Users/taylorpage/Repos/TyAu/TyAu-Pedals/TyAu-TubeCity
xcodebuild -project TubeCity.xcodeproj -scheme TubeCity clean
```

## Design Decisions

### UI Design
- Knob rotation: 7 o'clock to 5 o'clock (270° range)
- Start angle: -135° (bottom-left)
- End angle: 135° (bottom-right)
- Dead zone: Bottom position (typical audio equipment style)

### Signal Processing
See [SignalProcessing.md](SignalProcessing.md) for complete DSP documentation.

## Troubleshooting

### Logic Not Seeing Updates
1. Ensure you're building Debug configuration (not Release)
2. Check plugin registration: `pluginkit -m -v | grep TubeCity`
3. Try restarting Logic Pro
4. Clear audio unit cache: `killall -9 AudioComponentRegistrar`

### Build Errors
1. Check Xcode version compatibility
2. Verify code signing certificate is valid
3. Clean build folder and rebuild

### Plugin Crashes Logic
1. Check Console.app for crash logs
2. Look for Audio Unit validation errors
3. Verify DSP code doesn't have memory issues

## Development History

### Tube Visualization Implementation (February 2026)
We implemented signal-responsive vacuum tube visualizations:
1. Added three glowing vacuum tubes with animated filaments
2. Tubes respond to audio signal intensity in real-time
3. Each tube style (Classic, Asymmetric, Warm) has distinct visual characteristics
4. Added PNG tube graphics with transparency support

---

## Notes

- **CRITICAL:** Always rebuild and re-register after ANY code change
- Always build Debug configuration during development
- Logic Pro loads plugins from DerivedData during development
- The plugin needs to be properly signed to load in Logic
- Changes to DSP code typically require Logic restart
- UI changes DO NOT hot-reload - you must rebuild and re-register
- Opening the TubeCity.app after building ensures proper plugin registration

---

**Last Updated:** February 5, 2026
**Project Version:** 1.0
**Xcode Version:** 17C52
**macOS Version:** Sequoia 15.7

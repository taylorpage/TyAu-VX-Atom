# VX-ATOM Build and Development Guide

## Project Location

**Repository Root:** `/Users/taylorpage/Repos/TyAu/Compressor/TyAu-VX-Atom/`
**Xcode Project:** `VX-Atom.xcodeproj`
**Scheme:** `VX-Atom`

---

## Quick Start

```bash
cd /Users/taylorpage/Repos/TyAu/Compressor/TyAu-VX-Atom
./build.sh
```

Expected output:
```
** BUILD SUCCEEDED **
✅ Build succeeded!
📝 Registering Audio Unit extension...
🎸 VX-Atom is ready! Load it in Logic Pro.
```

---

## Key Configuration Values

| Setting | Value |
|---------|-------|
| AU Type | `aufx` (effect) |
| AU Subtype | `vxat` |
| Manufacturer | `TyAu` |
| Host bundle ID | `com.taylor.audio.VX-Atom` |
| Extension bundle ID | `com.taylor.audio.VX-Atom.VX-AtomExtension` |
| Deployment target | macOS 15.7 |
| C++ standard | C++20 |
| Swift version | 5.0 |

---

## Building

### Command Line (Recommended)

```bash
xcodebuild -project /Users/taylorpage/Repos/TyAu/Compressor/TyAu-VX-Atom/VX-Atom.xcodeproj \
  -scheme VX-Atom build -allowProvisioningUpdates
```

Or use the wrapper script (also opens and registers the host app):

```bash
./build.sh
```

### After Building — Register the AU

Always open the host app after a build. This registers the Audio Unit extension with the system:

```bash
open /Users/taylorpage/Library/Developer/Xcode/DerivedData/VX-Atom-*/Build/Products/Debug/VX-Atom.app
```

---

## Development Workflow

### Standard Cycle (After Every Code Change)

1. Make your code change
2. Build:
   ```bash
   ./build.sh
   ```
3. The script opens the host app automatically to re-register
4. Logic Pro will reload the updated binary (or prompt to reload)

### Testing in Logic Pro

1. Build and run `./build.sh`
2. Launch Logic Pro — it rescans on startup
3. Plugin appears under **Audio FX → Taylor Audio → VX-Atom**

If Logic doesn't see it after opening the host app:

```bash
# Force AU system rescan
killall -9 AudioComponentRegistrar

# Clear Logic's AU cache
rm -rf ~/Library/Caches/AudioUnitCache
rm -f ~/Library/Caches/com.apple.audio.InfoPlist_Cache.plist

# Then quit and relaunch Logic Pro
```

---

## Verifying Registration

Check if the extension is registered:

```bash
pluginkit -mA -p com.apple.AudioUnit-UI | grep -i vx-atom
```

Check if the AU is visible to the audio system:

```bash
auval -a | grep -i "TyAu"
```

Run full validation:

```bash
auval -v aufx vxat TyAu
```

Expected: `AU VALIDATION SUCCEEDED.`

---

## Project Structure

```
TyAu-VX-Atom/
├── build.sh                             # Build + register script
├── CONTRIBUTING.md                      # Commit message standards
├── BUILD_AND_DEVELOPMENT.md             # This file
├── AU_PLUGIN_CREATION_GUIDE.md          # Guide for creating new plugins from template
│
├── VX-Atom/                             # Host app (for testing the AU)
│   ├── VX-AtomApp.swift
│   ├── ContentView.swift
│   └── Model/
│       └── AudioUnitHostModel.swift     # AU type/subtype/manufacturer — must match Info.plist
│
└── VX-AtomExtension/                    # The Audio Unit plugin
    ├── Info.plist                        # AU registration (type, subtype, manufacturer, name)
    ├── VX-AtomExtension.entitlements    # App group
    │
    ├── Parameters/
    │   ├── VX-AtomExtensionParameterAddresses.h   ← C enum (shared by Swift + C++)
    │   └── Parameters.swift                        ← AUParameterTree specs
    │
    ├── DSP/
    │   └── VX-AtomExtensionDSPKernel.hpp           ← Compressor DSP engine (C++)
    │
    ├── UI/
    │   ├── VX-AtomExtensionMainView.swift          ← Nuclear aesthetic SwiftUI UI
    │   ├── ParameterKnob.swift                     ← Rotary knob component
    │   └── BypassButton.swift                      ← Stomp switch component
    │
    └── Common/                                      # Rarely needs editing
        ├── Audio Unit/
        │   └── VX-AtomExtensionAudioUnit.swift     ← AUAudioUnit subclass
        ├── UI/
        │   ├── AudioUnitViewController.swift
        │   └── ObservableAUParameter.swift
        ├── DSP/
        │   ├── VX-AtomExtensionAUProcessHelper.hpp
        │   └── VX-AtomExtensionBufferedAudioBus.hpp
        └── Parameters/
            └── ParameterSpecBase.swift
```

---

## Files You Will Edit

In most cases, only three files need changes when working on VX-ATOM:

| File | What to edit |
|------|-------------|
| `VX-AtomExtension/Parameters/VX-AtomExtensionParameterAddresses.h` | Add/rename parameter enum values |
| `VX-AtomExtension/Parameters/Parameters.swift` | Add/change ParameterSpec (range, default, units) |
| `VX-AtomExtension/DSP/VX-AtomExtensionDSPKernel.hpp` | DSP algorithm, setParameter/getParameter cases |
| `VX-AtomExtension/UI/VX-AtomExtensionMainView.swift` | UI layout, visual design |

The `Common/` directory is infrastructure and rarely needs changes.

---

## Critical Rules: Parameter Sync

The C header, Swift parameters, DSP kernel, and SwiftUI view must all use the same address integers. A mismatch causes silent bugs (wrong parameter gets set) or crashes.

**Dependency chain:**
```
ParameterAddresses.h  (C enum, raw values 0–N)
        ↓
Parameters.swift      (ParameterSpec array, uses address.rawValue)
        ↓
AudioUnitViewController.swift → setupParameterTree() → kernel.setParameter() for each
        ↓
DSPKernel.hpp         (switch on VXAtomExtensionParameterAddress cases)
        ↓
MainView.swift        (parameterTree.global.<identifier> dot notation)
```

When adding a parameter, always update **all four layers** before building.

---

## Troubleshooting

### Plugin doesn't appear in Logic

1. Confirm the AU is registered: `auval -a | grep TyAu`
2. Run validation: `auval -v aufx vxat TyAu`
3. Clear AU cache and restart Logic (see commands above)
4. Make sure the host app was opened after the build

### Host app loads but AU shows "failed to load"

- Check `subType` in `AudioUnitHostModel.swift` matches `subtype` in `Info.plist`
- These must be identical four-char strings: `vxat`

### Build fails with provisioning error

Always pass `-allowProvisioningUpdates` to xcodebuild:

```bash
xcodebuild ... build -allowProvisioningUpdates
```

### AU validates but sounds wrong / parameters don't work

- Check `VX-AtomExtensionParameterAddresses.h` — address integers must match order in `Parameters.swift`
- Check `VX-AtomExtensionDSPKernel.hpp` `setParameter()` / `getParameter()` switch cases cover every enum value

### VU meter not updating

The meter polls `gainReductionDB()` via a `gainReductionProvider` closure passed from `AudioUnitViewController`. If the meter is stuck at 0:
- Confirm `VXAtomExtensionAudioUnit.gainReductionDB()` is being called (add a print statement)
- Confirm `gainReductionProvider` is non-nil in the view (check `AudioUnitViewController.configureSwiftUIView`)
- Confirm `TimelineView` is firing (swap to a `Timer` if needed in non-animated host contexts)

---

**Last Updated:** February 2026

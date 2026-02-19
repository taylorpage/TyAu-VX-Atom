# TyAu-Template Build and Development Guide

## Project Location

**Repository Root:** `/Users/taylorpage/Repos/TyAu/TyAu-Template/`
**Xcode Project:** `Template.xcodeproj`
**Scheme:** `Template`

---

## Quick Start (First Time Setup)

When creating a new plugin from this template, do these steps in order:

### 1. Update the four-char codes in `TemplateExtension/Info.plist`

```xml
<key>subtype</key>
<string>tmpl</string>   <!-- Change this to your plugin's unique 4-char code -->

<key>manufacturer</key>
<string>TyAu</string>   <!-- Change to your manufacturer code -->
```

### 2. Update the matching values in `Template/Model/AudioUnitHostModel.swift`

```swift
init(type: String = "aufx", subType: String = "tmpl", manufacturer: String = "TyAu")
```

**Critical:** The `subType` and `manufacturer` here must exactly match `Info.plist`. A mismatch means the host app cannot find the registered AU.

### 3. Update app group in `TemplateExtension/TemplateExtension.entitlements`

```xml
<string>$(TeamIdentifierPrefix)com.taylor.audio.Template</string>
```

Replace `com.taylor.audio.Template` with your bundle ID. Must match the host app's bundle identifier.

### 4. Update bundle identifiers in Xcode

- **Host app:** `com.taylor.audio.Template` → `com.taylor.audio.YourPlugin`
- **Extension:** `com.taylor.audio.Template.TemplateExtension` → `com.taylor.audio.YourPlugin.YourPluginExtension`

### 5. Update the AU display name in `TemplateExtension/Info.plist`

```xml
<key>name</key>
<string>Taylor Audio: Template</string>   <!-- Change to your plugin name -->
```

---

## Building

### Command Line (Recommended for Development)

```bash
xcodebuild -project /Users/taylorpage/Repos/TyAu/TyAu-Template/Template.xcodeproj \
  -scheme Template build -allowProvisioningUpdates
```

### After Building — Register the AU

Always open the host app after a build. This registers the Audio Unit extension with the system:

```bash
open /Users/taylorpage/Library/Developer/Xcode/DerivedData/Template-*/Build/Products/Debug/Template.app
```

---

## Development Workflow

### Standard Cycle (After Every Code Change)

1. Make your code change
2. Build:
   ```bash
   xcodebuild -project Template.xcodeproj -scheme Template build -allowProvisioningUpdates
   ```
3. Open the host app to re-register:
   ```bash
   open ~/Library/Developer/Xcode/DerivedData/Template-*/Build/Products/Debug/Template.app
   ```
4. Logic Pro will reload the updated binary automatically (or may crash/prompt to reload)

### Testing in Logic Pro

1. Build and open the host app (see above)
2. Launch Logic Pro — it rescans on startup
3. The plugin appears under **Audio FX → Taylor Audio → Template**

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

Check if the extension is registered with pluginkit:

```bash
pluginkit -mA -p com.apple.AudioUnit-UI | grep -i template
```

Check if the AU is visible to the audio system:

```bash
auval -a | grep -i "TyAu"
```

Run full validation:

```bash
auval -v aufx tmpl TyAu
```

Replace `tmpl` and `TyAu` with your subtype and manufacturer codes.

---

## Project Structure

```
TyAu-Template/
├── Template/                        # Host app (for testing the AU)
│   ├── TemplateApp.swift
│   ├── ContentView.swift
│   ├── ValidationView.swift
│   └── Model/
│       ├── AudioUnitHostModel.swift  # AU type/subtype/manufacturer — must match Info.plist
│       └── AudioUnitViewModel.swift
│
└── TemplateExtension/               # The Audio Unit plugin
    ├── Info.plist                    # AU registration (type, subtype, manufacturer, name)
    ├── TemplateExtension.entitlements # App group must match host bundle ID
    ├── UI/
    │   ├── TemplateExtensionMainView.swift
    │   ├── ParameterKnob.swift
    │   └── BypassButton.swift
    ├── DSP/
    │   └── TemplateExtensionDSPKernel.hpp
    └── Parameters/
        ├── Parameters.swift
        └── TemplateExtensionParameterAddresses.h
```

---

## Key Configuration Values (Template Defaults)

| Setting | Value |
|---------|-------|
| AU Type | `aufx` (effect) |
| AU Subtype | `tmpl` |
| Manufacturer | `TyAu` |
| Host bundle ID | `com.taylor.audio.Template` |
| Extension bundle ID | `com.taylor.audio.Template.TemplateExtension` |
| Deployment target | macOS 15.7 |
| C++ standard | C++20 |
| Swift version | 5.0 |

---

## Troubleshooting

### Plugin doesn't appear in Logic

1. Confirm the AU is registered: `auval -a | grep TyAu`
2. Run validation: `auval -v aufx tmpl TyAu`
3. Clear AU cache and restart Logic (see commands above)
4. Make sure the host app was opened after the build

### Host app loads but AU shows "failed to load"

- Check that `subType` in `AudioUnitHostModel.swift` matches the `subtype` in `Info.plist`
- These must be identical four-char strings

### Build fails with provisioning error

Always pass `-allowProvisioningUpdates` to xcodebuild:
```bash
xcodebuild ... build -allowProvisioningUpdates
```

### AU validates but sounds wrong / parameters don't work

- Check `TemplateExtensionParameterAddresses.h` — addresses must match the order in `Parameters.swift`
- Check `TemplateExtensionDSPKernel.hpp` `setParameter()` switch cases match those addresses

---

**Last Updated:** February 2026

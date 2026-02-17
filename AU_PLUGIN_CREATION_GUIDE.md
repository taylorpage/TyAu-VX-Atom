# Audio Unit Plugin Creation Guide

This guide documents two methods for creating Audio Unit (AU) plugins on macOS:

1. **Recommended: Clone from TyAu-Template** - Fast, reliable, proven to work ✅
2. **Alternative: Create from Xcode** - Manual setup, requires bug fixes

## Prerequisites

- Xcode 16.2+ installed
- Valid Apple Developer certificate for code signing
- macOS development environment

---

# Method 1: Clone from TyAu-Template (Recommended)

**Time:** ~10-15 minutes | **Difficulty:** Easy

This is the recommended approach. TyAu-Template is a fully functional, validated AU plugin that you can clone and customize.

## Why Use the Template?

- ✅ **Already works** - Builds and validates immediately
- ✅ **Logic Pro compatible** - Mono (1-1) and stereo (2-2) support
- ✅ **No Xcode bugs** - All template issues pre-fixed
- ✅ **Simple baseline** - Basic gain plugin, easy to understand
- ✅ **Clean UI** - Professional design ready to customize

## Step-by-Step: Create Plugin from Template

### Step 1: Copy the Template

```bash
cd /Users/taylorpage/Developer/Projects/TyAu
cp -R TyAu-Template TyAu-YourPluginName
cd TyAu-YourPluginName
```

### Step 2: Rename Directories

```bash
# Rename the main directories
mv Template YourPluginName
mv TemplateExtension YourPluginNameExtension
mv Template.xcodeproj YourPluginName.xcodeproj
```

### Step 3: Rename All Files

Find and rename all files containing "Template":

```bash
# Find files to rename
find . -name "*Template*" -type f -not -path "./.git/*"

# Rename each file (repeat for each found file)
# Example:
mv YourPluginName/TemplateApp.swift YourPluginName/YourPluginNameApp.swift
mv YourPluginNameExtension/Common/Audio\ Unit/TemplateExtensionAudioUnit.swift \
   YourPluginNameExtension/Common/Audio\ Unit/YourPluginNameExtensionAudioUnit.swift
# ... continue for all files
```

**Files that need renaming:**
- `TemplateApp.swift` → `YourPluginNameApp.swift`
- `TemplateExtensionAudioUnit.swift` → `YourPluginNameExtensionAudioUnit.swift`
- `TemplateExtensionDSPKernel.hpp` → `YourPluginNameExtensionDSPKernel.hpp`
- `TemplateExtensionParameterAddresses.h` → `YourPluginNameExtensionParameterAddresses.h`
- `TemplateExtensionMainView.swift` → `YourPluginNameExtensionMainView.swift`
- `TemplateExtension-Bridging-Header.h` → `YourPluginNameExtension-Bridging-Header.h`

### Step 4: Replace Code References

Replace all "Template" references in code:

```bash
# Replace in Swift files
find . -name "*.swift" -type f -not -path "./.git/*" -exec sed -i '' 's/Template/YourPluginName/g' {} +

# Replace in C++ headers
find . -name "*.hpp" -o -name "*.h" -type f -not -path "./.git/*" -exec sed -i '' 's/Template/YourPluginName/g' {} +

# Replace in Info.plist
sed -i '' 's/Template/YourPluginName/g' YourPluginNameExtension/Info.plist

# Replace in build script
sed -i '' 's/Template/YourPluginName/g' build.sh

# Replace in Xcode project
sed -i '' 's/Template/YourPluginName/g' YourPluginName.xcodeproj/project.pbxproj
```

Also replace lowercase "template":

```bash
find . -type f \( -name "*.swift" -o -name "*.hpp" -o -name "*.h" \) -not -path "./.git/*" -exec sed -i '' 's/template/yourpluginname/g' {} +
```

### Step 5: Generate Unique Subtype Code

Create a unique 4-character code for your plugin:

**Rules:**
- Exactly 4 characters
- ASCII printable
- Must not conflict with existing plugins
- Avoid Apple reserved codes: `gain`, `dist`, `dely`, `revb`, `comp`, `filt`

**Examples:**
- "MyChorus" → `mych` or `chor`
- "SuperDelay" → `sdly` or `spdl`
- "VintageComp" → `vico` or `vncp`

### Step 6: Update Info.plist

Edit `YourPluginNameExtension/Info.plist`:

```xml
<key>description</key>
<string>YourPluginName</string>

<key>name</key>
<string>Taylor Audio: YourPluginName</string>

<key>subtype</key>
<string>yourSubtypeCode</string>  <!-- e.g., "mych" -->

<!-- Keep these unchanged: -->
<key>manufacturer</key>
<string>TyAu</string>

<key>manufacturerCode</key>
<integer>1954115685</integer>
```

### Step 7: Customize Parameters

Edit `YourPluginNameExtension/Parameters/YourPluginNameExtensionParameterAddresses.h`:

```cpp
typedef NS_ENUM(AUParameterAddress, YourPluginNameExtensionParameterAddress) {
    yourParameter1 = 0,
    yourParameter2 = 1,
    bypass = 2
};
```

Edit `YourPluginNameExtension/Parameters/Parameters.swift`:

```swift
let YourPluginNameExtensionParameterSpecs = ParameterTreeSpec {
    ParameterGroupSpec(identifier: "global", name: "Global") {
        ParameterSpec(
            address: .yourParameter1,
            identifier: "yourParameter1",
            name: "Your Parameter 1",
            units: .linearGain,
            valueRange: 0.0...1.0,
            defaultValue: 0.5
        )
        // Add more parameters...
        ParameterSpec(
            address: .bypass,
            identifier: "bypass",
            name: "Bypass",
            units: .boolean,
            valueRange: 0.0...1.0,
            defaultValue: 0.0
        )
    }
}
```

### Step 8: Implement Your DSP Algorithm

Edit `YourPluginNameExtension/DSP/YourPluginNameExtensionDSPKernel.hpp`:

Add your member variables:

```cpp
private:
    float mYourParameter1 = 0.5f;
    // Add more state variables...
```

Update `setParameter()`:

```cpp
void setParameter(AUParameterAddress address, AUValue value) {
    switch (address) {
        case YourPluginNameExtensionParameterAddress::yourParameter1:
            mYourParameter1 = value;
            break;
        // Handle other parameters...
    }
}
```

Implement your processing in `process()`:

```cpp
void process(std::span<float const*> inputBuffers, std::span<float *> outputBuffers,
             AUEventSampleTime bufferStartTime, AUAudioFrameCount frameCount) {
    assert(inputBuffers.size() == outputBuffers.size());

    if (mBypassed) {
        for (UInt32 channel = 0; channel < inputBuffers.size(); ++channel) {
            std::copy_n(inputBuffers[channel], frameCount, outputBuffers[channel]);
        }
    } else {
        for (UInt32 channel = 0; channel < inputBuffers.size(); ++channel) {
            for (UInt32 frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
                float input = inputBuffers[channel][frameIndex];

                // YOUR DSP ALGORITHM HERE
                float output = input * mYourParameter1;  // Example: simple gain

                outputBuffers[channel][frameIndex] = output;
            }
        }
    }
}
```

### Step 9: Customize UI

Edit `YourPluginNameExtension/UI/YourPluginNameExtensionMainView.swift`:

```swift
struct YourPluginNameExtensionMainView: View {
    var parameterTree: ObservableAudioUnitParameterGroup

    var body: some View {
        ZStack {
            // Your UI design here
            LinearGradient(...)
                .cornerRadius(8)

            VStack(spacing: 20) {
                Text("YOUR PLUGIN NAME")
                    .font(.system(size: 20, weight: .bold))

                // Add your parameter controls
                ParameterKnob(param: parameterTree.global.yourParameter1, size: 120)

                BypassButton(param: parameterTree.global.bypass)

                Text("TaylorAudio")
                    .font(.caption)
                    .foregroundColor(.gray)
            }
        }
        .frame(width: 300, height: 450)
    }
}
```

### Step 10: Build and Test

```bash
./build.sh
```

Expected output: `** BUILD SUCCEEDED **`

### Step 11: Validate

```bash
# Check registration
auval -a | grep TyAu

# Full validation
auval -v aufx yourSubtypeCode TyAu
```

Expected: `AU VALIDATION SUCCEEDED.`

### Step 12: Test in Logic Pro

1. Open Logic Pro
2. Create/open a project
3. Add audio track
4. Navigate to: **Audio FX** > **Taylor Audio** > **YourPluginName**
5. Test all parameters

### Step 13: Install for Permanent Use (Optional)

For production use, copy to Applications:

```bash
cp -R ~/Library/Developer/Xcode/DerivedData/YourPluginName-*/Build/Products/Debug/YourPluginName.app ~/Applications/
```

Then re-register:

```bash
open ~/Applications/YourPluginName.app
killall -9 AudioComponentRegistrar
```

### Step 14: Commit to Git

```bash
git init
git add .
git commit -m "feat: create YourPluginName audio unit plugin

- Implement [your DSP algorithm]
- Add [parameter list]
- Create UI with [controls]
- Pass auval validation

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>"
```

---

# Method 2: Create from Xcode Template (Alternative)

**Time:** ~30-45 minutes | **Difficulty:** Medium

Use this method if you need to understand the complete setup from scratch or want full control over the initial configuration.

⚠️ **Note:** Xcode's template has several bugs that need manual fixing. TyAu-Template (Method 1) already has these fixes applied.

## Step 1: Create Project in Xcode

### Open Xcode and Create New Project

1. File → New → Project
2. Select **"Multiplatform"** tab at the top (NOT macOS!)
3. Choose **"Audio Unit Extension App"** template
4. Click Next

### Configure Project Settings

Fill in the following fields:

- **Product Name:** `TaylorGain` (or your plugin name)
- **Team:** Select your Apple Developer team from dropdown
- **Organization Identifier:** `com.taylor.audio`
- **Bundle Identifier:** Auto-fills as `com.taylor.audio.TaylorGain`
- **Audio Unit Type:** Select **"Effect"**
- **Subtype Code:** `tygn` (exactly 4 characters, identifies plugin type - **AVOID Apple reserved codes like `gain`**)
- **Manufacturer Code:** `TyAu` (exactly 4 characters, at least 1 uppercase, represents your brand)

### Save Project

- **Save location:** `/Users/taylorpage/Repos/TyAu-gain`
- Click Create

### Important Notes on Codes

- **Subtype Code:** Must be exactly 4 characters, identifies the plugin functionality
  - **CRITICAL:** Avoid Apple's reserved subtypes (`gain`, `dist`, `dely`, `revb`, etc.)
  - Use unique codes like `tygn` (Taylor Gain) to prevent Logic Pro from filtering your plugin
- **Manufacturer Code:** Must be exactly 4 characters with at least 1 uppercase letter
  - This code is converted to a FourCC integer in Info.plist as `manufacturerCode`
- The combination of Type + Subtype + Manufacturer must be globally unique
- Example codes used: Type=`aufx`, Subtype=`tygn`, Manufacturer=`TyAu`

## Step 2: Close Xcode and Switch to VSCode

Once the project is created:

1. **Close Xcode completely** (important!)
2. Open the project directory in VSCode for examination

## Step 3: Remove "Extension" Suffix from Plugin Name

**IMPORTANT:** By default, Xcode adds "Extension" to your plugin name in the Info.plist. You should remove this before building.

1. Open the project in your editor (VSCode recommended)
2. Navigate to `[ProjectName]Extension/Info.plist`
3. Find the `name` and `description` keys under `AudioComponents`
4. Remove "Extension" from both values:
   - Change: `Taylor Audio: TaylorGainExtension` → `Taylor Audio: TaylorGain`
   - Change: `TaylorGainExtension` → `TaylorGain` (in description field)

This ensures your plugin appears with a clean name in GarageBand and Logic Pro.

## Step 4: Verify Template Structure

The template creates this structure:

```
TaylorGain/
├── TaylorGain/                          # Host app for testing
│   ├── ContentView.swift                # Main UI
│   ├── AudioUnitHostModel.swift         # AU host logic
│   └── Common/                          # Shared utilities
└── TaylorGainExtension/                 # The actual AU plugin
    ├── Common/
    │   ├── Audio Unit/
    │   │   └── TaylorGainExtensionAudioUnit.swift    # Main AU class
    │   └── DSP/
    │       ├── TaylorGainExtensionDSPKernel.hpp      # C++ DSP processing
    │       ├── TaylorGainExtensionAUProcessHelper.hpp
    │       └── TaylorGainExtensionBufferedAudioBus.hpp
    ├── Parameters/
    │   ├── Parameters.swift                           # Parameter definitions
    │   └── TaylorGainExtensionParameterAddresses.h   # Parameter enum
    ├── DSP/
    │   └── TaylorGainExtensionDSPKernel.hpp          # Main DSP implementation
    └── UI/
        └── TaylorGainExtensionMainView.swift         # Plugin UI
```

### Key Files to Understand

**TaylorGainExtensionDSPKernel.hpp** - Core DSP processing (C++)
- Contains `process()` method for audio processing
- Sample-by-sample gain multiplication at line 113
- Parameter handling in `setParameter()` and `getParameter()`

**Parameters.swift** - Parameter definitions
- Defines plugin parameters (gain range, default value, units)
- Already includes a gain parameter (0.0 to 1.0)

**TaylorGainExtensionParameterAddresses.h** - Parameter enums
- Maps parameter names to addresses for C++ access

**TaylorGainExtensionAudioUnit.swift** - Swift wrapper
- Bridges Swift and C++ DSP kernel
- Handles AU lifecycle (init, allocate, deallocate resources)

## Step 5: Add Logic Pro Compatibility Settings

**CRITICAL for Logic Pro Support:** Before building, you must add mono channel support and additional Info.plist keys.

### Update AudioUnit.swift

Edit `[ProjectName]Extension/Common/Audio Unit/[ProjectName]ExtensionAudioUnit.swift`:

1. **Fix the input bus type** (Xcode template bug - line ~31):
   ```swift
   // Change from:
   _inputBusses = AUAudioUnitBusArray(audioUnit: self, busType: AUAudioUnitBusType.output, busses: [inputBus.bus!])

   // To:
   _inputBusses = AUAudioUnitBusArray(audioUnit: self, busType: AUAudioUnitBusType.input, busses: [inputBus.bus!])
   ```

2. **Increase maximum channel count** (line ~25):
   ```swift
   // Change from:
   outputBus?.maximumChannelCount = 2

   // To:
   outputBus?.maximumChannelCount = 8
   ```

3. **Add mono and stereo channel capabilities** (replace the `channelCapabilities` property around line ~47):
   ```swift
   public override var channelCapabilities: [NSNumber]? {
       // Explicitly declare mono and stereo support
       // Format: [inputChannels, outputChannels, inputChannels, outputChannels, ...]
       return [
           1, 1,  // Mono in, Mono out - REQUIRED for Logic Pro
           2, 2   // Stereo in, Stereo out
       ] as [NSNumber]
   }
   ```

### Update Info.plist

Edit `[ProjectName]Extension/Info.plist` and add these keys inside the AudioComponents dict (after the `manufacturer` key):

```xml
<key>manufacturerCode</key>
<integer>1954115685</integer>
<key>resourceUsage</key>
<dict>
    <key>network.client</key>
    <false/>
    <key>temporary-exception.files.all.read-write</key>
    <false/>
</dict>
```

**Note:** The `manufacturerCode` integer `1954115685` is the FourCC representation of `"TyAu"`.

## Step 6: Build the Plugin

From the project directory, build in Release configuration:

```bash
cd /Users/taylorpage/Repos/TyAu-gain/TaylorGain
xcodebuild -scheme TaylorGain -configuration Release clean build
```

Expected output: `** BUILD SUCCEEDED **`

The plugin will be built to:
```
/Users/taylorpage/Library/Developer/Xcode/DerivedData/TaylorGain-[hash]/Build/Products/Release/
```

## Step 7: Register the Plugin

The plugin needs to be registered with the system before it can be validated or used.

### Register by Opening the Host App

```bash
open "/Users/taylorpage/Library/Developer/Xcode/DerivedData/TaylorGain-[hash]/Build/Products/Release/TaylorGain.app"
```

This registers the plugin with macOS. You can close the app after it opens.

### Reset Audio Component Cache (if needed)

If the plugin doesn't appear or you're having registration issues:

```bash
killall -9 AudioComponentRegistrar
```

Wait 2-3 seconds for the cache to rebuild.

## Step 8: Verify Registration

Check that your plugin is registered:

```bash
auval -a | grep -i taylor
```

Expected output:
```
aufx tygn TyAu  -  Taylor Audio: TaylorGain
```

You can also use pluginkit to see more details:

```bash
pluginkit -m -v | grep -i gain
```

## Step 9: Validate with auval

Run Apple's Audio Unit validation tool:

```bash
auval -v aufx tygn TyAu
```

Expected output: `AU VALIDATION SUCCEEDED.`

This comprehensive test validates:
- Plugin loads correctly
- Parameter handling works
- Audio rendering functions properly
- Format negotiation is correct
- All required properties are implemented

## Step 10: Clean Up Old Plugins (if applicable)

If you have old versions of plugins that need removal:

### Remove old DerivedData builds

```bash
rm -rf /Users/taylorpage/Library/Developer/Xcode/DerivedData/[OldProjectName]-[hash]
```

### Clear AU cache

```bash
killall -9 AudioComponentRegistrar
```

### Verify cleanup

```bash
auval -a | grep -i [your-search-term]
```

## Template Features

The Xcode template provides a complete, working plugin with:

1. **Gain Processing** - Already implemented in DSP kernel
2. **Parameter System** - Full AU parameter tree implementation
3. **SwiftUI Interface** - Modern UI with parameter controls
4. **Host App** - Complete test harness for development
5. **C++ DSP Kernel** - Efficient real-time audio processing
6. **Bypass Handling** - Built-in bypass functionality
7. **Multi-channel Support** - Stereo (2-2) by default
   - **NOTE:** Logic Pro requires mono (1-1) support to display plugins in Audio FX menu (see Critical Logic Pro Requirements below)

## Customization for Other Plugin Types

To create different plugin types, modify:

1. **Parameters.swift** - Add/modify parameters
2. **TaylorGainExtensionParameterAddresses.h** - Add parameter enums
3. **TaylorGainExtensionDSPKernel.hpp** - Implement your DSP algorithm

Example for a compressor:
- Add parameters: threshold, ratio, attack, release, makeup gain
- Implement compression algorithm in `process()` method
- Update parameter handling in `setParameter()` / `getParameter()`

## Critical Logic Pro Requirements

Logic Pro has stricter requirements than GarageBand. Your plugin MUST meet these to appear in Logic's Audio FX menu.

**NOTE:** If you followed Steps 3-5 above, these requirements are already satisfied. This section provides additional context and troubleshooting information.

### 1. Channel Configuration Support

Logic Pro **requires** plugins to support mono (1-1) configuration. This should already be added in Step 5, but here's the reference:

```swift
public override var channelCapabilities: [NSNumber]? {
    // Explicitly declare mono and stereo support
    // Format: [inputChannels, outputChannels, inputChannels, outputChannels, ...]
    return [
        1, 1,  // Mono in, Mono out - REQUIRED for Logic Pro
        2, 2   // Stereo in, Stereo out
    ] as [NSNumber]
}
```

Also ensure maximum channel counts are set properly in the init method:

```swift
outputBus?.maximumChannelCount = 8
inputBus.initialize(format, 8)
```

### 2. Correct Bus Types

Ensure input and output busses use the correct types:

```swift
_inputBusses = AUAudioUnitBusArray(audioUnit: self, busType: .input, busses: [inputBus.bus!])
_outputBusses = AUAudioUnitBusArray(audioUnit: self, busType: .output, busses: [outputBus!])
```

### 3. Info.plist Requirements

Your `Info.plist` must include:

```xml
<key>manufacturerCode</key>
<integer>1954115685</integer>  <!-- "TyAu" as FourCC integer -->
<key>resourceUsage</key>
<dict>
    <key>network.client</key>
    <false/>
    <key>temporary-exception.files.all.read-write</key>
    <false/>
</dict>
```

### 4. Avoid Apple's Reserved Subtype Codes

**Never use these common subtypes** - Logic Pro filters them out:
- `gain` - Reserved by Apple
- `dist` - Distortion
- `dely` - Delay
- `revb` - Reverb
- `comp` - Compressor
- `filt` - Filter

Use unique codes like `tygn` (Taylor Gain) instead.

### 5. Clear All Caches for Logic Pro

Logic Pro maintains multiple cache layers. To force a complete rescan:

```bash
# Kill all audio services
killall -9 "Logic Pro" AudioComponentRegistrar coreaudiod

# Clear the CRITICAL AudioComponentCache
rm ~/Library/Preferences/com.apple.audio.AudioComponentCache.plist

# Clear other AU caches
rm -rf ~/Library/Caches/AudioUnitCache/*
rm -rf ~/Library/Caches/com.apple.logic10

# Clear Logic Pro defaults for your plugin
defaults delete com.apple.logic10 "aufx-[subtype]-[manufacturer]"

# Re-register plugin
open ~/Applications/YourPlugin.app
# Close app after it opens

# Verify registration
auval -v aufx tygn TyAu
```

**Now launch Logic Pro** - it will perform a full AU scan and your plugin should appear in Audio FX menu.

## Common Issues and Solutions

### "Next" button disabled in Xcode
- Ensure Team is selected (not "None")
- Subtype Code must be exactly 4 characters
- Manufacturer Code must be exactly 4 characters with at least 1 uppercase

### Plugin appears in GarageBand but NOT Logic Pro
- **Root cause:** Logic Pro requires mono (1-1) channel support
- **Solution:** Add explicit `channelCapabilities` returning `[1, 1, 2, 2]`
- Clear `com.apple.audio.AudioComponentCache.plist` to force rescan
- Verify with `auval -v` that "1-1" appears in channel handling

### Plugin not appearing after build
- Open the host app to trigger registration
- Kill AudioComponentRegistrar to refresh cache
- Verify the plugin is in DerivedData/Build/Products/Release/
- For Logic Pro specifically, delete AudioComponentCache.plist

### auval warnings about channel configurations
- Template defaults to 2-2 (stereo in/out) only
- Logic Pro requires 1-1 (mono) support
- Add explicit `channelCapabilities` as shown above

### Old plugins conflicting or stale cache
- Remove old DerivedData folders
- **Delete** `~/Library/Preferences/com.apple.audio.AudioComponentCache.plist`
- Clear AU cache with `killall -9 AudioComponentRegistrar`
- Use unique manufacturer codes for different versions

### Logic Pro doesn't auto-scan on startup
- This is normal behavior
- Logic only scans when cache is missing or manually triggered
- Go to Logic Pro > Plug-in Manager > Reset & Rescan Selection
- Or delete AudioComponentCache.plist to force automatic scan

### Multiple plugins from same manufacturer not appearing
**Symptom:** You have multiple plugins with the same manufacturer code (e.g., TyAu) but only one appears in `auval -a`

**Root cause:** macOS's Audio Component cache can get confused when multiple plugins from the same manufacturer are only in DerivedData build folders.

**Solution:** Copy production plugins to Applications:
```bash
# Move stable/production plugins to Applications
cp -R ~/Library/Developer/Xcode/DerivedData/YourPlugin-*/Build/Products/Debug/YourPlugin.app ~/Applications/

# Keep development plugins in DerivedData for active work
# Re-register both
open ~/Applications/YourPlugin.app
killall -9 AudioComponentRegistrar
```

**Recommended setup:**
- **Production plugins:** `~/Applications/` (permanent)
- **Development plugins:** DerivedData (active work)

Both will then register correctly:
```bash
$ auval -a | grep TyAu
aufx tbcy TyAu  -  Taylor Audio: TubeCity
aufx tmpl TyAu  -  Taylor Audio: Template
```

## Production Deployment

For distributing your plugin to users:

1. Build in Release configuration
2. Code sign with Distribution certificate
3. Copy `.appex` bundle to:
   - System: `/Library/Audio/Plug-Ins/Components/`
   - User: `~/Library/Audio/Plug-Ins/Components/`
4. Users may need to reset AU cache after installation

## Reference Files

Key files in this project:
- [TaylorGainExtensionDSPKernel.hpp](TaylorGain/TaylorGainExtension/DSP/TaylorGainExtensionDSPKernel.hpp) - DSP implementation
- [Parameters.swift](TaylorGain/TaylorGainExtension/Parameters/Parameters.swift) - Parameter definitions
- [TaylorGainExtensionAudioUnit.swift](TaylorGain/TaylorGainExtension/Common/Audio%20Unit/TaylorGainExtensionAudioUnit.swift) - Main AU class

## Summary Checklist

- [ ] Create project in Xcode using "Audio Unit Extension App" template
- [ ] Fill in all required fields with **unique** Subtype code (avoid Apple's reserved codes)
- [ ] Close Xcode
- [ ] **Remove "Extension" suffix** from plugin name and description in Info.plist
- [ ] **Fix input bus type** from `.output` to `.input` in AudioUnit.swift
- [ ] **Increase max channel count** to 8 in AudioUnit.swift
- [ ] **Add mono support** to `channelCapabilities` in AudioUnit.swift
- [ ] **Add `manufacturerCode`** integer to Info.plist
- [ ] **Add `resourceUsage`** dictionary to Info.plist
- [ ] Build with `xcodebuild -scheme [Name] -configuration Release clean build`
- [ ] Copy app to `~/Applications/` for proper registration
- [ ] Open host app to register plugin
- [ ] Verify with `auval -a | grep [manufacturer]`
- [ ] Validate with `auval -v aufx [subtype] [manufacturer]`
- [ ] Confirm "AU VALIDATION SUCCEEDED" **and** "1-1" in Channel Handling
- [ ] For Logic Pro: Clear `com.apple.audio.AudioComponentCache.plist`
- [ ] Launch Logic Pro and verify plugin appears in Audio FX menu

Your plugin is now ready for development and fully compatible with Logic Pro!

---

## Quick Reference: Template-Based Workflow

**For creating a new plugin from TyAu-Template:**

```bash
# 1. Clone template
cd /Users/taylorpage/Developer/Projects/TyAu
cp -R TyAu-Template TyAu-YourPlugin

# 2. Rename directories
cd TyAu-YourPlugin
mv Template YourPlugin
mv TemplateExtension YourPluginExtension
mv Template.xcodeproj YourPlugin.xcodeproj

# 3. Rename all files with "Template" in name (manually or script)
# Use find command to locate: find . -name "*Template*" -type f

# 4. Replace code references
find . -name "*.swift" -type f -exec sed -i '' 's/Template/YourPlugin/g' {} +
find . \( -name "*.hpp" -o -name "*.h" \) -type f -exec sed -i '' 's/Template/YourPlugin/g' {} +
sed -i '' 's/Template/YourPlugin/g' YourPluginExtension/Info.plist
sed -i '' 's/Template/YourPlugin/g' build.sh
sed -i '' 's/Template/YourPlugin/g' YourPlugin.xcodeproj/project.pbxproj

# 5. Update Info.plist - Generate unique subtype code (4 chars)
# Edit YourPluginExtension/Info.plist:
#   <key>subtype</key>
#   <string>your</string>  <!-- unique 4-char code -->
#   <key>name</key>
#   <string>Taylor Audio: YourPlugin</string>

# 6. Customize parameters
# Edit YourPluginExtension/Parameters/YourPluginExtensionParameterAddresses.h
# Edit YourPluginExtension/Parameters/Parameters.swift

# 7. Implement DSP
# Edit YourPluginExtension/DSP/YourPluginExtensionDSPKernel.hpp

# 8. Customize UI
# Edit YourPluginExtension/UI/YourPluginExtensionMainView.swift

# 9. Build & validate
./build.sh
auval -v aufx your TyAu

# 10. Test in Logic Pro
# Audio FX > Taylor Audio > YourPlugin

# 11. For permanent installation
cp -R ~/Library/Developer/Xcode/DerivedData/YourPlugin-*/Build/Products/Debug/YourPlugin.app ~/Applications/
open ~/Applications/YourPlugin.app
killall -9 AudioComponentRegistrar
```

**Total time:** 10-15 minutes for a working plugin! 🎸

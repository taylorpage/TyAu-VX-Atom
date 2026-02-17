# TyAu-TubeCity Implementation Summary

## Overview
TyAu-TubeCity is a tube saturation/distortion audio unit plugin featuring a realistic vacuum tube visual element that responds to audio signal levels in real-time.

## Features Implemented

### 1. Vacuum Tube Visual Component
**File:** `TubeCityExtension/UI/VacuumTube.swift`

A realistic vacuum tube visualization with the following elements:
- **Metal base** with pin details
- **Glass envelope** with semi-transparent gradients and edge highlighting
- **Internal components**: anode (top plate), two grids, and cathode
- **Dynamic orange/amber glow** that intensifies based on signal level (0.3-1.0 opacity range)
- **Glass reflection highlights** for photorealistic appearance
- **Bypass integration**: tube goes dark when effect is bypassed

**Visual Behavior:**
- Glow intensity scales with audio signal level
- Fast attack, slow decay for natural response
- Completely dark when bypassed

### 2. Real-Time Signal Level Metering
**Files Modified:**
- `TubeCityExtension/DSP/TubeCityExtensionDSPKernel.hpp`
- `TubeCityExtension/Common/Audio Unit/TubeCityExtensionAudioUnit.swift`

**DSP Kernel (C++):**
- Added `mSignalLevel` member variable for peak tracking
- Implemented peak detection with:
  - Fast attack: instantly tracks peaks
  - Slow decay: 0.9995 coefficient per sample
- Resets to 0.0 when bypassed
- Thread-safe reading via `getSignalLevel()` method

**Audio Unit (Swift):**
- Added 30 Hz timer (`meterUpdateTimer`) for UI updates
- Timer starts when render resources are allocated
- Timer stops when resources are deallocated
- Updates signal level parameter automatically

### 3. Signal Level Parameter
**Files Modified:**
- `TubeCityExtension/Parameters/TubeCityExtensionParameterAddresses.h`
- `TubeCityExtension/Parameters/Parameters.swift`

**Parameter Definition:**
- Address: 5
- Identifier: `signallevel`
- Type: Linear Gain (0.0-1.0 range)
- Flags: Read-only (`.flag_IsReadable`)
- Default: 0.0

### 4. Output Volume Control
**All files modified for complete parameter integration**

**Purpose:**
Provides user-controllable master output level to compensate for tube saturation losses.

**Parameter Details:**
- Address: 6
- Identifier: `outputvolume`
- Range: 0.0-2.0
- Default: 1.0 (unity gain)
- Units: Linear Gain

**UI Implementation:**
- Large prominent knob (60px diameter)
- Positioned above tube selection knobs
- Labeled "VOLUME"

### 5. UI Layout Updates
**File:** `TubeCityExtension/UI/TubeCityExtensionMainView.swift`

**Layout Hierarchy (top to bottom):**
1. Vacuum tube visual (50x100px)
2. LED indicator (green when active, grey when bypassed)
3. Output Volume knob (60px diameter) - **NEW**
4. Three tube saturation knobs (45px diameter each):
   - Neutral
   - Warm
   - Aggressive
5. Bypass stomp switch
6. TaylorAudio logo

**ParameterKnob Enhancement:**
- Added optional `size` parameter (defaults to 140px)
- Scale radius now proportional to knob size
- Maintains backward compatibility

## Signal Processing Chain

```
Input
  ↓
Pre-EQ (75Hz high-pass filter)
  ↓
Tube Gain (multiplier)
  ↓
4x Oversampling
  ↓
Hard Clipping (asymmetric)
  ↓
Downsample (with DC blocker)
  ↓
Tube Saturation Processors (parallel mix):
  • Neutral Tube (drive: 1.5x, output: 0.92x)
  • Warm Tube (drive: 5.0x, output: 0.65x)
  • Aggressive Tube (drive: 9.0x, output: 0.45x)
  ↓
Makeup Gain (2.5x fixed) ← **CRITICAL FOR VOLUME**
  ↓
Output Volume (user control: 0.0-2.0x)
  ↓
Signal Level Meter (peak detection)
  ↓
Output
```

## Audio Level Compensation

### The Problem
Tube saturation processors have low output gains (0.45-0.92x), combined with hard clipping, which severely reduces signal levels.

### The Solution
Fixed makeup gain of **2.5x** applied after tube processing:

**File:** `TubeCityExtension/DSP/TubeCityExtensionDSPKernel.hpp:276-279`
```cpp
float makeupGain = 2.5f;  // Strong compensation for tube saturation losses
float output = tubeProcessed * makeupGain * mOutputVolume;
```

### Expected Signal Levels
With Output Volume at 1.0:
- **Neutral Tube (max):** ~0.9-1.0 (near unity)
- **Warm Tube (max):** ~0.6-0.7 (reduced but audible)
- **Aggressive Tube (max):** ~0.4-0.5 (heavily compressed)

With Output Volume at 2.0:
- Can boost up to 2x for hot signals

## Expected Sonic Behavior

### Neutral Tube (Cranked)
- Subtle saturation with light harmonic coloring
- Minimal obvious distortion
- Good for "always-on" tone enhancement
- Drive: 1.5x (mild)

### Warm Tube (Cranked)
- **Heavy saturation** with obvious warmth
- Clear harmonic distortion
- Classic "tube amp" overdrive sound
- Drive: 5.0x (strong)

### Aggressive Tube (Cranked)
- **Extreme saturation** with heavy distortion
- Compressed, fuzzy character
- Lots of harmonics
- Drive: 9.0x (very aggressive)

**Note:** All tubes produce audible distortion when cranked. If you hear clean audio, check parameter bindings.

## Technical Details

### Threading & Performance
- **DSP Kernel:** Real-time audio thread (C++)
- **Signal Level Updates:** 30 Hz timer on main thread (Swift)
- **Peak Tracking:** Per-sample in audio callback
- **Meter Decay:** Smooth exponential decay (0.9995 coefficient)

### Parameter Synchronization
1. User adjusts knob in UI
2. `ObservableAUParameter` updates value
3. Audio unit's `implementorValueObserver` callback fires
4. Kernel's `setParameter()` receives new value
5. Audio processing uses updated parameter

### Signal Level Flow
1. DSP kernel tracks peak level per sample
2. 30 Hz timer reads level from kernel
3. Timer updates signal level parameter
4. UI observes parameter changes
5. VacuumTube component re-renders with new glow intensity

## File Structure

```
TyAu-TubeCity/
├── TubeCityExtension/
│   ├── UI/
│   │   ├── VacuumTube.swift                    [NEW - Tube visual]
│   │   ├── TubeCityExtensionMainView.swift     [MODIFIED - Layout]
│   │   └── ParameterKnob.swift                 [MODIFIED - Size param]
│   ├── DSP/
│   │   └── TubeCityExtensionDSPKernel.hpp      [MODIFIED - Metering + volume]
│   ├── Parameters/
│   │   ├── Parameters.swift                     [MODIFIED - New params]
│   │   └── TubeCityExtensionParameterAddresses.h [MODIFIED - Addresses]
│   └── Common/
│       └── Audio Unit/
│           └── TubeCityExtensionAudioUnit.swift [MODIFIED - Timer]
└── IMPLEMENTATION.md                            [NEW - This file]
```

## Usage Instructions

### For Users
1. **Set Output Volume** to 0.7-1.0 (middle position)
2. **Turn up a tube knob** (try Warm at 0.5 first)
3. **Listen for distortion** - should be clearly audible
4. **Watch the tube glow** - should brighten with signal
5. **Adjust Output Volume** to taste
6. **Hit bypass** to compare - tube should go dark

### For Developers
- Signal level parameter is **read-only** - don't allow user writes
- Makeup gain is **fixed at 2.5x** - adjust if needed
- Timer runs at **30 Hz** - sufficient for visual feedback
- Peak detector has **fast attack, slow decay** - typical for meters

## Known Characteristics

### Distortion Behavior
✅ **Expected:** Audible harmonic distortion when tubes are cranked
✅ **Expected:** Heavy compression and saturation
✅ **Expected:** Reduced dynamic range with aggressive settings

❌ **Unexpected:** Clean audio with tubes at max
❌ **Unexpected:** No volume output
❌ **Unexpected:** Tube not glowing with signal present

### Volume Levels
- With all tubes at 0.0: Near unity gain (clean signal)
- With one tube cranked: Reduced level compensated by 2.5x makeup gain
- With Output Volume: Can boost ±2x from makeup-compensated level

## Build Information

**Platform:** macOS (Audio Unit v3)
**Language:** Swift (UI/Parameter system) + C++ (DSP)
**Framework:** AudioToolbox, SwiftUI
**Format:** Stereo/Mono Audio Unit Extension

**Build Status:** ✅ Succeeded
**Last Build:** 2026-02-04

## Future Enhancements (Potential)

- [ ] Add input gain control
- [ ] Expose Tube Gain parameter in UI
- [ ] Add tone controls (EQ)
- [ ] Implement preset system
- [ ] Add dry/wet mix control
- [ ] Alternative tube glow colors/styles

---

**Created:** February 4, 2026
**Author:** Claude Code Assistant
**Version:** 1.0

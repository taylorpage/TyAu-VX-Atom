# TubeCity — Tube Glow Behavior

## Overview

The tube glow visualization mimics the behavior of a real incandescent vacuum tube filament. Instead of instantly responding to signal peaks, it has smooth attack/release curves to create an organic, ambient visual effect.

---

## Signal Flow

```
Audio Output → DSP Metering → 30 Hz Timer → SwiftUI Parameters → Visual Glow
```

**DSP Kernel** (C++)
Two peak detectors run per-sample in `TubeCityExtensionDSPKernel.hpp`:

1. **`mSignalLevel`** — Main glow brightness
2. **`mFlickerLevel`** — Yellow overlay flash intensity

**AU Timer** (Swift)
A 30 Hz timer in `TubeCityExtensionAudioUnit.swift` pushes meter values to the parameter tree using `setValue(originator: nil)`.

**SwiftUI** (`TubeCityExtensionMainView.swift`)
The view reads `signalLevelParam.value` and `flickerLevelParam.value`, applies curves, and renders:
- Two radial gradient ellipses (outer + inner glow)
- Base tube image (constant brightness)
- Yellow flicker overlay (transient flash)

---

## Current Metering Behavior

### Main Glow (`mSignalLevel`)

**Purpose:** Drives overall tube brightness. Stays lit for a long time after signal stops, like a cooling filament.

| Property | Value | Result |
|----------|-------|--------|
| **Attack** | `0.05f` blend per sample | ~200ms smooth ramp-up when signal arrives |
| **Release/Decay** | `0.99995f` multiplier per sample | ~40 second gradual fade to darkness |
| **UI Curve** | `sqrt` + 15% floor | Quiet signals lifted into visible range; tube never fully dark until well after signal stops |

### Flicker Overlay (`mFlickerLevel`)

**Purpose:** Adds transient "pop" flashes on peaks for visual interest. Fades faster than main glow.

| Property | Value | Result |
|----------|-------|--------|
| **Attack** | `0.1f` blend per sample | ~100ms smooth ramp-up |
| **Release/Decay** | `0.9991f` multiplier per sample | ~4 second gradual fade |
| **UI Curve** | `0.9x` multiplier, clamped to 1.0 | Subtle yellow overlay, not jarring |

---

## Why These Timings

### Attack (Smooth Ramp-Up)

Real tube filaments take time to heat up. Instant peak-detection looks robotic. The blended attack gives the glow a "warm-up" feel.

### Release (Very Slow Decay)

Tubes don't instantly cool down when signal stops. The extremely slow decay (40 seconds for main glow, 4 seconds for flicker) creates an ambient effect where the tube stays lit long after you stop playing.

### 15% UI Floor + sqrt Curve

The sqrt curve lifts quiet signals into visible range (e.g., signal at 0.04 → visible glow at 0.2). The 15% floor ensures the tube never fully blacks out until signal has been silent for several seconds, mimicking the residual heat in a real filament.

---

## Visual Composition

The glow is built from layers in `tubeVisualization`:

1. **Outer glow** (wide, soft) — yellow radial gradient, opacity scales with `level * 0.5`
2. **Inner glow** (tight, bright) — yellow-white radial gradient, opacity scales with `level * 0.75`
3. **Base tube image** — constant brightness, no signal-driven changes
4. **Flicker overlay** — yellow-tinted tube image, opacity driven by fast-decay `flicker` value

The result: a smooth, organic glow that breathes with the music and lingers after it stops.

---

## Key Implementation Details

### Timer Must Run on Main Run Loop

`allocateRenderResources()` can be called on a background thread. If the 30 Hz timer is scheduled on a background run loop that never runs, the meter will never update. The fix:

```swift
DispatchQueue.main.async {
    self.meterUpdateTimer = Timer.scheduledTimer(...)
}
```

### `originator: nil` for Meter Updates

AU's `setValue` with a non-nil originator suppresses callbacks to that same token (feedback prevention). Meter updates must use `originator: nil` so `ObservableAUParameter`'s observer fires and pushes the value into SwiftUI.

### Bypass Does Not Kill Meters

The render block always runs (even when bypassed) so meters can track passthrough signal. The kernel's `if (mBypassed)` branch just copies input→output instead of running the DSP chain, but meters still update on the output afterward.

---

## Tuning Parameters

If you want to adjust the glow behavior:

| Change | File | Line | Coefficient |
|--------|------|------|-------------|
| Main glow attack speed | `TubeCityExtensionDSPKernel.hpp` | ~293 | `0.05f` (lower = slower) |
| Main glow decay speed | `TubeCityExtensionDSPKernel.hpp` | ~295 | `0.99995f` (closer to 1.0 = slower) |
| Flicker attack speed | `TubeCityExtensionDSPKernel.hpp` | ~301 | `0.1f` (lower = slower) |
| Flicker decay speed | `TubeCityExtensionDSPKernel.hpp` | ~303 | `0.9991f` (closer to 1.0 = slower) |
| UI brightness floor | `TubeCityExtensionMainView.swift` | ~199 | `0.15` (percentage) |
| UI curve | `TubeCityExtensionMainView.swift` | ~199 | `pow(rawLevel, 0.5)` (sqrt) |
| Flicker intensity | `TubeCityExtensionMainView.swift` | ~202 | `0.9x` multiplier |

---

## Summary

The tube glow is designed to feel **organic and ambient**, not clinical or precise. It warms up slowly, stays lit during signal, and cools down very gradually — just like a real vacuum tube filament. The flicker overlay adds transient visual interest without being jarring.

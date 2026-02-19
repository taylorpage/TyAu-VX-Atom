# VX-ATOM — Character Compressor

VX-ATOM is a character compressor Audio Unit (AUv3) plugin for macOS. It is designed to impose a signature sound — hard, aggressive compression with harmonic saturation — inspired by the JJP Vocal Compressor's approach to dynamics. Unlike a utility compressor (which aims for transparency), VX-ATOM is opinionated by design.

The visual aesthetic is nuclear-era fallout: worn teal-green metal panel, vintage VU meter, distressed typography, and a radioactive motif.

---

## Plugin Type

| Property | Value |
|----------|-------|
| AU Type | `aufx` (Effect) |
| Subtype | `vxat` |
| Manufacturer | `TyAu` |
| Bundle ID | `com.taylor.audio.VX-AtomExtension` |
| Platform | macOS 15.7+ |
| Channel Support | 1-1 (mono), 2-2 (stereo) |

---

## Controls

VX-ATOM has **5 knobs + bypass**. Every control is deliberately minimal — the plugin is designed to have a sound, not to be a toolbox.

### SQUEEZE `0.0 – 10.0` · default `5.0`

The core character dial. A single knob that drives all compression intensity parameters simultaneously:

| SQUEEZE value | Threshold | Ratio | Knee |
|---------------|-----------|-------|------|
| 0 (LIGHT) | −6 dB | 2:1 | 8 dB (very soft) |
| 3–4 (THHKY) | −18 dB | 5:1 | 4 dB (soft) |
| 6–7 (RADIO) | −24 dB | 8:1 | 2 dB (firm) |
| 10 (TOO MUCH) | −30 dB | 12:1 | 0 dB (hard) |

Automatic makeup gain is baked into the SQUEEZE mapping — the more you compress, the more gain is restored automatically. The OUTPUT knob trims on top of this.

Arc labels visible on the panel: **LIGHT · THHKY · RADIO · TOO MUCH**

---

### SPEED `0.0 – 10.0` · default `3.0`

Continuously morphs the compression time character between two classic styles:

| SPEED value | Style | Attack | Release |
|-------------|-------|--------|---------|
| 0 | Optical / warm | 50 ms | 400 ms |
| 5 | Hybrid | 26 ms | 238 ms |
| 10 | FET / aggressive | 3 ms | 75 ms |

- **Low SPEED** (0–3): Slow attack lets transients breathe through. Slow release creates musical glue. Sounds warm and open.
- **High SPEED** (7–10): Fast attack crushes transients immediately. Punchy and tight. Sounds hard and forward.

---

### TONE `0.0 – 10.0` · default `4.0`

Harmonic saturation amount applied via a soft-tanh waveshaper after the compressor stage.

| TONE value | Drive | Character |
|------------|-------|-----------|
| 0 | 1.0× | Clean — compression only, no coloring |
| 5 | 2.25× | Warm — audible 2nd/3rd harmonic richness |
| 10 | 3.5× | Heavy — aggressive harmonic distortion |

The waveshaper uses the approximation `x·(27 + x²) / (27 + 9x²)`, which generates both even (warmth) and odd (edge) harmonics depending on signal level. Drive is normalized post-processing to prevent large output level changes.

---

### OUTPUT `−12.0 – +12.0 dB` · default `0.0 dB`

Output trim applied after auto makeup gain. Use this to match the plugin's output level to your gain staging, or to push harder into the next stage.

---

### MIX `0.0 – 1.0` · default `1.0`

Parallel compression blend.

| MIX value | Result |
|-----------|--------|
| 0.0 | Dry signal only — plugin has zero effect |
| 0.2–0.4 | New York compression — transients preserved, thickness added |
| 1.0 | Fully wet — maximum character, full compression |

The dry path uses the post-input signal (before the envelope follower), ensuring consistent level reference when blending.

---

### BYPASS · default `off`

Stomp-switch bypass. Passes audio through unmodified. The yellow LED in the title bar dims when bypassed.

---

## DSP Architecture

### Signal Chain (per sample, per channel)

```
Input
  │
  ├──[Dry Path]──────────────────────────────────────────────────────────┐
  │                                                                       │
  └──►[Envelope Follower]──►[Gain Computer]──►[VCA]──►[Saturation]──►[Parallel Mix]──►[Output Trim]──► Output
         ↑                        ↑                        ↑                  ↑
      (SPEED controls         (SQUEEZE maps            (TONE controls      (MIX blends
      attack/release)         threshold/ratio/knee)    drive amount)       dry + wet)
```

### 1. Envelope Follower

First-order IIR leaky integrator (peak detector with asymmetric attack/release).

```
if |input| > envelope:
    envelope += attackCoeff × (|input| − envelope)
else:
    envelope += releaseCoeff × (|input| − envelope)
```

Coefficients: `1 − exp(−1 / (time_s × sampleRate))`, linearly interpolated from SPEED.
A `1e-10` floor prevents denormal floats during silence.

### 2. Gain Computer

Piecewise log-domain calculator with quadratic soft-knee interpolation.

```
if levelDB < (threshold − halfKnee):
    GR = 0                                                       // below knee
else if levelDB < (threshold + halfKnee) and knee > 0:
    GR = (1/ratio − 1) × (over + halfKnee)² / (2 × knee)       // soft knee (quadratic)
else:
    GR = (1/ratio − 1) × (levelDB − threshold)                  // above knee (full ratio)
```

At `SQUEEZE=10`, `knee=0` so the soft-knee branch is never entered — true hard knee.

### 3. Auto Makeup Gain

Approximation of expected gain loss at threshold, baked into SQUEEZE:

```
autoMakeupDB = −threshold × (1 − 1/ratio) × 0.5
```

Conservative ×0.5 factor avoids over-gain. Fine-tune with OUTPUT.

### 4. Saturation (Soft-Tanh Waveshaper)

Applied after compression. `drive = 1.0 + (tone/10) × 2.5`.

```
softTanh(x) = x × (27 + x²) / (27 + 9x²)

saturated = softTanh(compressed × drive) / drive
```

The `/ drive` normalization preserves approximate output level across the TONE range.

### 5. Parallel Mix

```
output = lerp(dryInput, saturated, mix) × outputGainLinear
```

### 6. Gain Reduction Metering

The kernel accumulates average gain reduction (dB) per buffer into `mGainReductionDB`.
This is read on the main thread via `getGainReductionDB()`, bridged to Swift via
`VXAtomExtensionAudioUnit.gainReductionDB()`, and polled by the UI at 30 fps using
`TimelineView(.animation(minimumInterval: 1/30))`.

> **Thread safety note:** `mGainReductionDB` is a plain `float`, not `std::atomic`.
> A stale read on the UI thread produces at most one incorrect meter frame and is
> acceptable for a display-only value. `std::atomic` operations are not real-time
> safe and are deliberately avoided in the render path.

---

## Parameters Reference

Defined in [`Parameters/VX-AtomExtensionParameterAddresses.h`](Parameters/VX-AtomExtensionParameterAddresses.h)
and [`Parameters/Parameters.swift`](Parameters/Parameters.swift).

| Address | Identifier | Name | Units | Range | Default |
|---------|------------|------|-------|-------|---------|
| 0 | `squeeze` | Squeeze | generic | 0.0 – 10.0 | 5.0 |
| 1 | `speed` | Speed | generic | 0.0 – 10.0 | 3.0 |
| 2 | `tone` | Tone | generic | 0.0 – 10.0 | 4.0 |
| 3 | `outputGain` | Output | decibels | −12.0 – +12.0 | 0.0 |
| 4 | `mix` | Mix | generic | 0.0 – 1.0 | 1.0 |
| 5 | `bypass` | Bypass | boolean | 0.0 – 1.0 | 0.0 |

> **BREAKING:** Parameter addresses 0 and 1 were previously `gain` and `bypass`
> from the gain template. Any serialized preset state from the gain template is incompatible.

---

## Project File Layout

```
VX-AtomExtension/
├── Parameters/
│   ├── VX-AtomExtensionParameterAddresses.h   ← C enum shared by Swift + C++
│   └── Parameters.swift                        ← AUParameterTree specs
│
├── DSP/
│   └── VX-AtomExtensionDSPKernel.hpp           ← Full compressor DSP (C++)
│
├── UI/
│   ├── VX-AtomExtensionMainView.swift          ← Nuclear UI (SwiftUI)
│   ├── ParameterKnob.swift                     ← Rotary knob component
│   └── BypassButton.swift                      ← Stomp switch component
│
└── Common/
    ├── Audio Unit/
    │   └── VX-AtomExtensionAudioUnit.swift     ← AUAudioUnit subclass + metering bridge
    ├── UI/
    │   ├── AudioUnitViewController.swift        ← NSViewController, AU factory
    │   └── ObservableAUParameter.swift          ← @Observable AU parameter wrapper
    ├── DSP/
    │   ├── VX-AtomExtensionAUProcessHelper.hpp ← Sample-accurate event dispatch
    │   └── VX-AtomExtensionBufferedAudioBus.hpp← Input bus buffer management
    └── Parameters/
        └── ParameterSpecBase.swift              ← ParameterTreeSpec / ParameterGroupSpec / ParameterSpec
```

---

## Adding a Parameter

All four layers must stay in sync. Follow these steps in order.

### Step 1 — Add address to the C enum

`Parameters/VX-AtomExtensionParameterAddresses.h`:

```c
typedef NS_ENUM(AUParameterAddress, VXAtomExtensionParameterAddress) {
    squeeze    = 0,
    speed      = 1,
    tone       = 2,
    outputGain = 3,
    mix        = 4,
    bypass     = 5,
    myNewParam = 6   // ← add here, always increment from last value
};
```

### Step 2 — Add a ParameterSpec

`Parameters/Parameters.swift`:

```swift
ParameterSpec(
    address: .myNewParam,
    identifier: "myNewParam",    // used for dot-notation: parameterTree.global.myNewParam
    name: "My Parameter",
    units: .generic,
    valueRange: 0.0...10.0,
    defaultValue: 5.0
)
```

### Step 3 — Handle it in the DSP kernel

`DSP/VX-AtomExtensionDSPKernel.hpp` — add to both `setParameter()` and `getParameter()`:

```cpp
// In setParameter():
case VXAtomExtensionParameterAddress::myNewParam:
    mMyNewParam = value;
    break;

// In getParameter():
case VXAtomExtensionParameterAddress::myNewParam:
    return mMyNewParam;

// Add member variable in the private section:
float mMyNewParam = 5.0f;
```

### Step 4 — Expose it in the UI

`UI/VX-AtomExtensionMainView.swift`:

```swift
ParameterKnob(param: parameterTree.global.myNewParam, size: 64)
```

---

## UI Components

### `ParameterKnob`

Generic rotary knob. Scale marks are positioned at `size × 0.464` radius from center,
appearing on the knob face. The total layout frame is `size + scaleRadius × 2`.
Drag vertically to change value (sensitivity: 0.005 × range per pixel).

```swift
ParameterKnob(param: parameterTree.global.squeeze, size: 86)  // size in pts
```

### `BypassButton`

Stomp-switch using the `stomp` image asset. Taps toggle `param.boolValue`.

```swift
BypassButton(param: parameterTree.global.bypass)
```

### `VUMeter` (private to MainView)

SwiftUI `Canvas` drawing a vertical needle showing 0–20 dB gain reduction.
Polled at 30 fps via `TimelineView`. Data flows:

```
DSPKernel.mGainReductionDB
    → kernel.getGainReductionDB()                   [C++]
    → VXAtomExtensionAudioUnit.gainReductionDB()    [Swift bridge]
    → gainReductionProvider closure                  [weak AU reference]
    → TimelineView poll in VUMeter                   [SwiftUI, 30fps]
```

---

## Languages & Tools

| Layer | Language / Framework |
|-------|---------------------|
| DSP | C++20 |
| Parameters | C (header), Swift |
| UI | SwiftUI (macOS 15.7+) |
| AU Lifecycle | Swift / AVFoundation |
| Build | Xcode 16.2+, `build.sh` |

---

## More Information

- [Apple Audio Developer Documentation](https://developer.apple.com/audio/)
- [AUv3 Extensions](https://developer.apple.com/documentation/audiotoolbox/audio_unit_v3_plug-ins)
- [Conventional Commits](https://www.conventionalcommits.org/) (this project's commit standard)

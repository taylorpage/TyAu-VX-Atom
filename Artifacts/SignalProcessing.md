# TyAu-Distortion Signal Processing Documentation

This document describes the complete signal processing chain and design decisions for the TyAu-Distortion audio plugin.

## Signal Flow Overview

```
Input Signal
    ↓
[Pre-Distortion EQ] - High-pass filter at 75Hz
    ↓
[Pre-Gain] - Drive-dependent gain (1x to 13x)
    ↓
[4x Oversampling - Upsample] - Linear interpolation
    ↓
[Hard Clipping] - Asymmetric threshold limiting
    ↓
[4x Oversampling - Downsample] - Averaging + DC blocker
    ↓
[Makeup Gain] - Progressive boost for consistent volume
    ↓
Output Signal
```

## 1. Pre-Distortion EQ

**Purpose:** Shape the frequency content before it hits the distortion stage

**Implementation:**
- **Type:** 2nd-order Butterworth high-pass filter
- **Frequency:** 75Hz
- **Q:** 0.707 (critically damped)
- **Processing:** Biquad IIR filter, per-channel state

**Why:**
- Removes low-frequency rumble and mud before distortion
- Prevents flabby bass from eating up headroom
- Creates tighter, more focused distortion character
- Modeled after natural guitar amp frequency response

**Filter Coefficients:**
```
fc = 75Hz / sampleRate
w0 = 2π × fc
α = sin(w0) / (2 × Q)

b0 = (1 + cos(w0)) / 2
b1 = -(1 + cos(w0))
b2 = (1 + cos(w0)) / 2
a0 = 1 + α
a1 = -2 × cos(w0)
a2 = 1 - α

All coefficients normalized by a0
```

## 2. Pre-Gain Stage

**Purpose:** Control distortion intensity

**Implementation:**
```cpp
preGain = 1.0 + (drive × 12.0)
// Maps drive (0.4-1.0) to gain (5.8x-13.0x)
```

**Drive Range:**
- Minimum: 40% → 5.8x gain
- Maximum: 100% → 13.0x gain
- Default: 70% → 9.4x gain

**Why aggressive gain:**
- High gain ensures sufficient saturation for lead/solo work
- Pushes clipping stage hard for rich harmonic content
- Gives the pedal ability to cut through dense mixes
- Matches or exceeds commercial crunch pedal drive levels

**Why restricted range:**
- Below 40% drive produces weak, unusable distortion
- Starting at 40% ensures consistent, quality distortion character
- User experience: knob only covers the "sweet spot"

## 3. 4x Oversampling

**Purpose:** Reduce aliasing artifacts from non-linear distortion

### Upsampling (1 sample → 4 samples)

**Method:** Linear interpolation
```cpp
out[0] = prev + (input - prev) × 0.25
out[1] = prev + (input - prev) × 0.50
out[2] = prev + (input - prev) × 0.75
out[3] = input
```

**Why 4x oversampling:**
- Significantly smoother than 2x oversampling
- Reduces high-frequency aliasing from aggressive clipping
- Eliminates "grungy" artifacts while maintaining tight character
- Balances CPU efficiency with audio quality

**Why linear interpolation:**
- Simple and efficient
- Low CPU overhead
- Sufficient for 4x oversampling
- Maintains phase relationships

### Downsampling (4 samples → 1 sample)

**Method:** Averaging with DC blocking
```cpp
downsampled = (sample1 + sample2 + sample3 + sample4) × 0.25

// DC blocker (1st-order high-pass at ~5Hz)
output = downsampled - z1 + 0.995 × previous_output
z1 = downsampled
```

**Why DC blocker:**
- Asymmetric clipping can introduce DC offset
- High-pass at 5Hz removes DC without affecting audio
- Prevents speaker cone drift and downstream issues

**Per-Channel State:**
- All oversampling state variables are per-channel (8 channel support)
- Prevents crosstalk between stereo channels
- Essential for independent left/right processing

## 4. Hard Clipping Algorithm

**Type:** Asymmetric hard threshold limiting

**Implementation:**
```cpp
positiveThreshold = 0.7 - (drive × 0.60)  // 0.46 to 0.10
negativeThreshold = 0.8 - (drive × 0.60)  // 0.56 to 0.20

if (sample > positiveThreshold)
    return positiveThreshold
else if (sample < -negativeThreshold)
    return -negativeThreshold
else
    return sample
```

**Threshold Design:**
- Very aggressive clipping at high drive (thresholds as low as 0.1/0.2)
- Creates extreme saturation for lead tones
- Combined with high pre-gain, generates dense harmonic content
- Asymmetry maintained throughout entire drive range

**Asymmetry:**
- Positive peaks clip earlier and harder (creates bite and presence)
- Negative peaks clip later (adds warmth and body)
- Generates even-order harmonics for richer, more complex tone
- Mimics natural tube amp behavior

**Why hard clipping:**
- Crisp, aggressive character perfect for rock/metal
- No soft-knee artifacts at low drive levels
- Predictable, consistent behavior
- 4x oversampling smooths the edges to prevent harshness

**Design Evolution:**
1. Started with soft clipping (tanh) → too grungy
2. Tried hybrid soft/hard → volume jumps at transitions
3. Settled on pure hard clip with 2x oversampling → clean but lacking bite
4. Increased to 4x oversampling + aggressive thresholds → smooth yet powerful

## 5. Makeup Gain

**Purpose:** Maintain consistent volume despite heavy clipping

**Implementation:**
```cpp
makeupGain = 1.0 + (drive × 2.5)
```

**Behavior:**
- At 40% drive: 2.0x makeup gain
- At 70% drive: 2.75x makeup gain
- At 100% drive: 3.5x makeup gain

**Why progressive boost:**
- Aggressive clipping reduces signal amplitude dramatically
- Higher drive = more clipping = more volume loss to compensate
- Progressive boost maintains perceived loudness across entire drive range
- Prevents the "volume sag" common in high-gain distortion pedals

**Design Rationale:**
- With clipping thresholds as low as 0.1/0.2, the signal is heavily reduced
- Traditional makeup gain (reducing volume) would make high drive unusable
- Progressive boost ensures the pedal stays punchy even at maximum drive
- Tested against commercial crunch pedals for volume consistency

## Technical Specifications

### Performance
- **Sample Rate:** Independent (initialized per-instance)
- **Channel Support:** Mono (1→1), Stereo (2→2), up to 8 channels
- **Max Frame Count:** 1024 frames per render block
- **In-Place Processing:** Supported
- **Thread Safety:** Real-time safe (no allocations, locks, or blocking)

### Latency
- **Additional Latency:** 0 samples
- 4x oversampling is internal buffering only
- No lookahead or delay compensation needed

### CPU Usage
- Very efficient despite 4x oversampling
- Simple algorithms optimized for real-time
- Per-sample processing, no FFT or convolution
- ~2x CPU usage vs 2x oversampling (still minimal on modern hardware)

## Frequency Response Characteristics

### Pre-Distortion EQ
- **75Hz:** -3dB
- **50Hz:** ~-7dB
- **30Hz:** ~-15dB
- **100Hz+:** Flat (0dB)

### Overall Character
- **Low End:** Tight and focused (75Hz HPF)
- **Midrange:** Punchy with rich harmonic saturation (aggressive asymmetric clipping)
- **High End:** Crisp and clear (4x oversampling prevents aliasing and harshness)
- **Overall:** Smooth yet aggressive, cuts through mixes without sounding grungy

## Design Philosophy

1. **Simplicity:** Each stage has a single, well-defined purpose
2. **Quality:** 4x oversampling ensures smooth, professional sound
3. **Aggression:** High gain and tight clipping for cutting lead tones
4. **Artifact-Free:** Pure hard clipping eliminates soft-knee transitions
5. **Musical:** Asymmetric clipping adds harmonic complexity
6. **User-Focused:** Drive range restricted to usable sweet spot (40-100%)
7. **Volume Consistency:** Progressive makeup gain maintains punch at all settings

## Design Iterations

### Version 1.0 (Initial)
- 2x oversampling
- Pre-gain: 1x-6x
- Clipping thresholds: 0.8/0.9 → 0.25/0.35
- Makeup gain: dividing (volume reduction)
- **Result:** Too grungy, lacked sustain, volume dropped at high drive

### Version 1.1 (Current)
- 4x oversampling
- Pre-gain: 1x-13x
- Clipping thresholds: 0.7/0.8 → 0.1/0.2
- Makeup gain: progressive boost (1x-3.5x)
- **Result:** Smooth yet aggressive, cuts through mixes, consistent volume

## Future Enhancements

Potential additions (not yet implemented):
- Adjustable pre-distortion EQ
- Mix/blend control (dry/wet)
- Multiple distortion modes (switchable algorithms)
- Post-distortion tone controls
- Additional drive parameter for extended gain range
- Switchable oversampling (2x/4x/8x)

---

**Last Updated:** January 27, 2026
**Version:** 1.1
**Author:** Taylor Page with Claude Sonnet 4.5

# VX-ATOM — Sound Design Document

This document captures the sonic philosophy, design decisions, and intended character of VX-ATOM. It explains *why* the plugin sounds the way it does and *why* those choices were made — context that lives alongside the technical implementation in [`VX-AtomExtension/README.md`](VX-AtomExtension/README.md) but is distinct from it.

---

## The Core Philosophy: Character vs. Utility

There are two fundamentally different reasons to reach for a compressor.

The first is **utility**: you need to control dynamics, tame peaks, and keep a signal in its lane. You want the compressor to be invisible. You're solving a technical problem. SSL G-Bus, Neve 33609, most mastering compressors — these are tools. The goal is to use them without leaving a trace.

The second is **character**: you want the compressor to *do something* to the sound. You want it to change the texture, thicken the tone, add aggression, push the signal forward. The compressor is part of the sound design, not a correction step. The Urei 1176, Teletronix LA-2A, Empirical Labs Distressor at high ratios — these are instruments.

VX-ATOM is firmly in the second camp. It is designed to impose a sound. If you want invisible dynamics control with precise, adjustable attack and release times, a variable ratio, and a surgical threshold, VX-ATOM is the wrong tool. If you want to slam a vocal into the mix and have it feel alive and aggressive and present, it is exactly the right tool.

This means the plugin is opinionated by design. Some decisions that would be bugs in a utility compressor are features here.

---

## The Inspiration: Jack Joseph Puig's Approach to Vocals

The primary sonic reference for VX-ATOM is the JJP Vocal Compressor by Waves — a plugin built around the processing philosophy of producer/mixer Jack Joseph Puig, whose credits include Green Day, Sheryl Crow, Rolling Stones, and hundreds of others.

What Puig's processing is known for is a quality that is hard to quantify but immediately recognizable: vocals that feel like they *sit inside* the track, not on top of it, while simultaneously being forward, present, and impossible to ignore. The compression is often very heavy — he is not shy about gain reduction — but it never sounds squashed or lifeless. The signal comes out more energized than it went in.

Several specific things account for this quality, and each one is reflected in VX-ATOM's architecture.

### Three-Stage Compression Architecture

The most significant structural decision in VX-ATOM is that compression happens in three serial stages, each with a genuinely different personality. This is not the same as setting a ratio high — it is what separates the sound of a heavily worked-over mix through multiple hardware units from the sound of one compressor turned up.

**Stage 1 — The Aggressor:** A VCA-style compressor with a threshold that descends from -12 dB to -60 dB as SQUEEZE increases. Ratio ranges from 4:1 to 200:1 with a fast attack derived from SPEED. This stage does the heavy dynamic work.

**Stage 2 — The Presser:** An independent compressor running on the post-Stage-1 signal, but with its own threshold range (-10 to -25 dB), its own ratio (4:1 to 8:1), and time constants that run 2× slower on attack and 1.5× longer on release than Stage 1. Because it has different settings, it responds to different parts of the already-compressed signal — particularly the pumping artifacts and dynamic variation that Stage 1 leaves behind. The two stages interact at different timescales, which is what creates the characteristic "stacked compressor" feel: compressed, but alive.

**Stage 3 — The Ceiling:** A fast brick-wall limiter (80:1, ~0.5 dB knee) sitting between Stage 2 and the saturation stage. Its threshold scales from -2 dB at zero SQUEEZE to -8 dB at maximum, with intentionally no auto makeup gain. The signal hits the ceiling and stays there — that is the sound. The ceiling pressing down against the signal's attempts to escape is a large part of the "pressed against the wall" quality that high SQUEEZE settings produce.

The three stages are not trying to do the same thing at increasing intensity. They are three different compressors with three different jobs, running in series. This is why stacking two instances of the same compressor in code doesn't produce the same result — mathematical duplication produces a harder version of one compressor. Three different personalities at three different timescales produces a different compression *texture*.

### Parallel Compression Architecture

Puig's processing chains typically blend a heavily compressed signal back with the dry signal rather than replacing it. This is the "New York compression" technique, though Puig takes it further than most. The heavily compressed signal provides density, thickness, and sustain. The dry signal provides attack transients, definition, and life. The blend is what makes the result feel both controlled and dynamic at the same time.

In VX-ATOM this is the **MIX** knob. At `MIX=1.0` you are fully compressed — maximum character but transients are softened. At `MIX=0.3–0.5` you get the Puig blend: the dense, thick compressed signal underneath the natural transient attack of the dry signal. The result is compression you can *feel* without it sounding like compression.

### Harmonic Enrichment Through Saturation

Optical and tube-based compressors (which Puig favors — the Fairchild 670 is his most famous tool) add harmonic distortion as a byproduct of how they work. The optical element and tube gain stage are inherently nonlinear. As the gain reduction changes, the waveshape of the signal changes with it, generating harmonics that weren't in the original signal.

These harmonics are not noise. Even-order harmonics (2nd, 4th) are musically consonant — they sit at the octave and the fifth, reinforcing the fundamental and adding warmth and body. Odd-order harmonics (3rd, 5th) are more complex and create edge, aggression, and presence. Tube circuits tend toward even-order. Transistor/FET circuits tend toward odd-order mixed with even. The result in both cases is a signal that occupies more sonic space — it sounds *bigger* in the mix than the level meter suggests.

In VX-ATOM, the **TONE** knob is the explicit version of what optical and tube compressors do implicitly. The soft-tanh waveshaper generates both even and odd harmonics in amounts that scale with the drive setting. At low TONE values the character is clean. As TONE increases, the signal gets richer, thicker, and more forward. At high TONE settings the saturation is heavy and intentional — aggressive, not subtle.

### Slow Attack as a Feature

The Fairchild 670's attack times run 200–400 ms by nature of its optical element. This is *slow* for a compressor — far slower than most engineers would reach for if they were setting an attack time manually. But it is not a limitation. It is the source of a large part of the Fairchild's character.

A slow attack means the initial transient — the consonant of a sung word, the pick attack of a guitar note — passes through uncompressed. The body of the note is what gets compressed. The result is a vocal that sounds natural and forward at the same time. The transient gives it life and attack. The compressed sustain gives it thickness and density. The contrast between the two is what makes it feel "musical."

In VX-ATOM, **SPEED** at 0 replicates this slow optical behavior: 50 ms attack, 400 ms release. Vocals and instruments treated at low SPEED will have their transients preserved and their sustain thickened. The character is warm and open. At high SPEED (toward 10), the behavior shifts to FET-style fast attack: 3 ms attack catches the transient immediately, and 75 ms release is fast enough to let the compressor react quickly without pumping. The character becomes punchy and tight. Neither is wrong — they are different sounds for different contexts.

---

## The SQUEEZE Concept

The most unconventional decision in VX-ATOM is mapping compression intensity to a single dial instead of exposing threshold and ratio separately.

The conventional approach is: you set a threshold, you set a ratio, and you adjust them independently based on how much gain reduction the meter shows. This is rational and precise and correct for a utility compressor.

But it creates a problem for a character compressor: the knob relationship matters. Threshold and ratio are not independent variables from a sonic standpoint. A high ratio with a threshold near 0 dB is a gentle compressor that barely engages. The same ratio with a threshold at -25 dB is a limiter. Ratio and threshold together determine *how much* of the signal gets compressed and *how hard* it gets hit when it does.

More importantly, for a character compressor, the *knee* is part of the sound. A soft knee (8 dB transition zone) feels smooth and musical — the compression engages gradually. A hard knee (0 dB — compression snaps on the instant the threshold is crossed) feels tight, aggressive, and industrial. These are not just sonic preferences; they change the harmonic content of what comes out. Fast gain changes generate more harmonic distortion than slow ones.

SQUEEZE links all three together in a way that is always sonically coherent:

- At low values (LIGHT range), the threshold is high, the ratio is gentle, and the knee is wide. The compressor barely engages. What engagement there is, is smooth and almost imperceptible. This is glue compression.
- In the middle (THHKY to RADIO range), the threshold drops into the body of most signals, the ratio is assertive, and the knee is starting to firm up. This is where the character lives for most use cases.
- At maximum (TOO MUCH), Stage 1's threshold is at -60 dB with a 200:1 ratio and a hard knee — everything above the noise floor is being compressed, and it is being compressed hard. Stage 2 simultaneously locks down with an 8:1 ratio, and Stage 3's ceiling drops to -8 dB. Combined with TONE, this is wall-of-sound territory. It is not subtle. The name is honest.

The auto makeup gain built into SQUEEZE means you can sweep it without the output level changing dramatically, making it easy to audition the character at different intensities during a mix.

---

## SPEED: Two Compression Philosophies on One Knob

The compression time constants — attack and release — are probably the most sonically important parameters in any compressor. More so than ratio. More so than threshold. The *timing* of gain reduction determines whether a compressor sounds transparent, musical, pumping, tight, or alive.

There are two archetypes worth understanding because VX-ATOM's SPEED knob spans exactly the range between them.

**Optical compression** (LA-2A, LA-3A, Fairchild): The gain reduction element is a light-dependent resistor (LDR) driven by an electroluminescent panel. The physics of how light and resistance change creates time constants that are not fixed — they vary with the signal. Low-level signals release slowly. High-level signals attack and release more quickly. This frequency-dependent, program-dependent behavior is what makes optical compressors sound "musical" — the compressor is responding to the music rather than following a rigid clock.

VX-ATOM's slow extreme (SPEED=0) approximates this with a 50 ms attack and 400 ms release. It will not pump. It will not clamp down on transients. It adds density and sustain without being heard as compression. Use this on sustained vocals, on pad layers, on bus material where you want cohesion without the compressor announcing itself.

**FET compression** (Urei 1176): The gain reduction element is a field-effect transistor. It is electronic, fast, and responds to the signal level in real time with very little lag. Attack times as low as 20 microseconds. Release as fast as 50 ms. The 1176 is famous for sounding aggressive and forward — it catches everything, colors everything, and makes everything punch. It is a big part of the sound of rock and hip-hop drums and vocals for the last 50 years.

VX-ATOM's fast extreme (SPEED=10) approximates this with a 3 ms attack and 75 ms release. At this setting the compressor responds quickly to transients. Vocals become tight and controlled. Drums become punchy. The TONE knob at mid to high settings combined with fast SPEED produces the most aggressive character in the plugin.

The SPEED knob is continuous because the truth is that most great-sounding compressor settings live somewhere between these extremes, and that middle ground is not well served by a binary "optical / FET" mode switch.

---

## TONE: Why Saturation Lives After the Compressor

A common question about saturation in a compressor is: why apply it after gain reduction? Why not before?

The answer is about what saturation at each point in the chain does to the sound.

Saturation *before* compression changes the signal that the compressor sees. Drive into a saturator and the peaks get rounded off, making the compressor respond differently — softer in some ways, but the saturation artifacts are then also being compressed. The result is often a particular kind of dense thickness, but it can also lose definition.

Saturation *after* compression operates on a signal that has already had its dynamics managed. The gain reduction has already happened. The compressed signal is then pushed through a nonlinear element that colors it without fighting with the dynamics. The saturation is added to the result, not mixed into the process. This tends to produce cleaner harmonic enrichment — the saturation adds character to the compressed signal rather than blurring the boundary between the two effects.

In VX-ATOM specifically, the saturation sits after all three compression stages — after Stage 1, Stage 2, and the Stage 3 ceiling limiter have all acted on the signal. This means the signal entering the waveshaper has already been dynamically compressed, further pressed, and ceiling-clamped. It is a very consistent, dense signal. The TONE waveshaper operates on material that is already thoroughly managed, generating harmonics that are proportional and musical rather than erratic. At high SQUEEZE settings this produces a thick, saturated density — the saturation and the three-stage compression reinforce each other's character rather than competing.

The soft-tanh waveshaper specifically was chosen over harder clippers or asymmetric saturators because it generates harmonics smoothly across both even and odd orders without a hard transition into clipping. There is no point where the saturation suddenly breaks or becomes obviously distorted — it grades continuously from clean to colored to thick. This matches the behavior of tape saturation and the subtle nonlinearity of the output transformer in vintage compressor designs, which is a large part of what makes those compressors sound "warm" rather than "distorted."

---

## The Nuclear Aesthetic: Why It Matters

The visual design of VX-ATOM is not cosmetic. It is part of the experience of using the plugin and part of how it communicates its intent.

The nuclear-era fallout aesthetic — the worn teal-green metal, the radioactive symbol, the industrial typography, the vintage VU meter — communicates that this is not a precision instrument. It is something that was built in an era before digital overthinking, when engineers made choices by ear and lived with them. The distressed panel suggests a tool that has been used hard. The "You sure?" subtitle under SQUEEZE is a genuine question. The "TOO MUCH" arc label is not ironic. There is a reason the bypass stomp switch is prominent.

This aesthetic also stands in deliberate contrast to the ultra-clean, Swiss-minimalist look of most modern plugin design. A utility compressor should look like a utility tool — precise, neutral, informative. A character compressor should have character. The visual design sets the expectation before the user touches a knob.

The VU meter showing gain reduction rather than output level is also intentional. A VU meter is a slow-responding, ballistic instrument — it was never accurate at showing fast transients. But it shows the *average* behavior of the compressor, which is exactly what matters for a character compressor. You are not trying to catch peak gain reduction to the tenth of a dB. You are watching how much the compressor is working on average and whether it feels like the right amount. The needle behavior — its physics, its overshoot — communicates musical information that a fast-response digital bar graph does not.

---

## Recommended Starting Points

These are not presets. They are jumping-off points that reflect the intended use of each control.

### Lead Vocal — Forward and Controlled
```
SQUEEZE:  6.0  (RADIO zone — firm threshold, assertive ratio)
SPEED:    4.0  (slightly optical — lets consonants breathe)
TONE:     4.0  (mild harmonic enrichment, not heavy)
OUTPUT:   0.0  (adjust to match dry level)
MIX:      0.7  (mostly wet with some parallel transient preservation)
```
The compressor works hard but the 4.0 SPEED keeps some natural attack character. TONE at 4 adds presence and thickness. MIX at 0.7 keeps the compressed density while letting transients punch through naturally.

---

### Vocal — Glue and Warmth
```
SQUEEZE:  4.0  (THHKY zone — threshold in the body of the signal)
SPEED:    1.5  (near optical — slow attack, long release)
TONE:     6.0  (more saturation for warmth and richness)
OUTPUT:   0.0
MIX:      1.0  (fully wet — let the optical character dominate)
```
Slow attack and long release means the compressor is smoothing out the average level rather than catching peaks. The higher TONE compensates by adding harmonic richness and keeping the vocal present. Good for intimate, warm vocal sounds where aggression is not the goal.

---

### Drum Bus — Punch and Density
```
SQUEEZE:  7.5  (toward TOO MUCH — deep threshold, high ratio)
SPEED:    9.0  (near FET — fast attack and release)
TONE:     5.0  (moderate saturation for drum color)
OUTPUT:  -1.0  (trim back slightly — auto makeup may be generous)
MIX:      0.4  (New York compression — dry attack, dense sustain)
```
Fast SPEED catches transients aggressively, adding punch. High SQUEEZE flattens dynamics. MIX at 0.4 lets the snare snap and kick thud through the dry path while the compressed signal adds thickness and sustain. This is the most obvious parallel compression application.

---

### Maximum Press — Nuclear Vocal or Instrument Character
```
SQUEEZE: 10.0  (TOO MUCH — all three stages at full engagement)
SPEED:    5.0  (mid-speed — enough attack to stay punchy, not so fast it just sounds crushed)
TONE:     6.0  (heavy harmonic enrichment — the saturation locks in with the compression)
OUTPUT:  -3.0  (the three-stage chain can hit hard; trim back to match dry level)
MIX:      1.0  (fully wet — the point is the pressed sound, not the transients)
```
All three compression stages are working simultaneously. Stage 1 crushes. Stage 2 presses. Stage 3's ceiling sits at -8 dB and holds everything under it. The result is a vocal or instrument sound that feels physically dense — not simply quieter with the peaks removed, but as though the sound has been pressed into a smaller space and pushed forward. Works best on material that needs to compete in a dense mix without riding faders. Not appropriate for acoustic or classical contexts. Very appropriate for electronic, hip-hop, or heavily produced pop sources.

---

### Bus Glue — Transparent but Present
```
SQUEEZE:  3.0  (LIGHT-to-THHKY — gentle engagement)
SPEED:    2.0  (optical-leaning — program-dependent feel)
TONE:     2.0  (subtle coloring only)
OUTPUT:   0.0
MIX:      0.85 (mostly wet with a touch of dry)
```
Light settings across the board. The compressor is barely working, just cohering the mix elements slightly. The low TONE adds a very subtle harmonic sheen. Good for a last-stage mix bus treatment where you want the compressor to be felt rather than heard.

---

## What VX-ATOM Is Not

Because it is a character compressor, there are things it is deliberately bad at:

**Precise dynamic control.** If you need exactly 6 dB of gain reduction on peaks above -12 dBFS with a 4:1 ratio and an 8 ms attack, use a different compressor. VX-ATOM's SQUEEZE mapping makes precise technical settings unavailable by design.

**Transparent operation.** At any setting above LIGHT, VX-ATOM is coloring the signal. That is the point. If you need to manage dynamics without any sonic footprint, use a different compressor.

**De-essing.** No frequency-conscious sidechain. VX-ATOM compresses the full signal. High-frequency content will trigger the compressor the same as low-frequency content.

**Multi-band compression.** Single wideband path only.

**Per-channel independent settings.** Stereo operation uses the same envelope follower index clamped to two channels. The plugin does not support mid/side or independent L/R processing.

---

## Design Decisions That Could Be Revisited

These are not bugs, but they are choices that have trade-offs and could reasonably be changed in future versions:

**The SPEED interpolation is linear.** The perceptual difference between 0–2 ms of attack is much larger than the difference between 20–22 ms. A logarithmic or exponential interpolation across the attack range might give more useful resolution at fast settings.

**The saturation is post-compression only.** An optional pre-compression saturation path (a "drive" stage before the VCA) would create a different tonal character and is how many vintage hardware designs work. Could be a mode or a second TONE-style knob.

**Auto makeup gain is an approximation.** Stages 1 and 2 both use the formula `−threshold × (1 − 1/ratio) × 0.5`, which is conservative. At extreme SQUEEZE settings the actual gain reduction can exceed the auto makeup, resulting in net attenuation. Stage 3 (the ceiling limiter) applies no auto makeup intentionally — the level staying down under the ceiling is part of the sound. The OUTPUT knob is the intended correction for overall level offset; a more accurate auto-gain algorithm measuring actual average gain reduction over time would make SQUEEZE sweeps more level-consistent.

**The VU meter shows average, not peak.** A secondary peak indicator (LED-style at the top of the meter range) would make it easier to see brief transient gain reduction that the ballistic needle misses. Not necessary for the plugin's intended use, but useful for educational purposes.

---

*Last updated: February 2026 — updated to reflect three-stage compression architecture (Stage 1 aggressor, Stage 2 presser, Stage 3 ceiling limiter)*

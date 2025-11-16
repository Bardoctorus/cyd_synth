# CYD Synthesizer Architecture

This document explains the code structure, signal flow, and purpose of each module in the CYD Synthesizer project.

## Project Structure

```
src/
├── main.cpp                    # Main entry point - orchestration only
├── config.h                    # Centralized configuration constants
├── hardware/
│   ├── display.h/cpp          # TFT display management
│   ├── touch.h/cpp            # Touchscreen input handling
│   └── audio_output.h/cpp     # I2S/DAC output with safety limiting
├── audio/
│   ├── oscillator.h/cpp       # Dual oscillator (sine + sawtooth)
│   ├── filter.h/cpp          # Biquad low-pass filter
│   ├── delay.h/cpp            # Stereo delay effect
│   ├── lfo.h/cpp              # Low frequency oscillator for delay modulation
│   └── adsr.h/cpp             # ADSR envelope generator
└── ui/
    ├── pentatonic_ui.h/cpp    # Pentatonic scale quantization and visualization
    └── parameter_control.h/cpp # Parameter sliders for delay, LFO, base tone, upper tone
```

## Signal Flow

The audio signal follows a "mixing desk" architecture, similar to a DAW:

```
┌─────────────────────────────────────────────────────────────┐
│                    AUDIO TASK (Core 0)                      │
│                                                              │
│  1. OSCILLATOR                                               │
│     ┌─────────────┐                                          │
│     │ Sine Wave   │                                          │
│     │ (freq)      │                                          │
│     └──────┬──────┘                                          │
│            │                                                  │
│     ┌──────▼──────┐                                          │
│     │ Sawtooth    │                                          │
│     │ (freq * ratio)│                                        │
│     └──────┬──────┘                                          │
│            │                                                  │
│            ▼                                                  │
│     ┌──────────────┐                                         │
│     │ Mix (50/50)  │ ────► rawInstrumentSignal             │
│     └──────┬───────┘                                         │
│            │                                                  │
│  2. ADSR ENVELOPE                                            │
│            ▼                                                  │
│     ┌──────────────┐                                         │
│     │ Attack/Release│ ────► instrumentSignal                │
│     │ (smooth)     │                                         │
│     └──────┬───────┘                                         │
│            │                                                  │
│  3. FILTER (Instrument Channel)                             │
│            ▼                                                  │
│     ┌──────────────┐                                         │
│     │ Low-Pass    │ ────► filteredInstrumentLeft/Right     │
│     │ Biquad      │                                         │
│     └──────┬──────┘                                         │
│            │                                                  │
│            ├─────────────────────────────────┐               │
│            │                                 │               │
│  4. DELAY SEND                              │               │
│            ▼                                 │               │
│     ┌──────────────┐                        │               │
│     │ Send Level   │                        │               │
│     │ (30%)        │                        │               │
│     └──────┬───────┘                        │               │
│            │                                 │               │
│            ▼                                 │               │
│     ┌──────────────┐                        │               │
│     │ LFO Mod      │                        │               │
│     │ (delay time) │                        │               │
│     └──────┬───────┘                        │               │
│            │                                 │               │
│            ▼                                 │               │
│     ┌──────────────┐                        │               │
│     │ Delay Buffer │                        │               │
│     │ (Stereo)     │                        │               │
│     └──────┬───────┘                        │               │
│            │                                 │               │
│            │ Feedback (60%)                  │               │
│            │◄────────────────────────────────┘               │
│            │                                                  │
│            ▼                                                  │
│     ┌──────────────┐                                         │
│     │ Delay Return│ ────► delayReturnLeft/Right          │
│     │ (No Filter)  │                                         │
│     └──────┬───────┘                                         │
│            │                                                  │
│  5. MASTER MIX                                               │
│            │                                                  │
│     ┌──────▼──────────────────┐                             │
│     │ Dry + Wet               │ ────► masterLeft/Right      │
│     │ (filtered + delay)      │                             │
│     └──────┬───────────────────┘                             │
│            │                                                  │
│  6. SAFETY LIMITER                                           │
│            ▼                                                  │
│     ┌──────────────┐                                         │
│     │ Hard Clip    │ ────► Protect DAC & ears                │
│     │ (-1.0 to 1.0)│                                         │
│     └──────┬───────┘                                         │
│            │                                                  │
│  7. AUDIO OUTPUT                                             │
│            ▼                                                  │
│     ┌──────────────┐                                         │
│     │ I2S / DAC    │ ────► PCM5102 DAC                      │
│     │ (Stereo)     │                                         │
│     └──────────────┘                                         │
└─────────────────────────────────────────────────────────────┘
```

### Key Design Decisions

1. Mixing Desk Architecture: The filter affects the instrument channel BEFORE it is sent to delay. The delay return is unfiltered, just like a DAW send/return channel.

2. ADSR Envelope: Provides smooth attack (5ms) and release (50ms) to prevent clicks and pops. Always generates signal, envelope controls amplitude.

3. LFO Modulation: LFO modulates delay time symmetrically around base delay time. Depth of 100ms means ±50ms modulation.

4. Stereo Width: The delay has slightly different delay times for left and right channels (delay ± variance), creating stereo width.

5. Sample Rate Conversion: The delay can run at a lower sample rate (22050 Hz) than the main audio (44100 Hz) to save memory, with automatic upsampling/downsampling.

6. Variable Read Speed: Delay read position advances at variable speed to smoothly change delay time without clicks, creating tape delay effect.

## Module Descriptions

### `config.h`
Purpose: Centralized configuration file containing all constants, pin definitions, and parameters.

Why it exists: Makes it easy to tune the synthesizer without hunting through multiple files. All timing, frequency ranges, filter parameters, delay settings, etc. are in one place.

Key Constants:
- Audio: SAMPLE_RATE, MIN_FREQ_BASE, MIN_FREQ, MAX_FREQ, FIXED_VOLUME_PERCENT
- Delay: DELAY_TIME_MS_DEFAULT, DELAY_TIME_MS_MIN, DELAY_TIME_MS_MAX, DELAY_FEEDBACK, DELAY_SEND_LEVEL, DELAY_SAMPLE_RATE
- LFO: LFO_DEPTH_MS_MIN, LFO_DEPTH_MS_MAX, LFO_SPEED_HZ_MIN, LFO_SPEED_HZ_MAX
- Filter: FILTER_MIN_CUTOFF, FILTER_MAX_CUTOFF, FILTER_Q, FILTER_GAIN_COMP
- Base Tone: BASE_NOTE_DETENTS, BASE_NOTE_DEFAULT, WHOLE_TONE_RATIO
- Upper Tone: UPPER_TONE_DETENTS, UPPER_TONE_DEFAULT, UPPER_TONE_SEMITONES
- Hardware: I2S pins, touchscreen pins, display parameters

### `hardware/display.h/cpp`
Purpose: Manages the TFT display initialization, drawing, and refresh.

Why it exists: Encapsulates all display operations, making it easy to change display code without affecting other modules. Handles periodic redraws to prevent screen elements from being rubbed out.

Key Functions:
- `init()`: Initialize display and draw static elements
- `drawPentatonicZones()`: Draw dark green vertical lines for pentatonic scale zones
- `redrawStaticElements()`: Redraw all static elements (pentatonic dividers, divider line)
- `update()`: Periodic update (call from main loop)

Signal Flow: Display-only, no audio signal.

### `hardware/touch.h/cpp`
Purpose: Handles touchscreen initialization and reading.

Why it exists: Abstracts touchscreen hardware details. Returns normalized touch data that other modules can use without knowing about XPT2046 specifics.

Key Functions:
- `init()`: Initialize touchscreen SPI and calibration
- `read(screenWidth, screenHeight)`: Read current touch state, returns `TouchData` struct with `active`, `x`, `y`

Signal Flow: Input-only, no audio signal. Provides control data to main loop.

### `hardware/audio_output.h/cpp`
Purpose: Manages I2S/DAC initialization and audio output with safety limiting.

Why it exists: Encapsulates I2S configuration and provides a safe interface for audio output. Includes hard limiting to protect the DAC and user's ears.

Key Functions:
- `init()`: Initialize I2S driver, pin configuration, and start I2S
- `write(left, right)`: Write single stereo sample (with limiting)
- `writeBuffer(buffer, samples)`: Write buffer of interleaved samples (more efficient)
- `applyLimiter()`: Hard clip to -1.0 to 1.0 range (private)

Signal Flow: Final stage of audio pipeline. Receives processed audio and sends to DAC.

### `audio/oscillator.h/cpp`
Purpose: Generates dual oscillator voice (sine + sawtooth).

Why it exists: Encapsulates oscillator logic. The sawtooth interval is configurable (default 12 semitones = one octave). Easy to extend with more oscillator types.

Key Functions:
- `generate(frequency, active)`: Generate next sample. Always generates (envelope controls amplitude).
- `setSemitoneInterval(semitones)`: Set sawtooth interval in semitones (5, 7, 9, 12, or 14)

Signal Flow: First stage of audio pipeline. Generates raw instrument signal.

Technical Details:
- Sine: `sin(phase)` with phase increment based on frequency
- Sawtooth: `(phase / PI) - 1.0` at frequency * semitoneRatio
- Mix: Equal mix (50/50) of both oscillators
- Semitone ratio: Pre-calculated with `pow()` when interval changes, cached to avoid expensive operations in audio loop

### `audio/adsr.h/cpp`
Purpose: ADSR envelope generator for smooth note transitions.

Why it exists: Prevents clicks and pops when starting/stopping notes. Provides smooth attack and release curves.

Key Functions:
- `process(gate)`: Process envelope, returns multiplier (0.0 to 1.0). Gate is true when touching.

Signal Flow: Applied to oscillator output before filter. Controls amplitude smoothly.

Technical Details:
- Attack: 5ms (hardcoded for now)
- Decay: 0ms (no decay stage)
- Sustain: 1.0 (full sustain, hardcoded for now)
- Release: 50ms (hardcoded for now)
- State machine: IDLE, ATTACK, DECAY, SUSTAIN, RELEASE
- Future: Parameters can be made runtime-controllable via setAttack(), setDecay(), setSustain(), setRelease()

### `audio/filter.h/cpp`
Purpose: Biquad low-pass filter with resonance (Q) and gain compensation.

Why it exists: Encapsulates filter logic. Can be easily replaced with different filter types. Per-channel instances (left/right) for stereo processing.

Key Functions:
- `setCutoff(cutoff)`: Update filter cutoff frequency (only recalculates if change is significant)
- `process(input)`: Process a single sample through the filter

Signal Flow: Applied to instrument channel after envelope, before delay send. Does NOT affect delay return.

Technical Details:
- Biquad filter (2nd order IIR)
- Q (resonance) set to 1.5 for classic filter sound
- Gain compensation (0.7) prevents resonant peak from causing volume jumps
- Coefficient recalculation only when cutoff changes by >1.0 Hz (optimization)

### `audio/delay.h/cpp`
Purpose: Stereo delay effect with feedback, send level, LFO modulation, and sample rate conversion.

Why it exists: Encapsulates delay logic. Handles complex sample rate conversion, stereo width, variable read speed, and circular buffer management.

Key Functions:
- `init()`: Allocate delay buffer and initialize read/write positions
- `process(inputLeft, inputRight, outputLeft, outputRight, delayTimeMs, delayVarianceMs)`: Process delay with feedback and variable delay time

Signal Flow: Receives filtered instrument signal (via send level), mixes with feedback, and outputs delayed signal. Delay return is mixed with dry signal at master output.

Technical Details:
- Circular buffer using `int16_t` (memory optimization)
- Stereo width: Left channel = delay - variance, Right channel = delay + variance
- Variable read speed: Read position advances at variable speed to smoothly change delay time without clicks
- Sample rate conversion: Can run at 22050 Hz while audio runs at 44100 Hz
- Feedback: Delay output feeds back into delay input (mono feedback from stereo average)
- Send level: Only a portion (30%) of filtered instrument goes to delay
- LFO modulation: Delay time is modulated by LFO (symmetric around base delay time)

### `audio/lfo.h/cpp`
Purpose: Low frequency oscillator for modulating delay time.

Why it exists: Provides smooth modulation of delay time, creating chorus/flanger-like effects.

Key Functions:
- `setSpeed(frequencyHz)`: Set LFO speed (0.1 to 50 Hz)
- `process()`: Generate next sample, returns -1.0 to 1.0 (sine wave)

Signal Flow: Used in audio task to modulate delay time. LFO output is scaled by depth and added to base delay time.

Technical Details:
- Sine wave generation using phase accumulator
- Speed: 0.1 Hz to 50 Hz (logarithmic control)
- Depth: 0ms to 100ms (represents total peak-to-peak range, so ±50ms at max depth)
- Modulation: `delayTime = baseDelayTime + (lfoValue * lfoDepth / 2.0)`

### `ui/pentatonic_ui.h/cpp`
Purpose: Handles pentatonic scale quantization, touch-to-frequency mapping, zone visualization, and base frequency adjustment.

Why it exists: Encapsulates all pentatonic logic and UI drawing. Makes it easy to change scale or visualization without affecting audio code.

Key Functions:
- `touchToFrequency(touchX, screenWidth)`: Convert touch X position to quantized pentatonic frequency
- `touchToFilterCutoff(touchY, screenHeight)`: Convert touch Y position to filter cutoff (top half of screen)
- `getZone(frequency)`: Get pentatonic zone index for a frequency
- `getZoneBounds(frequency, screenWidth)`: Get screen boundaries for a zone
- `updateDisplay(tft, touching, zoneIndex, frequency)`: Update zone highlighting on display
- `setBaseFrequency(baseFreq)`: Update base frequency when base tone slider changes

Signal Flow: Control-only, no audio signal. Provides frequency and filter cutoff values based on touch input.

Technical Details:
- Pentatonic scale: Root, Minor 3rd, Perfect 4th, Perfect 5th, Minor 7th
- Logarithmic frequency mapping (matches human perception)
- Base frequency adjustable via base tone slider (whole tone steps)
- Zone highlighting: Yellow fill when active, dark green outline when inactive

### `ui/parameter_control.h/cpp`
Purpose: Handles parameter sliders in the bottom half of the screen.

Why it exists: Encapsulates all parameter control UI and value mapping. Makes it easy to add new parameters or change control behavior.

Key Functions:
- `touchToParameter(touchX, touchY, screenWidth, screenHeight)`: Determine which parameter is being touched
- `touchToDelayTime(touchY, screenHeight)`: Convert touch Y to delay time (logarithmic, 50-1000ms)
- `touchToLFODepth(touchY, screenHeight)`: Convert touch Y to LFO depth (linear, 0-100ms)
- `touchToLFOSpeed(touchY, screenHeight)`: Convert touch Y to LFO speed (logarithmic, 0.1-50Hz)
- `touchToBaseNote(touchY, screenHeight)`: Convert touch Y to base note detent (0-4)
- `touchToUpperTone(touchY, screenHeight)`: Convert touch Y to upper tone detent (0-4)
- `drawControls(tft)`: Draw initial parameter controls
- `updateParameterDisplay(tft, param, uiValue, actualValue)`: Update parameter display with current values

Signal Flow: Control-only, no audio signal. Provides parameter values based on touch input.

Technical Details:
- 5 vertical sliders in bottom half of screen
- Each slider shows actual value (red) centered below slider
- Base tone and upper tone sliders show detent dots
- Smoothing applied to UI values for smooth slider movement

### `main.cpp`
Purpose: Main entry point - orchestrates all modules.

Why it exists: Provides high-level control flow. Easy to read and understand the overall system behavior.

Key Responsibilities:
1. Initialize all hardware (display, touch, audio, delay, envelope)
2. Create audio task on Core 0
3. Main loop (Core 1): Read touch, update control values, update display

Signal Flow: Orchestrates the entire system. Reads touch input, updates control values (frequency, filter cutoff, delay time, LFO parameters, base tone, upper tone), and manages display updates.

## Thread Safety

The system uses FreeRTOS tasks running on different CPU cores:

- Core 0 (Audio Task): Generates audio continuously. Reads `volatile` control variables set by main loop.

- Core 1 (Main Loop): Handles touch input and display updates. Writes to `volatile` control variables.

Volatile Variables:
- `currentFreq`: Current pitch (quantized to pentatonic)
- `currentFilterCutoff`: Current filter cutoff frequency
- `isTouching`: Whether screen is currently being touched
- `currentDelayTimeMs`: Current delay time (smoothed)
- `targetDelayTimeMs`: Target delay time (from touch input)
- `actualDelayTimeMs`: Actual delay time being used (with LFO modulation)
- `currentLFODepthMs`: Current LFO depth (smoothed)
- `targetLFODepthMs`: Target LFO depth (from touch input)
- `actualLFODepthMs`: Actual LFO depth being used
- `currentLFOSpeedHz`: Current LFO speed (smoothed)
- `targetLFOSpeedHz`: Target LFO speed (from touch input)
- `actualLFOSpeedHz`: Actual LFO speed being used
- `currentBaseNoteDetent`: Current base note detent (0-4)
- `currentUpperToneDetent`: Current upper tone detent (0-4)

These are read once per audio buffer (64 samples) to minimize race conditions.

## Memory Considerations

The delay buffer is the largest memory consumer:
- Size: `(DELAY_TIME_MS_MAX + DELAY_TIME_VARIANCE_MS_MAX) * DELAY_SAMPLE_RATE / 1000.0` samples
- Type: `int16_t` (2 bytes per sample) instead of `float` (4 bytes) to save memory
- Current: ~11.6 KB at 22050 Hz sample rate with 1000ms max delay

Other optimizations:
- Filter coefficients only recalculated when cutoff changes significantly
- Display updates decoupled from touch polling
- Static elements redrawn periodically instead of every frame
- Semitone ratio pre-calculated (no `pow()` in audio loop)
- LFO phase accumulator (no expensive operations per sample)

## Extending the System

### Adding a New Oscillator Type
1. Add oscillator generation code to `oscillator.cpp`
2. Add mix parameter to control balance
3. Update `generate()` function

### Adding a New Effect
1. Create new file in `audio/` directory (e.g., `reverb.h/cpp`)
2. Follow delay pattern: `process(input, output)` function
3. Insert in signal chain in `audioTask()` (before or after filter/delay)

### Changing the Scale
1. Update `PENTATONIC_RATIOS` in `config.h`
2. Update `NUM_PENTATONIC_ZONES` if needed
3. Visualization will automatically update

### Changing Display Layout
1. Modify `display.cpp` drawing functions
2. Update `pentatonic_ui.cpp` and `parameter_control.cpp` visualization if needed
3. No audio code changes required

### Making ADSR Parameters Runtime-Controllable
1. Add volatile variables for ADSR parameters in `main.cpp`
2. Add parameter sliders in `parameter_control.cpp`
3. Call `envelope.setAttack()`, `setDecay()`, `setSustain()`, `setRelease()` when values change
4. Update display to show ADSR values

## Summary

This modular architecture provides:
- Separation of Concerns: Each module has a single, clear responsibility
- Easy Testing: Modules can be tested independently
- Maintainability: Changes are isolated to specific modules
- Readability: `main.cpp` is now a high-level orchestration file
- Extensibility: Easy to add new oscillators, effects, or UI elements

The signal flow follows a professional mixing desk architecture, making it intuitive for anyone familiar with DAW workflows.

Performance optimizations ensure smooth real-time audio processing on the ESP32, with careful attention to avoiding expensive operations in audio loops.

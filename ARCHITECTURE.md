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
│   ├── filter.h/cpp           # Biquad low-pass filter
│   └── delay.h/cpp            # Stereo delay effect
└── ui/
    └── pentatonic_ui.h/cpp    # Pentatonic scale quantization and visualization
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
│     │ (freq * 2)  │                                          │
│     └──────┬──────┘                                          │
│            │                                                  │
│            ▼                                                  │
│     ┌──────────────┐                                         │
│     │ Mix (50/50)  │ ────► instrumentSignal                 │
│     └──────┬───────┘                                         │
│            │                                                  │
│  2. FILTER (Instrument Channel)                              │
│            ▼                                                  │
│     ┌──────────────┐                                         │
│     │ Low-Pass    │ ────► filteredInstrumentLeft/Right     │
│     │ Biquad      │                                         │
│     └──────┬──────┘                                         │
│            │                                                  │
│            ├─────────────────────────────────┐               │
│            │                                 │               │
│  3. DELAY SEND                              │               │
│            ▼                                 │               │
│     ┌──────────────┐                        │               │
│     │ Send Level   │                        │               │
│     │ (70%)        │                        │               │
│     └──────┬───────┘                        │               │
│            │                                 │               │
│            ▼                                 │               │
│     ┌──────────────┐                        │               │
│     │ Delay Buffer │                        │               │
│     │ (Stereo)     │                        │               │
│     └──────┬───────┘                        │               │
│            │                                 │               │
│            │ Feedback (90%)                  │               │
│            │◄────────────────────────────────┘               │
│            │                                                  │
│            ▼                                                  │
│     ┌──────────────┐                                         │
│     │ Delay Return│ ────► delayReturnLeft/Right          │
│     │ (No Filter)  │                                         │
│     └──────┬───────┘                                         │
│            │                                                  │
│  4. MASTER MIX                                               │
│            │                                                  │
│     ┌──────▼──────────────────┐                             │
│     │ Dry + Wet               │ ────► masterLeft/Right      │
│     │ (filtered + delay)      │                             │
│     └──────┬───────────────────┘                             │
│            │                                                  │
│  5. SAFETY LIMITER                                           │
│            ▼                                                  │
│     ┌──────────────┐                                         │
│     │ Hard Clip    │ ────► Protect DAC & ears                │
│     │ (-1.0 to 1.0)│                                         │
│     └──────┬───────┘                                         │
│            │                                                  │
│  6. AUDIO OUTPUT                                             │
│            ▼                                                  │
│     ┌──────────────┐                                         │
│     │ I2S / DAC    │ ────► PCM5102 DAC                      │
│     │ (Stereo)     │                                         │
│     └──────────────┘                                         │
└─────────────────────────────────────────────────────────────┘
```

### Key Design Decisions

1. **Mixing Desk Architecture**: The filter affects the instrument channel BEFORE it's sent to delay. The delay return is unfiltered, just like a DAW send/return channel.

2. **Touch-Only Generation**: Audio is only generated when the screen is touched (`isTouching` flag). However, the delay continues to echo even after touch ends, creating natural decay.

3. **Stereo Width**: The delay has slightly different delay times for left and right channels (delay ± variance), creating stereo width.

4. **Sample Rate Conversion**: The delay can run at a lower sample rate (22050 Hz) than the main audio (44100 Hz) to save memory, with automatic upsampling/downsampling.

## Module Descriptions

### `config.h`
**Purpose**: Centralized configuration file containing all constants, pin definitions, and parameters.

**Why it exists**: Makes it easy to tune the synthesizer without hunting through multiple files. All timing, frequency ranges, filter parameters, delay settings, etc. are in one place.

**Key Constants**:
- Audio: `SAMPLE_RATE`, `MIN_FREQ`, `MAX_FREQ`, `FIXED_VOLUME_PERCENT`
- Delay: `DELAY_TIME_MS`, `DELAY_FEEDBACK`, `DELAY_SEND_LEVEL`, `DELAY_SAMPLE_RATE`
- Filter: `FILTER_MIN_CUTOFF`, `FILTER_MAX_CUTOFF`, `FILTER_Q`, `FILTER_GAIN_COMP`
- Hardware: I2S pins, touchscreen pins, display parameters

### `hardware/display.h/cpp`
**Purpose**: Manages the TFT display initialization, drawing, and refresh.

**Why it exists**: Encapsulates all display operations, making it easy to change display code without affecting other modules. Handles periodic redraws to prevent screen elements from being "rubbed out."

**Key Functions**:
- `init()`: Initialize display and draw static elements
- `drawDeadZones()`: Draw red rectangles for filter dead zones
- `drawPentatonicZones()`: Draw dark green vertical lines for pentatonic scale zones
- `redrawStaticElements()`: Redraw all static elements (dead zones, pentatonic dividers, status text)
- `update()`: Periodic update (call from main loop)

**Signal Flow**: Display-only, no audio signal.

### `hardware/touch.h/cpp`
**Purpose**: Handles touchscreen initialization and reading.

**Why it exists**: Abstracts touchscreen hardware details. Returns normalized touch data that other modules can use without knowing about XPT2046 specifics.

**Key Functions**:
- `init()`: Initialize touchscreen SPI and calibration
- `read(screenWidth, screenHeight)`: Read current touch state, returns `TouchData` struct with `active`, `x`, `y`

**Signal Flow**: Input-only, no audio signal. Provides control data to main loop.

### `hardware/audio_output.h/cpp`
**Purpose**: Manages I2S/DAC initialization and audio output with safety limiting.

**Why it exists**: Encapsulates I2S configuration and provides a safe interface for audio output. Includes hard limiting to protect the DAC and user's ears.

**Key Functions**:
- `init()`: Initialize I2S driver, pin configuration, and start I2S
- `write(left, right)`: Write single stereo sample (with limiting)
- `writeBuffer(buffer, samples)`: Write buffer of interleaved samples (more efficient)
- `applyLimiter()`: Hard clip to -1.0 to 1.0 range (private)

**Signal Flow**: Final stage of audio pipeline. Receives processed audio and sends to DAC.

### `audio/oscillator.h/cpp`
**Purpose**: Generates dual oscillator voice (sine + sawtooth).

**Why it exists**: Encapsulates oscillator logic. The sawtooth is always one octave above the sine (2x frequency). Easy to extend with more oscillator types.

**Key Functions**:
- `generate(frequency, active)`: Generate next sample. Returns 0.0 if not active (prevents phase jumps).

**Signal Flow**: First stage of audio pipeline. Generates raw instrument signal.

**Technical Details**:
- Sine: `sin(phase)` with phase increment based on frequency
- Sawtooth: `(phase / PI) - 1.0` at 2x frequency
- Mix: Equal mix (50/50) of both oscillators
- Phase reset: When not active, phases reset to 0.0 to prevent jumps when resuming

### `audio/filter.h/cpp`
**Purpose**: Biquad low-pass filter with resonance (Q) and gain compensation.

**Why it exists**: Encapsulates filter logic. Can be easily replaced with different filter types. Per-channel instances (left/right) for stereo processing.

**Key Functions**:
- `setCutoff(cutoff)`: Update filter cutoff frequency (only recalculates if change is significant)
- `process(input)`: Process a single sample through the filter

**Signal Flow**: Applied to instrument channel before delay send. Does NOT affect delay return.

**Technical Details**:
- Biquad filter (2nd order IIR)
- Q (resonance) set to 1.5 for classic filter sound
- Gain compensation (0.7) prevents resonant peak from causing volume jumps
- Coefficient recalculation only when cutoff changes by >1.0 Hz (optimization)

### `audio/delay.h/cpp`
**Purpose**: Stereo delay effect with feedback, send level, and sample rate conversion.

**Why it exists**: Encapsulates delay logic. Handles complex sample rate conversion, stereo width, and circular buffer management.

**Key Functions**:
- `init()`: Allocate delay buffer and initialize read/write positions
- `process(inputLeft, inputRight, outputLeft, outputRight)`: Process delay with feedback

**Signal Flow**: Receives filtered instrument signal (via send level), mixes with feedback, and outputs delayed signal. Delay return is mixed with dry signal at master output.

**Technical Details**:
- Circular buffer using `int16_t` (memory optimization)
- Stereo width: Left channel = delay - variance, Right channel = delay + variance
- Sample rate conversion: Can run at 22050 Hz while audio runs at 44100 Hz
- Feedback: Delay output feeds back into delay input (mono feedback from stereo average)
- Send level: Only a portion (70%) of filtered instrument goes to delay

### `ui/pentatonic_ui.h/cpp`
**Purpose**: Handles pentatonic scale quantization, touch-to-frequency mapping, and zone visualization.

**Why it exists**: Encapsulates all pentatonic logic and UI drawing. Makes it easy to change scale or visualization without affecting audio code.

**Key Functions**:
- `touchToFrequency(touchX, screenWidth)`: Convert touch X position to quantized pentatonic frequency
- `touchToFilterCutoff(touchY, screenHeight)`: Convert touch Y position to filter cutoff (with dead zones)
- `getZone(frequency)`: Get pentatonic zone index for a frequency
- `getZoneBounds(frequency, screenWidth)`: Get screen boundaries for a zone
- `updateDisplay(tft, touching, zoneIndex, frequency)`: Update zone highlighting on display

**Signal Flow**: Control-only, no audio signal. Provides frequency and filter cutoff values based on touch input.

**Technical Details**:
- Pentatonic scale: Root, Minor 3rd, Perfect 4th, Perfect 5th, Minor 7th
- Logarithmic frequency mapping (matches human perception)
- Dead zones: Top 20px = filter open, Bottom 20px = filter closed
- Zone highlighting: Yellow fill when active, dark green outline when inactive

### `main.cpp`
**Purpose**: Main entry point - orchestrates all modules.

**Why it exists**: Provides high-level control flow. Easy to read and understand the overall system behavior.

**Key Responsibilities**:
1. Initialize all hardware (display, touch, audio)
2. Create audio task on Core 0
3. Main loop (Core 1): Read touch, update control values, update display

**Signal Flow**: Orchestrates the entire system. Reads touch input, updates control values (frequency, filter cutoff), and manages display updates.

## Thread Safety

The system uses FreeRTOS tasks running on different CPU cores:

- **Core 0 (Audio Task)**: Generates audio continuously. Reads `volatile` control variables (`currentFreq`, `currentFilterCutoff`, `isTouching`) set by main loop.

- **Core 1 (Main Loop)**: Handles touch input and display updates. Writes to `volatile` control variables.

**Volatile Variables**:
- `currentFreq`: Current pitch (quantized to pentatonic)
- `currentFilterCutoff`: Current filter cutoff frequency
- `isTouching`: Whether screen is currently being touched

These are read once per audio buffer (64 samples) to minimize race conditions.

## Memory Considerations

The delay buffer is the largest memory consumer:
- Size: `(DELAY_TIME_MS + DELAY_TIME_VARIANCE_MS) * DELAY_SAMPLE_RATE / 1000.0` samples
- Type: `int16_t` (2 bytes per sample) instead of `float` (4 bytes) to save memory
- Current: ~11.6 KB at 22050 Hz sample rate

Other optimizations:
- Filter coefficients only recalculated when cutoff changes significantly
- Display updates decoupled from touch polling
- Static elements redrawn periodically instead of every frame

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
2. Update `pentatonic_ui.cpp` visualization if needed
3. No audio code changes required

## Summary

This modular architecture provides:
- **Separation of Concerns**: Each module has a single, clear responsibility
- **Easy Testing**: Modules can be tested independently
- **Maintainability**: Changes are isolated to specific modules
- **Readability**: `main.cpp` is now a high-level orchestration file
- **Extensibility**: Easy to add new oscillators, effects, or UI elements

The signal flow follows a professional mixing desk architecture, making it intuitive for anyone familiar with DAW workflows.


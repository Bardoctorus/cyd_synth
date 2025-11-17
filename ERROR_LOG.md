# Error Log - CYD Synthesizer Project

This document catalogs all errors encountered during development, their causes, and the fixes applied. 

**This will probably get deleted, it's just a neat summary of things that went wrong while getting up and running**

---

## 1. TFT_WIDTH and TFT_HEIGHT Not Declared

**Error:**
```
'TFT_WIDTH' was not declared in this scope
'TFT_HEIGHT' was not declared in this scope
```

**Location:** `TFT_eSPI` library compilation

**Cause:** The `platformio.ini` file had `-DUSER_SETUP_LOADED=1` in the `build_flags`. This flag prevents the TFT_eSPI library from including the custom `User_Setup.h` file where `TFT_WIDTH` and `TFT_HEIGHT` are defined.

**Fix:** Removed the `-DUSER_SETUP_LOADED=1` flag from `platformio.ini`. The `extra_scripts.py` script already handles copying the custom `User_Setup.h` to the library directory, so the flag was unnecessary and counterproductive.

---

## 2. No Audio Output from DAC

**Error:** PCM5102 DAC producing no sound despite I2S appearing to be configured correctly.

**Cause:** Multiple potential issues:
- I2S buffer configuration (mono vs stereo)
- Communication format mismatch
- Missing continuous data stream (PCM5102 requires continuous data)
- APLL clock issues with some PCM5102 modules

**Fix:** 
- Changed audio buffer to stereo interleaved format
- Updated I2S communication format to `I2S_COMM_FORMAT_STAND_I2S`
- Removed `vTaskDelay(1)` from audio task (PCM5102 needs continuous stream)
- Disabled APLL (some PCM5102 modules are sensitive to clock)
- Added explicit `i2s_start()` call
- Increased `AMPLITUDE` constant

---

## 3. Touchscreen Not Registering Touch

**Error:** Touchscreen not responding to touch input.

**Cause:** The XPT2046 touchscreen requires a separate SPI bus with explicit pin configuration. The initial implementation was trying to share the display's SPI bus or didn't configure all required pins.

**Fix:**
- Created separate `SPIClass` instance (`touchscreenSPI`)
- Defined all XPT2046 pins explicitly:
  - XPT2046_IRQ: 36
  - XPT2046_MOSI: 32
  - XPT2046_MISO: 39
  - XPT2046_CLK: 25
  - XPT2046_CS: 33
- Initialized `touchscreenSPI` with explicit pins
- Updated touch detection to use `ts.tirqTouched() && ts.touched()`
- Adjusted touch coordinate mapping with calibrated values

---

## 4. I2S Pin Conflicts

**Error:** I2S pins conflicting with other hardware.

**Issues Found:**
- **IO35**: Input-only pin, cannot be used for I2S LRCK output
- **IO21**: Conflicts with TFT backlight control

**Cause:** Initial pin assignments didn't account for hardware limitations and conflicts on the CYD board.

**Fix:** Updated I2S pin assignments:
- `I2S_BCK_PIN`: Changed from 21 to 26 (then later to 1 for JST connector compatibility)
- `I2S_LRCK_PIN`: Changed from 35 to 27 (CN1 connector)
- `I2S_DATA_PIN`: Remained 22

---

## 5. IO26 Speaker Connector Cannot Be Used as GPIO

**Error:** Attempted to use IO26 (speaker connector) as I2S BCK pin.

**Cause:** The pin documentation specifically stated that the speaker connector cannot be used as a GPIO, but this was overlooked in initial pin assignment.

**Fix:** Changed `I2S_BCK_PIN` from IO26 to IO1 (TX on P1 connector), and switched debug output from `Serial` to `Serial2` to free up IO1.

---

## 6. Volume Control Scaling Issues

**Error:** Volume control felt "off" - massive jump from dead zone, little difference between min/max.

**Cause:** 
- Linear scaling doesn't match human perception (logarithmic)
- Dead zone implementation had inverted middle section
- Volume mapping was incorrect

**Fix:**
- Implemented logarithmic scaling for volume
- Fixed dead zone inversion (middle section was scaling in wrong direction)
- Adjusted logarithmic mapping in middle area (1% to 88% range for smoother curve)

---

## 7. Filter Sounded Like Volume Control

**Error:** Low-pass filter sounded more like a volume control than a resonant filter.

**Cause:** 
- Filter Q (resonance) was too high (5.0), causing sudden volume jumps
- No gain compensation at resonance peak
- Filter cutoff changes were too abrupt

**Fix:**
- Reduced `FILTER_Q` from 5.0 to 2.5
- Added `FILTER_GAIN_COMP` (0.7) to control resonant peak volume boost
- Added threshold check before recalculating coefficients to smooth transitions

---

## 8. Pitch Quantization Issues

**Error:** Pitch didn't match visual zones, sounded constant despite zone changes.

**Cause:** Frequency wasn't being quantized to pentatonic scale - raw frequency was being used instead of quantized value.

**Fix:**
- Implemented `quantizeToPentatonic()` function to snap frequency to nearest pentatonic note
- Applied quantization in `touchToFrequency()` before returning value
- Ensured audio frequency matches visual zone highlighting

---

## 9. Memory Overflow (dram0_0_seg)

**Error:**
```
dram0_0_seg' overflowed by 58640 bytes
```

**Cause:** Delay buffer was too large:
- 400ms delay at 44100 Hz = ~35KB per channel
- Using `float` (4 bytes) instead of `int16_t` (2 bytes) doubled memory usage
- Stereo delay with variance required even more memory

**Fix:**
- Reduced delay time from 400ms to 250ms
- Changed delay buffer from `float[]` to `int16_t[]` (halved memory usage)
- Reduced delay variance from 55ms to 20ms
- Introduced `DELAY_SAMPLE_RATE` (22050 Hz) to further reduce memory (optional)

---

## 10. Delay Signal Flow Issues

**Error:** Filter no longer affected dry tone, delay was still being filtered after the fact.

**Cause:** Audio signal flow was incorrect - filter was being applied to delay return instead of instrument channel.

**Fix:** Refactored to "mixing desk architecture":
- Instrument signal → Filter → Delay send
- Delay return (unfiltered) → Master mix
- Filtered instrument (dry) + Delay return → Master output

---

## 11. Pitch Drop Implementation Caused Noise

**Error:** Attempted to implement tape-style pitch drop on delay repetitions, resulted in "extremely nasty noise."

**Cause:** Fractional read positions and variable speed calculations were causing buffer read errors and audio artifacts.

**Fix:** Completely reverted pitch drop implementation, restored simple integer-based delay read logic.

---

## 12. Text Not Refreshing on Screen

**Error:** Screen text getting "rubbed off" and not refreshing.

**Cause:** Text elements weren't being redrawn in the periodic static redraw loop.

**Fix:** Added text redraw calls to `STATIC_REDRAW_INTERVAL` block in `loop()` function.

---

## 13. Build Errors After Refactoring

**Multiple Errors:**

### 13a. PI Not Declared
**Error:** `'PI' was not declared in this scope`

**Cause:** `PI` was defined in `main.cpp` but not available in new module files after refactoring.

**Fix:** Added `#define PI` with guard to `config.h`.

### 13b. int16_t Not a Type
**Error:** `'int16_t' does not name a type`

**Cause:** Missing `#include <stdint.h>` in `delay.h`.

**Fix:** Added `#include <stdint.h>` to `delay.h`.

### 13c. Delay Name Conflict
**Error:** Ambiguity with `delay()` function

**Cause:** Global `delay` instance conflicted with Arduino's `delay()` function.

**Fix:** Renamed global instance from `delay` to `stereoDelay`.

### 13d. STATIC_REDRAW_INTERVAL Conflict
**Error:** Multiple definition of `STATIC_REDRAW_INTERVAL`

**Cause:** Constant was defined in both `config.h` and `display.h`.

**Fix:** Removed duplicate definition from `display.h`, kept only in `config.h`.

---

## 14. Modulo Operator with Float

**Error:**
```
invalid operands of types 'float' and 'int' to binary 'operator%'
```

**Location:** `src/audio/delay.cpp:38:73`

**Cause:** Attempting to use modulo operator (`%`) directly with `float` values. C++ modulo operator only works with integer types.

**Fix:** Cast `float` values to `int` before modulo operation, then cast result back to `float`:
```cpp
int delayLeftSamplesInt = (int)delayLeftSamples;
delayReadPosLeft = (float)((DELAY_BUFFER_SIZE - delayLeftSamplesInt) % DELAY_BUFFER_SIZE);
```

---

## 15. DELAY_TIME_MS Not Declared

**Error:**
```
'DELAY_TIME_MS' was not declared in this scope
```

**Location:** `src/main.cpp:59` (debug output)

**Cause:** After refactoring, `DELAY_TIME_MS` was removed as a direct `#define` and replaced with `DELAY_TIME_MS_DEFAULT`.

**Fix:** Changed debug `Serial2.printf` to use `DELAY_TIME_MS_DEFAULT` instead of `DELAY_TIME_MS`.

---

## 16. LFO Depth Calculation Error

**Error:** LFO depth of 100ms was adding ±100ms instead of ±50ms to delay time.

**Cause:** LFO output (-1.0 to 1.0) was being scaled by full depth, but user wanted depth to represent total peak-to-peak range.

**Fix:** Changed calculation to use half the depth for symmetric modulation:
```cpp
float modulationAmount = lfoValue * (lfoDepth / 2.0);  // Half depth for ±range
```

---

## 17. Performance Issue: pow() in Audio Loop

**Error:** Crackling audio, MCU hangs and resets on boot.

**Cause:** `pow(2.0, semitoneInterval / 12.0)` was being called inside `oscillator.generate()`, which runs 64 times per audio buffer. This is 64 expensive floating-point power operations per buffer, overwhelming the CPU.

**Fix:**
- Moved `pow()` calculation out of audio loop
- Calculate semitone ratio once when interval changes (in `setSemitoneInterval()`)
- Store pre-calculated `semitoneRatio` in oscillator class
- Use simple multiplication in audio loop: `frequency * semitoneRatio`

**Performance Impact:** Reduced from 64 `pow()` calls per buffer to 1 `pow()` call only when user changes upper tone slider.

---

## 18. Switch Case Variable Initialization Error

**Error:**
```
jump to case label [-fpermissive]
crosses initialization of 'float releaseSamples'
```

**Location:** `src/audio/adsr.cpp:125`

**Cause:** In C++, you cannot declare variables in a switch case without wrapping them in braces. The variable `releaseSamples` was declared in a case statement, and the compiler complained about crossing initialization when jumping to the next case label.

**Fix:** Wrapped the variable declaration in braces to create a proper scope:
```cpp
case ATTACK:
case DECAY:
case SUSTAIN:
  state = RELEASE;
  {
    float releaseSamples = releaseMs * SAMPLE_RATE / 1000.0;
    // ... rest of code
  }
  break;
```

---

## Summary

Most errors fell into these categories:
1. **Configuration issues**: Pin conflicts, build flags, library setup
2. **Signal flow/logic errors**: Audio routing, scaling calculations
3. **Performance issues**: Expensive operations in audio loops
4. **C++ syntax errors**: Variable scoping, type mismatches
5. **Memory constraints**: Buffer size optimization

The key lesson: **Never call expensive functions (like `pow()`, `log()`, etc.) inside audio processing loops**. Always pre-calculate values and cache them.


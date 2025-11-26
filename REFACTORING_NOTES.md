# Refactoring Notes: Menu System and Future Modes

This document explains the refactoring done to prepare the codebase for different modes and configuration options.

## What Was Implemented

### 1. Menu System (`src/ui/menu.h/cpp`)
- Small menu button in bottom right corner (40x30 pixels)
- Placeholder menu screen with "MENU WILL BE HERE" text
- Back button to return to main interface
- Menu state management (open/closed)

### 2. Settings Structure (`src/config/settings.h`)
- `Settings` struct to hold all configuration options
- Scale types: Pentatonic, Chromatic, Major, Minor, Dorian, Mixolydian
- Pitch modes: Quantized (current) or Continuous
- Waveshape types: Sine, Sawtooth, Square, Triangle, Sine+Saw mix
- Effect flags: Delay, Distortion, Reverb, Chorus (bit flags for multiple)
- Parameter visibility flags: Which parameters are shown in bottom half

### 3. Scale Abstraction (`src/audio/scale.h/cpp`)
- Abstract `Scale` base class with virtual methods
- `PentatonicScale` implementation (current default)
- `ChromaticScale` implementation (12 semitones per octave)
- Factory function `createScale()` to create scales by type
- Easy to add more scales (Major, Minor, etc.) in the future

## How This Prepares for Future Features

### Scale Selection
- Current code uses `PentatonicUI` which is hardcoded to pentatonic
- Future: Replace `PentatonicUI` with a generic `ScaleUI` that uses the `Scale` abstraction
- Settings can switch between scales: `settings.scaleType = SCALE_CHROMATIC`

### Continuous Pitch Mode
- Current code quantizes to scale in `PentatonicUI::touchToFrequency()`
- Future: Add `if (settings.pitchMode == PITCH_MODE_CONTINUOUS)` to skip quantization
- Settings: `settings.pitchMode = PITCH_MODE_CONTINUOUS`

### Number of Notes/Octaves
- Settings structure has `numOctaves` field
- Future: Use this to adjust frequency range calculation
- Example: `settings.numOctaves = 3` would give 3 octaves instead of current 2

### Waveshape Selection
- Current `Oscillator` generates sine + sawtooth
- Settings structure has `primaryWaveshape` and `secondaryWaveshape`
- Future: Add waveshape generation methods to `Oscillator`:
  - `generateSine()`, `generateSawtooth()`, `generateSquare()`, `generateTriangle()`
  - `generate()` method selects based on `settings.primaryWaveshape` and `settings.secondaryWaveshape`
  - Mix controlled by `settings.waveshapeMix`

### Configurable Parameters
- Settings structure has `visibleParameters` bit flags
- Current `ParameterControl` draws all 5 parameters
- Future: Modify `ParameterControl::drawControls()` to check `settings.visibleParameters`
- Only draw parameters that are enabled
- Example: `settings.visibleParameters = PARAM_VISIBLE_DELAY_TIME | PARAM_VISIBLE_MASTER_GAIN`

### Effect Selection
- Settings structure has `activeEffects` bit flags
- Current code always uses delay
- Future: Check `settings.activeEffects & EFFECT_DELAY` before processing delay
- Add distortion, reverb, chorus effects as separate modules
- Enable/disable via settings flags

## Integration Points

### Main Loop (`src/main.cpp`)
- Menu state is checked first
- If menu is open, only handle menu interactions
- If menu is closed, handle normal interface
- Menu button touch is checked before other area checks
- Settings structure is declared but not yet used (ready for future)

### Display (`src/hardware/display.h/cpp`)
- Added `isInMenuButtonArea()` to prevent parameter controls from conflicting with menu button
- Menu button position is calculated in `init()`
- Menu button is drawn separately by `Menu` class

### Parameter Control (`src/ui/parameter_control.h/cpp`)
- Currently draws all 5 parameters
- Future: Check `settings.visibleParameters` to determine which to draw
- Slider count and positions will adjust dynamically

## Next Steps (Not Implemented Yet)

1. **Menu Implementation**: Replace placeholder with actual menu options
   - Scale selection dropdown/list
   - Pitch mode toggle
   - Parameter visibility checkboxes
   - Waveshape selection
   - Effect enable/disable

2. **Scale UI Refactoring**: Replace `PentatonicUI` with generic `ScaleUI`
   - Use `Scale` abstraction instead of hardcoded pentatonic
   - Support different scales based on settings

3. **Oscillator Refactoring**: Add waveshape selection
   - Implement square and triangle wave generation
   - Select waveshapes based on settings

4. **Parameter Control Refactoring**: Make parameters configurable
   - Check `settings.visibleParameters` when drawing
   - Adjust slider count and positions dynamically

5. **Effect System**: Add distortion, reverb, chorus
   - Create effect modules similar to delay
   - Enable/disable based on `settings.activeEffects`

6. **Settings Persistence**: Save settings to EEPROM/Flash
   - Load settings on startup
   - Save settings when changed in menu

## Current State

- Menu button works and shows placeholder screen
- Settings structure is defined and ready to use
- Scale abstraction is implemented (pentatonic and chromatic)
- Code is structured to easily add new features
- No breaking changes to existing functionality

All existing features continue to work as before. The refactoring is additive and prepares the codebase for future expansion.


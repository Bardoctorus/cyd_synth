/*
 * Pentatonic UI
 * Handles pentatonic scale quantization, zone calculation, and visualization
 */

#ifndef PENTATONIC_UI_H
#define PENTATONIC_UI_H

#include <TFT_eSPI.h>

struct PentatonicZone {
  int zoneIndex;      // Which pentatonic zone (0-4)
  int xStart;         // Screen X position of zone start
  int xEnd;           // Screen X position of zone end
};

class PentatonicUI {
public:
  PentatonicUI();
  
  // Number of visual keys in the current pentatonic layout
  int getTotalKeys(int screenWidth);
  
  // Convert touch position to a discrete key index (0 .. totalKeys-1)
  int touchToKeyIndex(int touchX, int screenWidth);
  
  // Convert a key index to frequency, using the current base frequency
  float keyIndexToFrequency(int keyIndex);
  
  // Set base frequency for pentatonic scale (called when base note slider changes)
  void setBaseFrequency(float baseFreq);
  
  // Convert touch position to filter cutoff
  float touchToFilterCutoff(int touchY, int screenHeight);
    
  // Update display with key highlighting (keyIndex is the visual key index)
  void updateDisplay(TFT_eSPI& tft, bool touching, int keyIndex);
  
private:
  float quantizeToPentatonic(float freq);
  int lastXStart, lastXEnd, lastZone;
  float currentBaseFreq;  // Current base frequency (adjusted by base note slider)
};

#endif // PENTATONIC_UI_H


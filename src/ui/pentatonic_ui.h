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
  
  // Convert touch position to frequency (quantized to pentatonic)
  float touchToFrequency(int touchX, int screenWidth);
  
  // Convert touch position to filter cutoff
  float touchToFilterCutoff(int touchY, int screenHeight);
  
  // Get pentatonic zone for a frequency
  int getZone(float frequency);
  
  // Get zone boundaries for visualization
  PentatonicZone getZoneBounds(float frequency, int screenWidth);
  
  // Update display with zone highlighting
  void updateDisplay(TFT_eSPI& tft, bool touching, int zoneIndex, float frequency);
  
private:
  float quantizeToPentatonic(float freq);
  int lastXStart, lastXEnd, lastZone;
};

#endif // PENTATONIC_UI_H


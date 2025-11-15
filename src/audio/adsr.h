/*
 * ADSR Envelope Generator
 * Attack-Decay-Sustain-Release envelope for smooth note transitions
 */

#ifndef ADSR_H
#define ADSR_H

class ADSR {
public:
  ADSR();
  
  // Process envelope - returns multiplier (0.0 to 1.0)
  float process(bool gate);
  
  // Set envelope parameters (for future real-time control)
  void setAttack(float attackMs);
  void setDecay(float decayMs);
  void setSustain(float sustainLevel);  // 0.0 to 1.0
  void setRelease(float releaseMs);
  
  // Get current envelope state
  enum State {
    IDLE,
    ATTACK,
    DECAY,
    SUSTAIN,
    RELEASE
  };
  State getState() { return state; }
  
private:
  // Envelope parameters (in milliseconds)
  float attackMs;
  float decayMs;
  float sustainLevel;  // 0.0 to 1.0
  float releaseMs;
  
  // Internal state
  State state;
  float envelopeValue;  // Current envelope value (0.0 to 1.0)
  float attackIncrement;
  float decayIncrement;
  float releaseIncrement;
  
  // Calculate increments based on sample rate
  void updateIncrements();
};

#endif // ADSR_H


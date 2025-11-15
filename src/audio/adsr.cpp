/*
 * ADSR Envelope Implementation
 */

#include "adsr.h"
#include "../config.h"
#include <math.h>

ADSR::ADSR() 
  : attackMs(5.0),      // Tiny attack (5ms) - hardcoded for now
    decayMs(0.0),       // No decay - hardcoded for now
    sustainLevel(1.0),  // Full sustain (100%) - hardcoded for now
    releaseMs(50.0),    // Little release (50ms) - hardcoded for now
    state(IDLE),
    envelopeValue(0.0) {
  updateIncrements();
}

void ADSR::updateIncrements() {
  // Calculate samples per stage
  float attackSamples = attackMs * SAMPLE_RATE / 1000.0;
  float decaySamples = decayMs * SAMPLE_RATE / 1000.0;
  float releaseSamples = releaseMs * SAMPLE_RATE / 1000.0;
  
  // Calculate increments per sample
  if (attackSamples > 0) {
    attackIncrement = 1.0 / attackSamples;  // Go from 0 to 1.0
  } else {
    attackIncrement = 1.0;  // Instant attack
  }
  
  if (decaySamples > 0) {
    decayIncrement = (1.0 - sustainLevel) / decaySamples;  // Go from 1.0 to sustainLevel
  } else {
    decayIncrement = 0.0;  // No decay
  }
  
  if (releaseSamples > 0) {
    releaseIncrement = sustainLevel / releaseSamples;  // Go from sustainLevel to 0.0
  } else {
    releaseIncrement = sustainLevel;  // Instant release
  }
}

void ADSR::setAttack(float attackMs) {
  this->attackMs = attackMs;
  updateIncrements();
}

void ADSR::setDecay(float decayMs) {
  this->decayMs = decayMs;
  updateIncrements();
}

void ADSR::setSustain(float sustainLevel) {
  this->sustainLevel = sustainLevel;
  updateIncrements();
}

void ADSR::setRelease(float releaseMs) {
  this->releaseMs = releaseMs;
  updateIncrements();
}

float ADSR::process(bool gate) {
  if (gate) {
    // Gate is ON (touching)
    switch (state) {
      case IDLE:
      case RELEASE:
        // Start attack
        state = ATTACK;
        envelopeValue = 0.0;
        break;
        
      case ATTACK:
        // Attack phase: 0.0 -> 1.0
        envelopeValue += attackIncrement;
        if (envelopeValue >= 1.0) {
          envelopeValue = 1.0;
          if (decayMs > 0.0) {
            state = DECAY;
          } else {
            state = SUSTAIN;
          }
        }
        break;
        
      case DECAY:
        // Decay phase: 1.0 -> sustainLevel
        envelopeValue -= decayIncrement;
        if (envelopeValue <= sustainLevel) {
          envelopeValue = sustainLevel;
          state = SUSTAIN;
        }
        break;
        
      case SUSTAIN:
        // Sustain phase: hold at sustainLevel
        envelopeValue = sustainLevel;
        break;
    }
  } else {
    // Gate is OFF (not touching)
    switch (state) {
      case IDLE:
        // Already idle, stay at 0
        envelopeValue = 0.0;
        break;
        
      case ATTACK:
      case DECAY:
      case SUSTAIN:
        // Start release from current value
        state = RELEASE;
        // Calculate release increment from current value
        {
          float releaseSamples = releaseMs * SAMPLE_RATE / 1000.0;
          if (releaseSamples > 0) {
            releaseIncrement = envelopeValue / releaseSamples;
          } else {
            releaseIncrement = envelopeValue;
          }
        }
        break;
        
      case RELEASE:
        // Release phase: current value -> 0.0
        envelopeValue -= releaseIncrement;
        if (envelopeValue <= 0.0) {
          envelopeValue = 0.0;
          state = IDLE;
        }
        break;
    }
  }
  
  return envelopeValue;
}


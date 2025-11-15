/*
 * Audio Output Hardware Implementation
 */

#include "audio_output.h"
#include "../config.h"
#include <Arduino.h>
#include <HardwareSerial.h>

AudioOutput::AudioOutput() : i2sNum(I2S_NUM) {
}

bool AudioOutput::init() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,  // Standard I2S format for PCM5102
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,  // High priority interrupt
    .dma_buf_count = 8,  // More buffers for smoother streaming
    .dma_buf_len = 128,  // 64 samples * 2 channels = 128 samples
    .use_apll = true,  // Use APLL for accurate clock
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };
  
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCK_PIN,
    .ws_io_num = I2S_LRCK_PIN,
    .data_out_num = I2S_DATA_PIN,
    .data_in_num = I2S_PIN_NO_CHANGE
  };
  
  // Install I2S driver
  esp_err_t err = i2s_driver_install(i2sNum, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    return false;
  }
  
  // Set pin configuration
  err = i2s_set_pin(i2sNum, &pin_config);
  if (err != ESP_OK) {
    return false;
  }
  
  // Clear DMA buffer and start I2S
  i2s_zero_dma_buffer(i2sNum);
  err = i2s_start(i2sNum);
  
  return (err == ESP_OK);
}

void AudioOutput::applyLimiter(float& left, float& right) {
  // Hard clip to -1.0 to 1.0 range (safety limit before DAC)
  // This prevents any possibility of clipping or damage
  // With proper gain staging, this should rarely be needed
  left = constrain(left, -1.0, 1.0);
  right = constrain(right, -1.0, 1.0);
}

void AudioOutput::write(float leftSample, float rightSample) {
  // Apply hard limiter (safety protection)
  applyLimiter(leftSample, rightSample);
  
  // Scale to fixed amplitude and convert to int16
  int16_t left = (int16_t)(leftSample * FIXED_AMPLITUDE);
  int16_t right = (int16_t)(rightSample * FIXED_AMPLITUDE);
  
  // Interleave samples
  int16_t buffer[2] = {left, right};
  size_t bytesWritten;
  i2s_write(i2sNum, buffer, sizeof(buffer), &bytesWritten, portMAX_DELAY);
}

void AudioOutput::writeBuffer(int16_t* buffer, size_t samples) {
  size_t bytesWritten;
  esp_err_t err = i2s_write(i2sNum, buffer, samples * sizeof(int16_t), &bytesWritten, portMAX_DELAY);
  if (err != ESP_OK) {
    Serial2.printf("I2S write error: %d\n", err);
  }
}


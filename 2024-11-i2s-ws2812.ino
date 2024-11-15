#include <Arduino.h>
#include <hardware/pio.h>
#include <hardware/dma.h>
#include "ws2812.pio.h"  // Include the generated PIO header file
#include <I2S.h>
#include "StarWars30.h"

#define NUM_STRIPS 3                   // Number of parallel LED strips
#define NUM_LEDS_PER_STRIP 60          // Number of LEDs per strip
#define PIO_PIN_BASE 20                // Starting pin for LED strips
#define LED_DATA_RATE 800000           // Data rate for WS2812 LEDs

// Audio setup
I2S i2s(OUTPUT);
const int sampleRate = 11025;

// LED setup
PIO pio = pio0;                        // Use PIO0
int sm = 0;                            // State machine index
uint dma_channel;         // DMA channels for each LED strip
uint32_t pixel_buffer[NUM_LEDS_PER_STRIP * 8]; // Buffer for each LED strip
volatile bool dma_busy = false; // Flags to track if DMA is in progress per row

unsigned long updateTime = 0;

void setup() {
  Serial.begin(9600);
  delay(500); // Allow some time for Serial to initialize

  i2s.setBCLK(24);
  i2s.setDATA(23);
  i2s.setBitsPerSample(16);
  i2s.setBuffers(4, 64);

  ledSetup();
}
 
void loop() {
  // updateAudio();
  updateLights();
}

bool playing = false;
const unsigned char *ptr = StarWars30_raw;
size_t toPlay = sizeof(StarWars30_raw);
size_t pos = 0;

float hue = 0.0;

void updateLights() {

  if (millis()-updateTime < 1000/60) return;

  updateTime = millis();

  fillHSV(hue, 1.0, 0.5);

  hue = hue+0.01;
  if (hue>=1.0) hue = 0.0;

  transferData();
}

void updateAudio() {

  startAudio();

  if(!playing) { return; }

  size_t amount = i2s.write(ptr, sizeof(ptr));
  ptr += amount;
  toPlay -= amount;

  if(toPlay == 0) {
    i2s.end();
    playing = false;
  }

}

void startAudio() {

  if(playing) { return; }

  ptr = StarWars30_raw;
  toPlay = sizeof(StarWars30_raw);
  pos = 0;
  i2s.begin(sampleRate);
  playing = true;

}
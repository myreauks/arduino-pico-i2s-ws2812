#define VAL0          0  /* 0 Bit: 0.396 us (need: 0.4 us low) */
#define VAL1          1  /* 1 Bit: 0.792 us (need: 0.8 us high */

float fract(float x) { return x - int(x); }
float mix(float a, float b, float t) { return a + (b - a) * t; }
float step(float e, float x) { return x < e ? 0.0 : 1.0; }

void clearStrip() {
  for (int row = 0; row < NUM_STRIPS; row++) {
    for (int i = 0; i < NUM_LEDS_PER_STRIP; i++) {
      setPixelRGBW(row, i, 0,0,0,0);
    }
  }
} 

void fillHSV(float h, float s, float v) {
  int r = 255 * v * mix(1.0, constrain(abs(fract(h + 1.0) * 6.0 - 3.0) - 1.0, 0.0, 1.0), s);
  int g = 255 * v * mix(1.0, constrain(abs(fract(h + 0.6666666) * 6.0 - 3.0) - 1.0, 0.0, 1.0), s);
  int b = 255 * v * mix(1.0, constrain(abs(fract(h + 0.3333333) * 6.0 - 3.0) - 1.0, 0.0, 1.0), s);
  int w = 0;

  for (int row = 0; row < NUM_STRIPS; row++) {
    for (int i = 0; i < NUM_LEDS_PER_STRIP; i++) {
      setPixelRGBW(row, i, r,g,b,w);
    }
  }
}

// Function to start DMA transfers for all strips in parallel
void transferData() {
    if (dma_busy) return;
    dma_channel_set_read_addr(dma_channel, pixel_buffer, true);
}

// DMA completion interrupt handler for parallel rows
void dma_complete_handler() {
    // Check which DMA channel triggered the interrupt
    uint32_t interrupt_flags = dma_hw->ints0;  // Get the interrupt flags

    // If the interrupt was triggered by our channel, clear it and reset the status
    if (interrupt_flags & (1u << dma_channel)) {
        dma_hw->ints0 = 1u << dma_channel;  // Clear the interrupt flag for our channel

        // Mark the DMA as no longer busy, allowing the next transfer to start
        dma_busy = false;
    }
}

void ledSetup() {

  // Initialize the PIO program for WS2812 parallel output
  uint offset = pio_add_program(pio, &ws2812_parallel_program);
  ws2812_parallel_program_init(pio, sm, offset, PIO_PIN_BASE, NUM_STRIPS, LED_DATA_RATE);

  // Set up DMA for each strip
  dma_channel = dma_claim_unused_channel(true);
  dma_channel_config c = dma_channel_get_default_config(dma_channel);
  channel_config_set_transfer_data_size(&c, DMA_SIZE_32);  // 32-bit data size
  channel_config_set_read_increment(&c, true);             // Increment source address
  channel_config_set_write_increment(&c, false);           // Do not increment destination
  channel_config_set_dreq(&c, pio_get_dreq(pio, sm, true)); // Use the PIO's data request

  dma_channel_configure(
    dma_channel,
    &c,
    &pio->txf[sm],              // PIO TX FIFO as destination
    pixel_buffer,       // Source buffer for this strip
    NUM_LEDS_PER_STRIP * 8,         // Number of transfers
    false                       // Do not start yet
  );

  irq_add_shared_handler(DMA_IRQ_0, dma_complete_handler, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
  irq_set_enabled(DMA_IRQ_0, true);

}

void setPixelRGBW(int lane, int pos, uint8_t red, uint8_t green, uint8_t blue, uint8_t white) {

  int idx;
  uint8_t *p;

  idx = pos*4*2;
  p = (uint8_t*)&pixel_buffer[idx];
  /* green */
  for(int i=0;i<8;i++) {
    if (white&0x80) {
      *p |= (VAL1<<lane); /* set bit */
    } else {
      *p &= ~(VAL1<<lane); /* clear bit */
    }
    white <<= 1; /* next bit */
    p++;
  }
  /* red */
  for(int i=0;i<8;i++) {
    if (red&0x80) {
      *p |= (VAL1<<lane); /* set bit */
    } else {
      *p &= ~(VAL1<<lane); /* clear bit */
    }
    red <<= 1; /* next bit */
    p++;
  }
  /* blue */
  for(int i=0;i<8;i++) {
    if (green&0x80) {
      *p |= (VAL1<<lane); /* set bit */
    } else {
      *p &= ~(VAL1<<lane); /* clear bit */
    }
    green <<= 1; /* next bit */
    p++;
  }
  /* white */
  for(int i=0;i<8;i++) {
    if (blue&0x80) {
      *p |= (VAL1<<lane); /* set bit */
    } else {
      *p &= ~(VAL1<<lane); /* clear bit */
    }
    blue <<= 1; /* next bit */
    p++;
  }
}
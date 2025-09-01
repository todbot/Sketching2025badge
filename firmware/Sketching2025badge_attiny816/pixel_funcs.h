extern volatile uint8_t led_buf[];

// Set an LED at location n an specific RGB color
void pixel_set(uint8_t n, uint8_t r, uint8_t g, uint8_t b) {
  led_buf[n*3+0] = (uint16_t)(g * LED_BRIGHTNESS) / 256; // G
  led_buf[n*3+1] = (uint16_t)(r * LED_BRIGHTNESS) / 256; // R 
  led_buf[n*3+2] = (uint16_t)(b * LED_BRIGHTNESS) / 256; // B
}

// Fill all LEDs a specific RGB color
void pixel_fill(uint8_t r, uint8_t g, uint8_t b) { 
  for(int i=0; i<NUM_LEDS; i++) { 
    pixel_set(i, r,g,b);
  }
}

// Fade a single LED by an amount
void pixel_fade(uint8_t n, uint8_t amount) {
  led_buf[n*3+0] = max(led_buf[n*3+0] - amount, 0);
  led_buf[n*3+1] = max(led_buf[n*3+1] - amount, 0);
  led_buf[n*3+2] = max(led_buf[n*3+1] - amount, 0);
}

// Fade all LEDs down by some amount
void pixel_fade_all(uint8_t amount) { 
  for(int i=0; i<NUM_LEDS*3; i++) { 
    led_buf[i] = max(led_buf[i] - amount, 0);
  }
}

// Display LED data to real LEDs
void pixel_show() {
  noInterrupts();
  tinyNeoPixel_show(NEOPIXEL_PIN, NUM_LEDS*3, (uint8_t *)led_buf);
  interrupts();
}



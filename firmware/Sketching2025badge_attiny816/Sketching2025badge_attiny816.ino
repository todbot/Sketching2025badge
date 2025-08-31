/**
 * Sketching2025badge_attiny816.ino -- 
 * 25 Aug 2025 - @todbot / Tod Kurt
 * 
 * - Designed for ATtiny816
 * - Uses "megaTinyCore" Arduino core: https://github.com/SpenceKonde/megaTinyCore/
 * - Board Configuration in Tools:
 *   - Defaults
 *   - Clock: 10 MHz internal
 *   - printf: minimal
 *   - Programer: "SerialUPDI - SLOW 57600 baud"
 * - Program with "Upload with Programmer" with a USB-Serial dongle 
 #    as described here: 
 *    https://learn.adafruit.com/adafruit-attiny817-seesaw/advanced-reprogramming-with-updi
 * 
 **/

// touch state
// - touched
// - held
// LED state
// - pattern_playing
// - pattern_update()
// - pattern_start()


#include <Wire.h>
#include "TouchyTouch.h"
// #include <tinyNeoPixel_Static.h>  
#include "Adafruit_seesawPeripheral_tinyneopixel.h"
// using seesaw's neopixel driver instead of megatinycore's tinyneopixel 
//  because it saves 500 bytes of flash
// but seems to glitch out, so going back to tinyNeoPixel_Static for now
// update: ahh seems like tinyNeoPixel_Static calls `noInterrupts()` so we'll mirror that

#define LED_BRIGHTNESS  80     // range 0-255
#define LED_UPDATE_MILLIS (10)
#define TOUCH_THRESHOLD_ADJ (1.2)
#define FPOS_FILT (0.05)

// note these pin numbers are the megatinycore numbers: 
// https://github.com/SpenceKonde/megaTinyCore/blob/master/megaavr/variants/txy6/pins_arduino.h
#define LED_STATUS_PIN  5 // PB4
#define NEOPIXEL_PIN 12  // PC2
#define MySerial Serial
#define NUM_LEDS (9)

const int touch_pins[] = {0, 1, 2, };  // which pins are the touch pins
const int touch_count = sizeof(touch_pins) / sizeof(int);
TouchyTouch touches[touch_count];
volatile uint8_t led_buf[NUM_LEDS*3];  // RGB LEDs: 3 bytes per LED

bool do_startup_demo = true;
uint8_t ledr, ledg, ledb;  // FIXME
uint32_t last_debug_time;
uint32_t last_led_time;
uint8_t pos = 0;
uint8_t touched = 0;
bool held = false;
uint16_t touch_timer = 0;

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

// Fade all LEDs down by some amount
void pixel_fade(uint8_t amount) { 
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

void startup_demo() {
  // do a little fade up to indicate we're alive
  for(byte i=0; i<100; i++) { 
    ledr = i, ledg = i, ledb = i;
    pixel_fill(ledr, ledg, ledb);
    pixel_show();
    delay(5);
  }
  for(byte i=0; i<255; i++) { 
    for(byte n=0; n < NUM_LEDS; n++) { 
      uint32_t c = wheel(i + n*50);
      uint8_t r = (c>>16) & 0xff, g = (c>>8) & 0xff, b = c&0xff;
      //r /= 2; g /= 2; b /= 2;  // dim a bit
      pixel_set(n, r,g,b);
    }
    pixel_show();
    delay(5);
  }
}

void setup() {
  MySerial.begin(115200);
  MySerial.println("\r\nHello there!");
  
  pinMode(LED_STATUS_PIN, OUTPUT);
  pinMode(NEOPIXEL_PIN, OUTPUT);

  // Touch buttons
  for (int i = 0; i < touch_count; i++) {
    touches[i].begin( touch_pins[i] );
    touches[i].threshold = touches[i].raw_value * TOUCH_THRESHOLD_ADJ; // auto threshold doesn't work
  }
  //touch_recalibrate();
}

void touch_recalibrate() {
    for(int i=0; i< touch_count; i++) { 
      uint16_t v = touches[i].recalibrate();
      touches[i].threshold = v * TOUCH_THRESHOLD_ADJ; // auto threshold doesn't work
    }
}

// update touch input state
// must be called within a block that's timed (like if(now-update_millis))
void update_touch() { 
  touched = 0;
  for ( int i = 0; i < touch_count; i++) {
    touches[i].update();
    touched |= (touches[i].touched() << i);
  }
  if (touched) { 
    touch_timer = min(touch_timer+1, 60000);
    if( touch_timer > 100 ) {  // held for a while
      held = true;
    }
  }
  else { 
    //touch_timer = 0;
    held = false;
  }
}

// main loop
void loop() {

  if(do_startup_demo) { 
    startup_demo();
    // recalibrate in case pads were being touched on power up (or power not stable)
    touch_recalibrate();
    do_startup_demo = false;
    touch_timer = 1000;
  }
  
  // LED update
  uint32_t now = millis();
  if( now - last_led_time > LED_UPDATE_MILLIS ) {  // only update neopixels every N msec
    last_led_time = now;

    update_touch();  

    digitalWrite(LED_STATUS_PIN, held ? HIGH : LOW);  // simple held status indicator

    if      (touches[0].pressed()) { pos = 85*0; }
    else if (touches[1].pressed()) { pos = 85*1; }
    else if (touches[2].pressed()) { pos = 85*2; }

    if (held) { 
      pos += 1;
    }

    // if touched, light up
    // if(touched) {  // nope this sucks
    //   uint32_t c = wheel(pos*10000); // random());
    //   pixel_set((pos%NUM_LEDS), (c>>16) & 0xff, (c>>8) & 0xff, (c>>0) & 0xff);
    // }
    if( touched ) {
      for(int n=0; n < NUM_LEDS; n++) { 
        uint32_t c = wheel(pos + n*50);
        uint8_t r = (c>>16) & 0xff, g = (c>>8) & 0xff, b = c&0xff;
        pixel_set(n, r,g,b);
      }
    }
    // if( touched ) { 
    //   uint32_t c = wheel(pos);
    //   // sigh, unpack: fixme: change wheel to take r,g,b ptrs
    //   //uint8_t n = (pos/8) % NUM_LEDS;
    //   uint8_t n = random() % NUM_LEDS;
    //   pixel_set(n, (c>>16) & 0xff, (c>>8) & 0xff, (c>>0) & 0xff);
    // }

    // fade down LEDs on release
    if( !held ) { 
      if( touch_timer-- ) { 
        pixel_fade(1);
      }
    }
    
    pixel_show();
  }

  // debug
  // if( now - last_debug_time > 50 ) { 
  //   last_debug_time = now;
  //   MySerial.printf("pos: %d %d\r\n", pos, touch_timer);
  // }

} // loop()


// colorwheel
// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return pack_color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return pack_color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return pack_color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

static uint32_t pack_color(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint32_t)r << 16) | ((uint32_t)g <<  8) | b;
}

/**
 * Sketching2025badge_attiny816.ino -- 
 * 25 Aug 2025 - @todbot / Tod Kurt
 * 
 * - Designed for ATtiny816
 * - Uses "megaTinyCore" Arduino core: https://github.com/SpenceKonde/megaTinyCore/
 * - Board Configuration in Tools:
 *   - Defaults
 *   - Clock: 8 MHz internal
 *   - printf: minimal
 *   - Programer: "SerialUPDI - SLOW 57600 baud"
 * - Program with "Upload with Programmer" with a USB-Serial dongle 
 #    as described here: 
 *    https://learn.adafruit.com/adafruit-attiny817-seesaw/advanced-reprogramming-with-updi
 * 
 **/

#include <Wire.h>
#include "TouchyTouch.h"
#include "Adafruit_seesawPeripheral_tinyneopixel.h"

#define LED_BRIGHTNESS  80     // range 0-255
#define LED_UPDATE_MILLIS (15)
#define LED_IDLE_MILLIS (10*1000)   // 30 seconds
#define TOUCH_THRESHOLD_ADJ (1.2)
#define FPOS_FILT (0.05)

// note these pin numbers are the megatinycore numbers: 
// https://github.com/SpenceKonde/megaTinyCore/blob/master/megaavr/variants/txy6/pins_arduino.h
#define LED_STATUS_PIN  5 // PB4
#define NEOPIXEL_PIN 12  // PC2
#define MySerial Serial
#define NUM_LEDS (9)

#include "pixel_funcs.h"

const int touch_pins[] = {0, 1, 2, };  // which pins are the touch pins
const int touch_count = sizeof(touch_pins) / sizeof(int);
TouchyTouch touches[touch_count];
volatile uint8_t led_buf[NUM_LEDS*3];  // RGB LEDs: 3 bytes per LED

bool do_startup_demo = true;
uint8_t ledr, ledg, ledb;  // FIXME
uint8_t pos = 0;
uint8_t touched = 0;
bool held = false;
uint16_t touch_timer = 0;
uint32_t last_debug_time;
uint32_t last_led_time;
uint32_t last_touch_millis = 0;


void startup_demo(uint8_t delay_millis=15) {
  for(byte i=0; i<255; i++) { 
    for(byte n=0; n < NUM_LEDS; n++) { 
      uint32_t c = wheel(i + n*50);
      uint8_t r = (c>>16) & 0xff, g = (c>>8) & 0xff, b = c&0xff;
      pixel_set(n, r,g,b);
    }
    pixel_fade_all((255-i)/4);  // fade up
    pixel_show();
    delay(delay_millis);
  }
}

void setup() {
  MySerial.begin(115200);
  MySerial.println("\r\nWelcome to Sketching2025!");
  
  pinMode(LED_STATUS_PIN, OUTPUT);
  pinMode(NEOPIXEL_PIN, OUTPUT);

  // Touch buttons
  for (int i = 0; i < touch_count; i++) {
    touches[i].begin( touch_pins[i] );
    touches[i].threshold = touches[i].raw_value * TOUCH_THRESHOLD_ADJ; // auto threshold doesn't work
  }
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
  for (int i = 0; i < touch_count; i++) {
    touches[i].update();
    touched |= (touches[i].touched() << i);
  }
  if (touched) { 
    touch_timer = touch_timer < 60000 ? touch_timer+1 : touch_timer;
    //touch_timer = min(touch_timer+1, 60000);
    if (touch_timer > 100) {  // held for a while
      held = true;
    }
  }
  else { 
    held = false;
  }
}

// main loop
void loop() {

  if(do_startup_demo) { 
    startup_demo();
    touch_recalibrate();  // recalibrate in case pads touched on power up or unstable power
    do_startup_demo = false;
    touch_timer = 1000;
  }
  
  // LED update
  uint32_t now = millis();

  if( now - last_led_time > LED_UPDATE_MILLIS ) {  // only update neopixels every N msec
    last_led_time = now;

    update_touch();  

    digitalWrite(LED_STATUS_PIN, held ? HIGH : LOW);  // simple held status indicator

    if (touched) { 
      last_touch_millis = now;
    }

    if (now - last_touch_millis > LED_IDLE_MILLIS) {
      startup_demo(5);
      last_touch_millis = now;
    }
    
    // start at a different location on the colorwheel depending on button
    if      (touches[0].pressed()) { pos = 85*0; }
    else if (touches[1].pressed()) { pos = 85*1; }
    else if (touches[2].pressed()) { pos = 85*2; }

    if (touched) {
      if (held) { 
        pos += 1;  // rotate color wheel while held
      }
      for(int n=0; n < NUM_LEDS; n++) {   // rainbow across the LEDs
        uint32_t c = wheel(pos + n*10);
        uint8_t r = (c>>16) & 0xff, g = (c>>8) & 0xff, b = c&0xff;
        pixel_set(n, r,g,b);
      }
    }

    // fade down LEDs on release
    if( !held ) { 
      if( touch_timer-- ) { 
        pixel_fade_all(1);
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

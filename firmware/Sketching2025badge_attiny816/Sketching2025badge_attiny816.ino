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
 *   - Programer: "SerialUPDI - SLOW 57600 baud" or "SerialUPDI - 230400"
 * - Program with "Upload with Programmer" with a USB-Serial dongle 
 #    as described here: 
 *    https://learn.adafruit.com/adafruit-attiny817-seesaw/advanced-reprogramming-with-updi
 * 
 **/

#include <Wire.h>
#include "TouchyTouch.h"
#include "Adafruit_seesawPeripheral_tinyneopixel.h"

#define LED_BRIGHTNESS  80         // range 0-255, used by pixel_funcs.hs
#define LED_UPDATE_MILLIS (15)     // how often to update the LEDs, affects fade down time
#define LED_IDLE_MILLIS (30*1000)  // 30 seconds
#define TOUCH_THRESHOLD_ADJ (1.2)

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

uint32_t last_debug_time;
uint32_t last_led_time;
uint32_t last_touch_millis = 0;  // last time any touch was pressed
uint16_t fade_timer = 0;
uint8_t r,g,b;
uint8_t pos = 0;  // color wheel position
uint8_t touched = 0;  // bit-field of touches
bool held = false;   // is a touch held (any touch)
bool do_startup_demo = true;



// ---------------------------------------------------------------------------
// setup
void setup() {
  MySerial.begin(115200);
  MySerial.println("\r\nWelcome to Sketching2025!");
  
  pinMode(LED_STATUS_PIN, OUTPUT);
  pinMode(NEOPIXEL_PIN, OUTPUT);

  // touch buttons
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
void update_touch(uint32_t now) { 
  uint8_t touched_last = touched;
  touched = 0;
  for (int i = 0; i < touch_count; i++) {
    touches[i].update();
    touched |= (touches[i].touched() << i);
  }

  if (touched) {
    fade_timer = 0;
    if (touched != touched_last) { 
      last_touch_millis = now;  // record when touch changed
    }
    if (now - last_touch_millis > 500) { 
      held = true;  // note held if touched more than 0.5 secs
    }
  }
  else {  // on release
    held = false;
    fade_timer = 1000;
  }
}

void pattern_colorwheel(uint8_t pos, uint8_t wheel_step=50) {
  for(uint8_t n=0; n<NUM_LEDS; n++) { 
    colorwheel(pos + n*wheel_step, &r, &g, &b);
    pixel_set(n, r,g,b);
  }
}

void pattern_sparkles(uint8_t pos) {
  pixel_fade_all(3);
  //r = 225; g = 25; b = 160; // a warmer white, sorta, but fades bad
  r = g = b = 225;
  if (pos%16 == 0 ) {  // slow it down
    pixel_set(pos%NUM_LEDS, r,g,b);
  }
}

void pattern_whitespin(uint8_t pos) {
  r  = g = b = 200; 
  for(uint8_t n=0; n < 3; n++) { 
    pixel_set(n, r,g,b);
    if (pos%3 == n) { 
      pixel_set(n, 0,0,0); // turn one of them off to emulate spinning
    }
  }
}

void pattern_colorwheel_demo(uint16_t delay_ms=5) {
  for( int i=0; i<255; i++) {
    pattern_colorwheel(pos++);
    pixel_show();
    delay(delay_ms);
  }
}

// ---------------------------------------------------------------------------
// main loop
void loop() {

  if(do_startup_demo) { 
    pattern_colorwheel_demo();
    touch_recalibrate();  // recalibrate in case pads touched on power up or unstable power
    do_startup_demo = false;
    fade_timer = 1000;
  }
  
  // LED update
  uint32_t now = millis();

  if( now - last_led_time > LED_UPDATE_MILLIS ) {  // only update neopixels every N msec
    last_led_time = now;

    update_touch(now);   // updates toucheed and touches[] 

    digitalWrite(LED_STATUS_PIN, held ? HIGH : LOW);  // simple held status indicator

    if (now - last_touch_millis > LED_IDLE_MILLIS) { // idle timer expired, let's sparkle
      last_touch_millis = now;
      for( int i=0; i< 10; i++) { 
        pixel_fill(i*6, i*3, i*6);
        pixel_show();
        delay(100);
      }
      pattern_colorwheel_demo();
    }
    
    // show how to handle touch "pressed" events
    //   start at a different location on the colorwheel depending on button
    if (touches[0].pressed()) {       // button 0  (top right)
      pos = 0;
    }
    else if (touches[1].pressed()) {  // button 1  (bottom left)
      pixel_fill(0,0,0);
    }
    else if (touches[2].pressed()) {  // button 2 (top left)
    }

    // act on touch state
    if (touched) {
      if (held) { 
        pos++;  // while held, rotate "pos" value for color wheel or other uses
      }
      if( touched == 0b100 ) {         // button 2 pressed (top left)
        pos++;                         // gives the sense of it 'revving up'
        pattern_whitespin(pos/4);      // the /4 slows down the spinning;
      }
      else if( touched == 0b010 ) {    // button 1 pressed (bottom left)
        pattern_colorwheel(pos, 100);  // play pattern on touch
      }
      else if( touched == 0b001 ) {    // button 0 pressed (top right)
        pos++;  
        pattern_sparkles(pos);         // play pattern on touch
      }
    }
    else {  // if not touched
      // fade down LEDs on release
      if( fade_timer ) { 
        fade_timer--;
        pixel_fade_all(1);
      }
    }

    pixel_show();
  }

  //debug
  // if( now - last_debug_time > 100 ) { 
  //   last_debug_time = now;
  //   MySerial.printf("pos: %d %d\r\n", pos, touched);
  // }

} // loop()


// RGB colorwheel, just for Tom Igoe
// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
// pass in pointers to three bytes: r,g,b to set them
void colorwheel(byte wheelPos, uint8_t* pr, uint8_t* pg, uint8_t* pb ) {
  wheelPos = 255 - wheelPos;
  if (wheelPos < 85) {
    *pr = 255 - wheelPos * 3;  *pg = 0;  *pb = wheelPos * 3;
  }
  else if (wheelPos < 170) {
    wheelPos -= 85;
    *pr = 0; *pg = wheelPos * 3, *pb = 255 - wheelPos * 3;
  }
  else { 
    wheelPos -= 170;
    *pr = wheelPos * 3; *pg = 255 - wheelPos*3, *pb = 0;
  }
}

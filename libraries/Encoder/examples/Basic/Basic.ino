/* Encoder Library - Basic Example
 * https://github.com/hjd1964/Encoder
 * http://www.pjrc.com/teensy/td_libs_Encoder.html
 *
 * This example code is in the public domain.
 */

#include <Encoder.h>

// Avoid using pins with LEDs attached.
//   * Defaults to RISING edge except for AB_QUADRATURE (which always uses CHANGE)

// Change these two numbers to the pins connected to your encoder
#define ENC_PIN1 5  // A or CW or PULSE
#define ENC_PIN2 6  // B or CCW or DIR

// This is for a quadrature A/B encoder
//   * Best Performance: both pins have interrupt capability
//   * Good Performance: only the first pin has interrupt capability
//   * Low Performance:  neither pin has interrupt capability
Encoder myEnc(ENC_PIN1, ENC_PIN2);

// This is for a CW/CCW encoder (trigger on pin RISING)
//   * Best Performance: both pins have interrupt capability
//   * Low Performance:  neither pin has interrupt capability
//Encoder myEnc(ENC_PIN1, ENC_PIN2, CW_CCW);

// This is for a PULSE/DIR encoder (trigger on pin FALLING)
//   * Best Performance: PULSE pin has interrupt capability
//   * Low Performance:  neither pin has interrupt capability
//Encoder myEnc(ENC_PIN1, ENC_PIN2, PULSE_DIR, FALLING);

// This is for a PULSE only encoder where an external variable controls direction
// this example overrides default RISING edge for pin CHANGE and moves in reverse (due to -1)
//   * Best Performance: PULSE pin has interrupt capability
//volatile int8_t dir = -1;
//Encoder myEnc(ENC_PIN1, 0, PULSE_ONLY, CHANGE, &dir);

void setup() {
  Serial.begin(9600);
  Serial.println("Basic Encoder Test:");
}

long oldPosition  = -999;

void loop() {
  long newPosition = myEnc.read();
  if (newPosition != oldPosition) {
    oldPosition = newPosition;
    Serial.println(newPosition);
  }
}

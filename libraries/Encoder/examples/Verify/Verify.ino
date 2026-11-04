/* Encoder Library - Verify Example
 * https://github.com/hjd1964/Encoder
 *
 * This example code is in the public domain.
 */

// Change these two numbers to the pins reading the "encoder", avoid using pins with LEDs attached
#define ENC_PIN1 15  // A or CW or PULSE
#define ENC_PIN2 16  // B or CCW or DIR

// Change these two numbers to the pins simulating the "encoder", avoid using pins with LEDs attached
// Wire from OUT_PIN1 to ENC_PIN1 and OUT_PIN2 to ENC_PIN2
#define OUT_PIN1 34  // A or CW or PULSE
#define OUT_PIN2 35  // B or CCW or DIR

// Change encoder type being tested: AB_QUADRATURE, CW_CCW, PULSE_DIR, PULSE_ONLY
#define ENCODER_TYPE AB_QUADRATURE

// Change encoder trgger being tested: CHANGE, RISING, FALLING (doesn't apply to AB encoders)
#define ENCODER_TRIGGER CHANGE

// Optionally turn off interrupts
//#define ENCODER_DO_NOT_USE_INTERRUPTS

#include <Encoder.h>

volatile int8_t directionHint = 1;
Encoder myEnc(ENC_PIN1, ENC_PIN2, ENCODER_TYPE, ENCODER_TRIGGER, &directionHint);

int sequence = 0;
unsigned int phase = 0;
long position  = 0;
unsigned long outputPeriod = 100; // in ms
unsigned long outputNext = 0;

void setup() {
  Serial.begin(9600);
  delay(2000);
  Serial.println("Encoder Verification Test:");

  pinMode(OUT_PIN1, OUTPUT);
  pinMode(OUT_PIN2, OUTPUT);
  digitalWrite(OUT_PIN1, LOW);
  digitalWrite(OUT_PIN2, LOW);
  delay(1);
  myEnc.readAndReset();
}

void loop() {
  unsigned long now = millis();

  // show the motion
  long newPos = myEnc.read();
  if (newPos != position) {
    position = newPos;
    Serial.println(position);
  }

  // simulate the encoder
  if ((now - outputNext) >= 0) {
    outputNext = now + outputPeriod;
    if (sequence < 100) moveFwd();
    if (sequence >= 100 && sequence < 200) moveRev();
    delay(1);

    sequence++;
    if (sequence > 200) sequence = 200;
  }
}

#if ENCODER_TYPE == AB_QUADRATURE
  void moveFwd() {
    if (phase % 4 == 0) { digitalWrite(OUT_PIN1, 0); digitalWrite(OUT_PIN2, 1); }
    if (phase % 4 == 1) { digitalWrite(OUT_PIN1, 1); digitalWrite(OUT_PIN2, 1); }
    if (phase % 4 == 2) { digitalWrite(OUT_PIN1, 1); digitalWrite(OUT_PIN2, 0); }
    if (phase % 4 == 3) { digitalWrite(OUT_PIN1, 0); digitalWrite(OUT_PIN2, 0); }
    phase++;
  }
  void moveRev() {
    phase--;
    if (phase % 4 == 0) { digitalWrite(OUT_PIN1, 0); digitalWrite(OUT_PIN2, 1); }
    if (phase % 4 == 1) { digitalWrite(OUT_PIN1, 1); digitalWrite(OUT_PIN2, 1); }
    if (phase % 4 == 2) { digitalWrite(OUT_PIN1, 1); digitalWrite(OUT_PIN2, 0); }
    if (phase % 4 == 3) { digitalWrite(OUT_PIN1, 0); digitalWrite(OUT_PIN2, 0); }
  }
#endif

#if ENCODER_TYPE == CW_CCW
  void moveFwd() {
    if (phase % 2 == 0) digitalWrite(OUT_PIN1, 1); else digitalWrite(OUT_PIN1, 0);
    phase++;
  }
  void moveRev() {
    phase--;
    if (phase % 2 == 0) digitalWrite(OUT_PIN2, 1); else digitalWrite(OUT_PIN2, 0);
  }
#endif

#if ENCODER_TYPE == PULSE_DIR || ENCODER_TYPE == PULSE_ONLY
  void moveFwd() {
    #if ENCODER_TYPE == PULSE_DIR
      digitalWrite(OUT_PIN2, 0);
    #else
      directionHint = 1;
    #endif
    if (phase % 2 == 0) digitalWrite(OUT_PIN1, 1); else digitalWrite(OUT_PIN1, 0);
    phase++;
  }
  void moveRev() {
    phase--;
    #if ENCODER_TYPE == PULSE_DIR
      digitalWrite(OUT_PIN2, 1);
    #else
      directionHint = -1;
    #endif
    if (phase % 2 == 0) digitalWrite(OUT_PIN1, 1); else digitalWrite(OUT_PIN1, 0);
  }
#endif

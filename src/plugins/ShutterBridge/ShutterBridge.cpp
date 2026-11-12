// Local Feather shutter telemetry bridge for OCS4 / Teensy 4.1.
// The OCS4 host-board Serial3 connector is traced to Teensy pins 7/8, which
// are Serial2 RX/TX in Teensyduino. Serial_C must be disabled so OCS does not
// start a second reader on Serial2; this plugin owns that UART.

#include "ShutterBridge.h"

#include "../../Common.h"
#include "../../lib/tasks/OnTask.h"
#include "../../lib/sense/Sense.h"
#include "../../observatory/roof/Roof.h"

#ifdef SERIAL_C
  #error "Disable SERIAL_C: ShutterBridge owns Serial2 and OCS must not start its command reader."
#endif

#include <stdlib.h>
#include <string.h>

extern float remoteBatteryVoltage;

ShutterBridge shutterBridge;

namespace {
  constexpr uint32_t SHUTTER_UART_BAUD = 115200;
  constexpr uint32_t SHUTTER_STATUS_TIMEOUT_MS = 5000;

  void shutterBridgePoll() {
    shutterBridge.poll();
  }

  void setRoofLimitStates(bool openLimit, bool closedLimit) {
    // Remote status is authoritative for these two roof limits. Keep the
    // physical DIN pins as inputs; never drive the host-board protection path.
    sense.setVirtualState(ROOF_LIMIT_OPENED_SENSE, openLimit);
    sense.setVirtualState(ROOF_LIMIT_CLOSED_SENSE, closedLimit);
  }
}

namespace {
  uint32_t lastStatusMs = 0;
  bool statusFresh = false;
}

void ShutterBridge::init() {
#if !defined(ARDUINO_TEENSY41)
  #error "ShutterBridge uses Serial2 on the OCS Teensy 4.1 host."
#endif

  VLF("MSG: ShutterBridge, starting on Serial2 (RX2 pin 7) at 115200 baud");

  // Start with both remote-backed limits unknown/inactive. The sense layer
  // supplies these states in software and leaves the physical pins untouched.
  setRoofLimitStates(false, false);
  lastStatusMs = millis();
  statusFresh = false;

  Serial2.begin(SHUTTER_UART_BAUD);

  if (!tasks.add(10, 0, true, 5, shutterBridgePoll, "ShutSer")) {
    Serial.println("ERR: ShutterBridge, failed to start telemetry task");
  }
}

void ShutterBridge::poll() {
  while (Serial2.available() > 0) {
    char c = (char)Serial2.read();

    if (c == '\r') continue;

    if (c == '\n') {
      if (!discardingLine && lineLength > 0) {
        lineBuffer[lineLength] = '\0';
        processLine(lineBuffer);
      }
      lineLength = 0;
      discardingLine = false;
      continue;
    }

    if (discardingLine) continue;

    if (lineLength < sizeof(lineBuffer) - 1) {
      lineBuffer[lineLength++] = c;
    } else {
      // Ignore an overlong frame through its newline, then resume cleanly.
      lineLength = 0;
      discardingLine = true;
    }
  }

  if ((uint32_t)(millis() - lastStatusMs) > SHUTTER_STATUS_TIMEOUT_MS) {
    if (statusFresh) {
      Serial.println("ShutterBridge: telemetry stale; clearing virtual roof limits");
    }
    statusFresh = false;
    setRoofLimitStates(false, false);
    if (roof.isMoving()) roof.stop();
  }
}

bool ShutterBridge::batteryVoltageAvailable() const {
  return statusFresh &&
         (uint32_t)(millis() - lastStatusMs) <= SHUTTER_STATUS_TIMEOUT_MS &&
         !isnan(remoteBatteryVoltage);
}

float ShutterBridge::batteryVoltage() const {
  return remoteBatteryVoltage;
}

void ShutterBridge::processLine(char *line) {
  Serial.print("ShutterBridge RX: ");
  Serial.println(line);

  char *comma = strchr(line, ',');
  if (comma == nullptr) return;
  *comma = '\0';

  if (strcmp(line, "OPEN") == 0) {
    setRoofLimitStates(true, false);
  } else if (strcmp(line, "CLOSED") == 0) {
    setRoofLimitStates(false, true);
  } else if (strcmp(line, "MOVING") == 0 || strcmp(line, "UNKNOWN") == 0) {
    setRoofLimitStates(false, false);
  } else if (strcmp(line, "ERROR") == 0) {
    roof.stop();
    setRoofLimitStates(false, false);
  } else {
    // Do not alter roof state for unrecognized status text.
    return;
  }

  // Preserve the remote battery value for the web status tile and Alpaca Switch.
  char *voltageEnd = nullptr;
  float voltage = strtof(comma + 1, &voltageEnd);
  if (voltageEnd == comma + 1 || isnan(voltage) || voltage < 0.0F || voltage > 6.0F) {
    remoteBatteryVoltage = NAN;
  } else {
    remoteBatteryVoltage = voltage;
  }
  lastStatusMs = millis();
  statusFresh = true;

  Serial.printf("ShutterBridge: status=%s battery=%.2fV roofOpen=%d roofClosed=%d roofMoving=%d\n",
                line, remoteBatteryVoltage,
                roof.isOpen(), roof.isClosed(), roof.isMoving());

}

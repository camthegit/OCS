#pragma once

#include <Arduino.h>

class ShutterBridge {
public:
  void init();
  void poll();
  bool batteryVoltageAvailable() const;
  float batteryVoltage() const;

private:
  void processLine(char *line);

  char lineBuffer[48] = {};
  size_t lineLength = 0;
  bool discardingLine = false;
};

extern ShutterBridge shutterBridge;

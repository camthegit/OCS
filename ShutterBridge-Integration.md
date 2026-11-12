# OCS and Feather Shutter Bridge

This note records the working OCS4 / Teensy 4.1, Feather Base, and Feather Remote arrangement and the software path used to report shutter status to the OCS web panel and Alpaca clients such as NINA.

## Command and status flow

```text
OCS web panel / NINA (ASCOM Alpaca)
        │ open / close / stop
        ▼
OCS Roof controller ── RE1 / RE2 outputs ──► Feather Base GPIO1 / GPIO3
                                                   │ ESP-NOW command
                                                   ▼
                                             Feather Remote
                                                   │ ESP-NOW status + battery
                                                   ▼
Feather Base Serial0 TX ──UART──► OCS Serial2 RX (Teensy pin 7)
                                                   │
                                                   ▼
                                      ShutterBridge plugin
                                                   │ virtual roof limits
                                                   ▼
                                    OCS Roof / web / Alpaca state
```

OCS sends movement requests to the Base through its configured roof relays. The Base reads those requests on GPIO1 (open) and GPIO3 (close), then sends `OPEN`, `CLOSE`, or `STOP` commands to the Remote over ESP-NOW. The Remote returns status and battery telemetry by ESP-NOW. The Base forwards each report as a text line such as `MOVING,4.23` or `CLOSED,4.23` on `Serial0`.

The bridge parses that line, updates OCS's virtual roof-limit states, and updates the remote battery value used by the OCS status display. OCS's normal roof monitor then observes the virtual endpoint and finishes the movement. The Alpaca dome shutter status reads the same OCS roof state, so the web panel and NINA see a consistent result.

## Alpaca Switch battery sensor

`ALPACA_REMOTE_BATTERY_SWITCH` enables a read-only variable sensor named **Remote Feather Battery Voltage**. It is appended after the configured relay and lighting switches, preserving their existing IDs. In the current configuration it follows two power switches and two room-light switches, so it is switch ID 4 (IDs start at 0); clients should query `GetSwitchName` rather than assume that ID if the configuration changes.

- `CanWrite` is false. Attempts to set the battery sensor return an invalid-operation error.
- `GetSwitchValue` returns the latest remote voltage as a numeric value in volts, with a declared range of 0–5 V and 0.01 V step.
- `GetSwitch` returns false at 0 V and true above 0 V, as required for the boolean companion method on a variable sensor.
- If no valid voltage has arrived, or telemetry has been stale for more than 5 seconds, value reads return an invalid-operation error instead of presenting an old reading as current.

This uses ASCOM Switch's read-only sensor model: `CanWrite=false` identifies a sensor, and `GetSwitchValue` supplies its variable numeric reading. [ASCOM Switch interface guidance](https://ascom-standards.org/newdocs/switch-faq.html)

## Serial and board connections

- The OCS4 host-board connector is labelled **Serial3**, but the user's board trace found it wired to Teensy 4.1 pins **7 and 8**. Those Teensy pins are **Serial2 RX and TX**, respectively; the plugin therefore owns `Serial2` at **115200 baud**.
- Connect Feather Base **Serial0 TX** to OCS **Serial2 RX, Teensy pin 7**, and connect the boards' grounds. The ShutterBridge currently receives telemetry only; OCS Serial2 TX, Teensy pin 8, is not used by this plugin.
- OCS `ROOF_MOTOR_OPEN_RELAY` is configured as relay 1 and `ROOF_MOTOR_CLOSE_RELAY` as relay 2. The OCS4 pin map assigns those relay outputs to Teensy pins 24 and 25. The Feather Base firmware reads its open and close requests on **GPIO1** and **GPIO3**, configured with pull-downs; a HIGH requests the corresponding direction.
- The command wiring is assumed to provide a Feather-safe logic level at GPIO1/GPIO3. Do not connect a higher-voltage relay output directly to an ESP32-S3 GPIO; use the intended isolated or level-limited interface.
- The Base's ESP-NOW peer address is configured in `Feather_Base.ino`. It must match the Feather Remote's active MAC address.
- The Base uses `Serial0` for telemetry to OCS and USB `Serial` for its debug console. The OCS bridge does not use this UART to send movement commands.

## Virtual limit behavior

`Config.h` assigns roof-open to sense handle 1 and roof-closed to handle 2. Both configured sense polarities are active LOW. The ShutterBridge no longer switches the physical DIN1/DIN2 pins to outputs or writes to them. Instead, `SenseInput` supports a logical virtual-state override, and the plugin updates the two roof-limit handles from remote telemetry:

| Remote report | OCS open limit | OCS closed limit | Effect |
|---|---:|---:|---|
| `OPEN` | Active | Inactive | Roof monitor completes opening and reports open |
| `CLOSED` | Inactive | Active | Roof monitor completes closing and reports closed |
| `MOVING` | Inactive | Inactive | Existing OCS opening/closing operation remains in motion |
| `UNKNOWN` | Inactive | Inactive | Does not cancel a command; while idle, no confirmed endpoint means Alpaca may report Error |
| `ERROR` | Inactive | Inactive | OCS stops the roof operation; Alpaca reports Error |
| No recognized status for over 5 seconds | Inactive | Inactive | Bridge clears endpoints and stops an active OCS movement |

The important `UNKNOWN` behavior is intentional: the Remote can report UNKNOWN during startup or before its first confirmed limit. Treating that report as a stop used to cancel each OCS command before it could run. At the bridge level, only an explicit `ERROR` or stale telemetry stops movement; OCS's normal roof safety and movement-timeout logic still applies.

The bridge's virtual override applies to the configured roof-sense handles; other OCS sense inputs retain their normal behavior. A virtual endpoint remains in effect until another recognized status changes it or telemetry becomes stale.

## Test mode and observed result

The Remote firmware is in test mode with virtual limits enabled and motors disconnected. In that mode its outputs indicate simulated motion and endpoint states; it should not be used as evidence of connected motor operation.

The user confirmed that open/close commands and shutter status now work from both the OCS web page and NINA through ASCOM Alpaca.

## Electrical note for the next host board

Earlier measurements reported about 4.14 V at Teensy pins 2 and 3 and 3.29 V at pins 20 and 21. The software no longer drives pins 2 and 3, but OCS still initializes the configured physical sense pins as inputs. The virtual-state change does not electrically isolate those pins or resolve the measured voltage. Verify the protection network and voltage at the MCU side before finalizing or powering a replacement host board. The in-circuit 1.65 kΩ readings do not establish the individual resistor values.

The OCS4 pin map lists SENSE5/SENSE6 as pins 20/21 and notes that they share their pins with analog inputs. They were discussed as possible future inputs, but this working software bridge does not require remapping the roof limits to them.

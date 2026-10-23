# Serial API

`modules/serial_api.yaml` exposes a device's entities on a UART as a line protocol. Anything that
can open a serial port can read every sensor, drive every actuator and receive every state change
in real time — the same entities, commands and values the device offers over MQTT, with no broker,
network or credentials in the path.

This page is the protocol contract. The `HELLO` banner reports the version it describes:
**protocol version 1**.

## Enabling it

```yaml
substitutions:
  serial_api_tx_pin: "1"   # required, no default
  serial_api_rx_pin: "3"   # required, no default

packages:
  criotive:
    url: https://github.com/criotive/esphome-sdk
    ref: vX.Y.Z
    files:
      # … the room's other modules …
      - path: modules/serial_api.yaml
        vars: {sdk_ref: vX.Y.Z}
```

Nothing else is configured. Every entity the device declares is on the wire on the next build,
including entities a room adds later.

## Wiring

The line is **115200 baud, 8N1, no flow control**. Two pin choices exist, and each costs something:

| | Pins | Cable | Gives up |
|---|---|---|---|
| **The board's USB port** (UART0) | `"1"` / `"3"` | a USB cable | SW1 on `hds_v1_1` (set `hds_v1_1_sw1_enabled: "false"`); CAN on `hds_v1_1`, whose CAN RX is GPIO3; the logger's serial console; `improv_serial.yaml`, so Wi-Fi comes from a station slot, `wifi_ap.yaml` or `improv_ble.yaml` |
| **A slot pair** | any free pair, e.g. `"${slot_1_audio_tx_dat}"` / `"${slot_1_audio_rx}"` | a 3.3 V USB-serial adapter: **TX, RX and GND only** | nothing — CAN, SW1 and the console keep their pins |

`esphome config` rejects a configuration in which the serial API's pins collide with SW1, with a
`canbus:` block's configured pins, or — on UART0 — with the logger's console or `improv_serial`.
The check runs in the component itself, so it applies to every room that imports the module.

**On a slot pair, never connect DTR or RTS.** They drive EN and GPIO0 on a USB-serial adapter
wired for flashing, so a client that toggles them resets the board or holds it in reset.

**On the USB port, DTR and RTS are already wired** to EN and GPIO0 by the board's auto-reset
circuit: the board resets while RTS is asserted and DTR is not, and enters download mode in the
opposite case. Asserting both, or neither, is harmless. Two client habits trip it:

- **Closing the port.** Linux drops DTR and RTS on close (the `hupcl` terminal flag), and on a
  NodeMCU-32 bench board that reset the device or held it in reset until the port was closed again,
  in five of six cycles. Turn the flag off once after the adapter is plugged in, before any client
  opens the port, and the board survives any number of reconnects (twenty of twenty measured):

  ```sh
  stty -F /dev/ttyUSB0 -hupcl
  ```

  On other systems, keep the port open for as long as the integration runs.
- **Setting DTR and RTS one at a time.** pyserial's `dtr = False` followed by `rts = False` passes
  through the reset state. Leave both at their defaults; the operating system asserts both together
  when the port opens.

**Every reset prints boot text first.** The ROM and the bootloader write about 35 lines on GPIO1 at
115200 before the firmware runs. A client must ignore any line that does not begin with one of the
device keywords below; this is part of the protocol, not a courtesy. After `HELLO` the line carries
protocol lines only.

## Framing

- UTF-8, one record per line. A line ends with `\n`; a `\r` immediately before it is ignored, so
  CRLF clients work unchanged.
- Fields are separated by exactly one space. A leading space on a request is ignored; trailing
  spaces are significant, because a raw value may end in one.
- A request is at most **256 bytes**, terminator included. A longer line answers
  `ERR parse line-too-long`, and input is discarded up to the next `\n`.
- **Quoted fields** — an `ENT` name and each entry of a list domain — are wrapped in `"`; inside
  them `\` escapes `\` and `"`, and no other escape exists.
- **Raw values** (text, select and light values in `STATE`, `EVT` and `SET`) start after the single
  space that follows the address and run verbatim to the end of the line. `SET text/label ab  `
  writes `ab  `.
- **The empty value** is spelled `SET <address>` or `SET <address> ` (one trailing space). Nothing
  else means empty.
- A `SET` value containing a byte below 0x20, or that is not valid UTF-8, answers `ERR bad-value`.
  A value therefore never contains a newline.

## Requests

| Request | Reply |
|---|---|
| `HELLO` | the banner |
| `PING` | `PONG` |
| `LIST` | one `ENT` line per exposed entity, then `END` |
| `GET <address>` | `STATE <address> <value>`, or `ERR` |
| `GET *` | one `STATE` line per entity that has a state, then `END` |
| `SET <address> <value>` | `OK`, or `ERR` |
| `UNSUB` | `OK`; state changes are no longer announced |
| `SUB` | `OK`; state changes are announced again |

Verbs are case-sensitive. A request with an argument it does not take (`PING x`) answers
`ERR parse`.

## Device lines

```text
HELLO serial_api <protocol-version> <device-name> <firmware-version>
ENT <type> <object_id> <rw|ro> "<name>"[ <domain>]
STATE <address> <value>
EVT <address> <value>
OK
END
PONG
ERR <code>[ <detail>]
```

### Events

**Every state change is announced as an `EVT` line, from boot, without a request.** A client that
only listens learns of a button pressed, a relay switched or a sensor reading the moment the device
publishes it — the same moment it reaches MQTT. Polling with `GET` works at any time as well.
`UNSUB` silences the stream and `SUB` resumes it; the setting lasts until the next reboot.

- An `EVT` can arrive between a request and its reply, but never inside a multi-line `LIST` or
  `GET *` reply. Route lines by their first word, not by position.
- A `SET` that changes a state produces that state's `EVT` as well as the `OK`, usually first.
- The device prints `HELLO` when the firmware starts. States published before that are not
  announced, so a client that needs the full picture sends `GET *` after `HELLO`.
- Buttons have no state, so pressing one produces no `EVT`.

## Addresses

An address is `<type>/<object_id>`. `<type>` is the ESPHome entity type (`switch`, `sensor`, …) and
`<object_id>` is the entity's object id, byte for byte the segment its MQTT topics use: the switch
named `Puzzle Teclado` is `switch/puzzle_teclado` on serial and `…/switch/puzzle_teclado/…` on
MQTT. MQTT files text sensors under `sensor`; serial keeps the entity type, `text_sensor`.

One address is not an entity: **`mqtt/status`** reports the broker connection, `online` or
`offline` — the payloads of the device's MQTT status topic. It is listed, readable and announced
like any read-only entity, and is present whenever the device runs MQTT.

## Values

| Type | `STATE` / `EVT` value | `SET` accepts | `ENT` domain |
|---|---|---|---|
| `switch` | `ON` / `OFF` | `ON`, `OFF`, `TOGGLE` | — |
| `binary_sensor` | `ON` / `OFF` | read-only | — |
| `sensor` | decimal with the entity's `accuracy_decimals` (the MQTT payload), or `NaN` before the first reading | read-only | — |
| `number` | shortest decimal that round-trips, or `NaN` | a decimal within `[min, max]` | `min max step` |
| `text` | raw string | a raw string within the length bounds, measured in bytes | `min_length max_length` |
| `text_sensor` | raw string | read-only | — |
| `select` | the current option, raw | one of the options, byte for byte | the options, each quoted |
| `button` | — (`GET` answers `ERR no-state`) | `PRESS` | — |
| `light` | the JSON document of the MQTT state topic, compact, on one line | `ON`, `OFF`, `TOGGLE`, or a JSON object in the MQTT light command schema (`state`, `brightness`, `color`, `color_temp`, `white`, `effect`, `transition`, `flash`) | the effect names, each quoted, `"None"` last |
| `mqtt/status` | `online` / `offline` | read-only | — |

Any other ESPHome entity type is listed `ro` with no domain and answers `ERR unsupported-type` to
`GET` and `SET`, so a client sees that it exists.

A `SET` is applied exactly as the matching MQTT command is: through the entity's own call, so the
new state is published on MQTT and announced on serial. Values are validated first; a value the
entity would decline answers `ERR bad-value` and nothing is written.

## What is exposed

The serial API offers what MQTT offers:

| Entity | On serial |
|---|---|
| any entity with `internal: true` | hidden — not listed, `GET`/`SET` answer `ERR denied` |
| `secret_ota` | hidden — it is a credential |
| an object id starting with `ota_` | listed `ro`, readable and announced; `SET` answers `ERR denied` |
| everything else | listed, readable, announced and — for the writable types — writable |

"Everything else" includes the system controls (`button/system_restart`, `button/system_shutdown`,
`button/system_factory_reset`), `button/location_request` and `switch/reset_room`. The `ota_` prefix
is **reserved**: an entity named `ota_*` can never be written over serial, so a serial client cannot
start a firmware update.

Two MQTT features have no serial counterpart: the `ota` command on the system command topic, and the
Wi-Fi scan that `location_request` publishes on the telemetry topic. Pressing `location_request`
over serial starts the scan; its result still goes to MQTT only.

There is no authentication. Physical access to the port is the trust boundary: whoever can reach
the connector can reach the relays it drives.

## Errors

`ERR <code>` or `ERR <code> <detail>`. The detail is for a person reading a terminal; a client
branches on the code alone. The set is closed:

| Code | Meaning |
|---|---|
| `parse` | the line is not a valid request: unknown verb, missing or extra field, malformed address, line too long |
| `unknown-entity` | no entity has that address |
| `unsupported-type` | the entity's type has no value encoding in this protocol version |
| `read-only` | the entity's type cannot be written |
| `bad-value` | the value is not valid for that entity; nothing was written |
| `denied` | the exposure rule keeps the entity off the wire, or makes it read-only |
| `no-state` | the entity has no state to read (a button) |

## Compatibility

The verbs, the device lines, the value encodings and the error codes are a public contract.
**No CI gate covers them**: `check-contract-diff.sh` reads only the substitutions in `modules/` and
`hardware/`, so a change to the wire format is caught by human review alone. An incompatible change
raises the protocol version the banner reports.

## Example

Recorded on a bench device:

```text
→ HELLO
← HELLO serial_api 1 bench-serial-d68abc 1.0.0-serial
→ LIST
← ENT binary_sensor boot_button ro "Boot Button"
← ENT light strip rw "Strip" "Pulse" "Rainbow" "None"
← ENT sensor temperature ro "Temperature"
← ENT switch relay rw "Relay"
← ENT button system_restart rw "System Restart"
← ENT text_sensor ota_status ro "OTA status"
← ENT number volume rw "Volume" 0 100 5
← ENT text label rw "Label" 0 16
← ENT select mode rw "Mode" "Easy" "Hard Mode"
← ENT lock door_lock ro "Door Lock"
← ENT mqtt status ro "MQTT status"
← END
← EVT sensor/temperature 20.95
→ SET switch/relay ON
← EVT switch/relay ON
← OK
← EVT binary_sensor/relay_feedback ON
→ SET number/volume 101
← ERR bad-value number/volume
→ SET light/strip {"state":"ON","brightness":128,"color":{"r":255,"g":0,"b":0}}
← EVT light/strip {"effect":"None","effect_index":0,"effect_count":2,"color_mode":"rgb","state":"ON","brightness":128,"color":{"r":255,"g":0,"b":0}}
← OK
→ SET text/ota_https_prod https://example.com/fw.bin
← ERR denied ota_https_prod
→ GET lock/door_lock
← ERR unsupported-type lock/door_lock
```

A client in Python:

```python
import serial

# On the board's USB port, run `stty -F /dev/ttyUSB0 -hupcl` once first, and leave DTR/RTS alone.
port = serial.Serial("/dev/ttyUSB0", 115200, timeout=1)
port.write(b"SET switch/relay ON\n")
while True:
    line = port.readline().decode("utf-8", "replace").rstrip("\r\n")
    word = line.split(" ", 1)[0]
    if word == "EVT":
        print("event:", line[4:])
    elif word in ("OK", "ERR"):
        print("reply:", line)
```

# Serial API

The serial API lets any program that can open a serial port talk to a Criotive device: list its
entities, read every sensor, drive every actuator and receive every state change the moment it
happens. It is the same set of entities, commands and values the device offers over MQTT, with no
network, broker or credentials in the path — a cable and a line of text.

This page is the protocol reference for anyone writing a client: a tablet app, a PC program, a
PLC gateway. It describes **protocol version 1**, the version the device reports in its `HELLO`
banner. How to enable the API in a device's firmware is a separate page:
[`serial-api-firmware.md`](serial-api-firmware.md).

## At a glance

- **115200 baud, 8N1, no flow control.** Text lines in UTF-8, each ending in `\n`.
- **Requests and replies.** The client sends one request per line. The device answers each request
  with one reply line, or with a block of lines that ends in `END`. Replies come in the order the
  requests were sent.
- **Events.** Whenever an entity's state changes, the device sends an `EVT` line on its own, without
  being asked, from the moment it says `HELLO`.
- **Addresses.** Every entity has an address `<type>/<object_id>`, such as `switch/door_lock` or
  `binary_sensor/fuse_inserted`.

A short session (`→` is the client, `←` the device):

```text
→ HELLO
← HELLO serial_api 1 cofre-c1-d68abc 1.2.0
→ GET switch/door_lock
← STATE switch/door_lock ON
→ SET switch/door_lock OFF
← EVT switch/door_lock OFF
← OK
← EVT binary_sensor/fuse_inserted ON
```

The last line was not requested: a player put a fuse in place, and the device reported it.

## Connecting

**Wiring.** The device's serial pins use 3.3 V logic. Connect three wires to a USB-serial adapter
(CP2102, CH340, FTDI or similar): the device's **TX to the adapter's RX**, the device's **RX to the
adapter's TX**, and **GND to GND**. Do not connect anything else.

**Never connect DTR or RTS.** On adapters wired for flashing ESP32 boards these lines drive the
chip's reset and boot-mode pins, so a client that toggles them resets the device or holds it in
reset.

**Line settings.** 115200 baud, 8 data bits, no parity, 1 stop bit, no hardware or software flow
control.

**Ignore lines you do not recognise.** A line that does not start with one of the device keywords
(`HELLO`, `ENT`, `STATE`, `EVT`, `OK`, `END`, `PONG`, `ERR`) is not part of the protocol: electrical
noise when the cable is plugged in, or boot text (below). A client must skip such lines. This is
part of the protocol, not a courtesy.

### When the device is connected through its own USB port

Some devices expose the serial API on the board's USB port instead of a dedicated pair of pins. The
firmware author tells you which. On the USB port two extra rules apply:

- **Every reset prints about 35 lines of boot text** before the firmware starts and sends `HELLO`.
  Skip them like any other unrecognised line.
- **DTR and RTS are already wired to reset.** The board resets while RTS is asserted and DTR is
  not, and enters download mode in the opposite case. Asserting both, or neither, is harmless. Two
  client habits trip it:
  - **Closing the port.** Linux drops DTR and RTS on close (the `hupcl` terminal flag), which can
    reset the board or hold it in reset until the port is opened again. Turn the flag off once
    after the adapter is plugged in (`stty -F /dev/ttyUSB0 -hupcl`), or keep the port open for as
    long as the client runs.
  - **Setting DTR and RTS one at a time.** pyserial's `dtr = False` followed by `rts = False` passes
    through the reset state. Leave both at their defaults: the operating system asserts both
    together when the port opens.

## A session, step by step

This is the flow a robust client follows.

1. **Open the port** and start reading lines. Route every line by its first word (see
   [Ordering](#ordering)); skip lines that are not protocol lines. Send a bare `\n` first: if
   noise from plugging in the cable is waiting in the device's input, it ends there, as a harmless
   `ERR parse`, instead of spoiling your first request.
2. **Send `HELLO`.** The reply is the banner `HELLO serial_api 1 <device-name> <firmware-version>`.
   Check that the protocol version is `1`. Do not match the device name exactly: it ends with a
   suffix taken from the board's hardware address (`cofre-c1-d68abc`), so two boards running the
   same firmware report different names.
3. **Discover the entities** with `LIST`, if the client does not already know them.
4. **Read the current state** with `GET *`, and keep it in a table.
5. **Keep the table current from `EVT` lines.** Each one is the new state of one entity.
6. **Send commands** with `SET`. `OK` means the device accepted the command. When the command
   changes the state, an `EVT` with the new state comes with it; when it does not — `ON` to a
   switch that is already on — there is no `EVT`. Never block waiting for one: if you need to
   confirm the result, `GET` the entity.
7. **Watch for a `HELLO` you did not ask for.** The device sends one every time its firmware
   starts, so an unrequested `HELLO` means it restarted: switches and other outputs are back in
   their power-on state, and changes during the restart were not announced. Send `GET *` again and
   re-send every output your application expects.
8. **Check the link** with `PING` when the line has been quiet. No `PONG` within a couple of
   seconds means the cable, the adapter or the device is gone; reopen the port and start again from
   step 2.

**Do not wait for a change you may have missed.** An `EVT` comes when the device publishes a state,
normally because it changed. If your application needs a condition — "is the fuse in place?" —
read it from your state table (or with `GET`) when it starts caring, instead of waiting for the
next `EVT`: if the condition was already true, no new `EVT` may come.

## Message format

- UTF-8, one message per line. A line ends with `\n`; a `\r` immediately before it is ignored, so
  CRLF clients work unchanged.
- Fields are separated by exactly one space. A leading space on a request is ignored; trailing
  spaces are significant, because a raw value may end in one.
- A request is at most **256 bytes**, terminator included. A longer line is answered with
  `ERR parse line-too-long`, and the input is discarded up to the next `\n`.
- **Quoted fields** — an entity's name in `ENT`, and each option in a list — are wrapped in `"`.
  Inside them, `\` escapes `\` and `"`; no other escape exists.
- **Raw values** — the values of text, text sensor, select and light entities in `STATE`, `EVT` and
  `SET` — start after the single space that follows the address and run to the end of the line.
  `SET text/label ab  ` writes `ab  `, trailing spaces included.
- **The empty value** is written `SET <address>` or `SET <address> ` (one trailing space). Nothing
  else means empty.
- A `SET` value containing a byte below 0x20, or that is not valid UTF-8, is answered with
  `ERR bad-value`, before the address is even looked up.
- **Every line the device sends is one line of valid UTF-8.** A state that holds a control
  character or an invalid byte — a text entity written over MQTT, a Wi-Fi network name — is sent
  with each such byte replaced by U+FFFD (`�`), so a value can never break a line or look like a
  reply. The replacement is one-way: a select option holding such a byte cannot be chosen over
  serial, and a text read and written back stores the `�`.
- Verbs are case-sensitive: `PING` works, `ping` is `ERR parse unknown-verb`. An empty line is
  `ERR parse unknown-verb` too.

### Trying it by hand

Any serial terminal works for exploring a device, as long as it ends each line with `\n` (or
`\r\n`). Several terminals send only `\r` when Enter is pressed — `screen`, `minicom` and PuTTY in
their default settings — and then the device never sees the end of a line, until 256 bytes later
it answers `ERR parse line-too-long`. `picocom --omap crlf` or the serial monitor of the Arduino
IDE (set to "Newline") work as they are.

## Requests

| Request | Reply |
|---|---|
| `HELLO` | the banner: `HELLO serial_api <protocol-version> <device-name> <firmware-version>` |
| `PING` | `PONG` |
| `LIST` | one `ENT` line per exposed entity, then `END` |
| `GET <address>` | `STATE <address> <value>`, or `ERR` |
| `GET *` | one `STATE` line per readable entity, then `END` |
| `SET <address> <value>` | `OK`, or `ERR` |
| `UNSUB` | `OK`; state changes are no longer announced |
| `SUB` | `OK`; state changes are announced again |

A request with an argument it does not take (`PING x`) is answered with `ERR parse`.

Requests may be **pipelined**: a client can send several lines without waiting, and the replies
come back in the same order. Keep no more than about **1 KB of requests unanswered**. The device
holds 1 KB of input while it is busy writing a long reply; beyond that, input is lost, and two
half-requests can merge into one wrong request. A client that waits for each reply before sending
the next never comes near this limit. The device answers requests in short bursts, so that its
own work — Wi-Fi, MQTT, the entities — keeps running between requests while a client is sending.

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

### `ENT` — the entity list

```text
ENT switch door_lock rw "Door Lock"
ENT number volume rw "Volume" 0 100 5
ENT select mode rw "Mode" "Easy" "Hard Mode"
ENT binary_sensor fuse_inserted ro "Fuse Inserted"
```

- `<type>` and `<object_id>` form the address: `switch/door_lock`.
- `rw` means the client may `SET` the entity; `ro` means it may only read it.
- `"<name>"` is the human-readable name, quoted.
- `<domain>` describes the accepted values (see [Values](#values)). It is present whenever the
  entity has one, also on an entity listed `ro`. A light without effects and a select without
  options have none.

### Events

**Every state the device publishes for an entity of a [supported type](#values) is announced as an
`EVT` line, without a request, from the moment the device says `HELLO`.** A client that only listens learns of a fuse inserted, a relay
switched or a sensor reading the moment the device publishes it — the same moment it reaches MQTT.
Polling with `GET` works at any time as well.

- **An `EVT` is the current value, not necessarily a change.** Most `EVT` lines follow a change,
  but some entities publish the same value again — a sensor read on a timer, for example — and
  each publish is announced. Treat every `EVT` as "this is the value now".
- **Nothing is sent before `HELLO`.** The device sends `HELLO` when its firmware starts, and only
  then starts announcing. States published while it was starting are not announced, so a client
  that needs the full picture sends `GET *` after `HELLO`.
- `UNSUB` silences the stream and `SUB` resumes it. Changes made while unsubscribed are not
  announced later, so send `GET *` after `SUB`. The setting lasts until the device restarts, which
  turns events back on.
- Buttons have no state: pressing one produces no `EVT`.

### Ordering

- **Replies come in request order**, one per request (a block counts as one).
- **An `EVT` can arrive between a request and its reply**, but never inside a `LIST` or `GET *`
  block. Route lines by their first word, never by their position.
- A `SET` that changes a state produces that state's `EVT` as well as the `OK`, usually first.

## Addresses

An address is `<type>/<object_id>`. `<type>` is the ESPHome entity type (`switch`, `sensor`, …)
and `<object_id>` is the entity's object id — the same segment its MQTT topics use. An entity
named `Puzzle Teclado` is `switch/puzzle_teclado`.

One address is not an entity: **`mqtt/status`** reports the device's connection to its MQTT broker,
`online` or `offline`. It is listed, readable and announced like any read-only entity, and is
present whenever the device runs MQTT.

## Values

| Type | `STATE` / `EVT` value | `SET` accepts | `ENT` domain |
|---|---|---|---|
| `switch` | `ON` / `OFF` | `ON`, `OFF`, `TOGGLE` | — |
| `binary_sensor` | `ON` / `OFF` | read-only | — |
| `sensor` | a decimal with the entity's configured precision, or `NaN` when there is no reading (before the first one, or after a failed one) | read-only | — |
| `number` | the shortest decimal that reads back as the same value, or `NaN` before the first value | a decimal within `[min, max]` | `min max step` |
| `text` | the raw string | a raw string whose length in **bytes** is within the bounds | `min_length max_length` |
| `text_sensor` | the raw string | read-only | — |
| `select` | the current option, raw | one of the options, byte for byte | the options, each quoted |
| `button` | — (`GET` answers `ERR no-state`) | `PRESS` | — |
| `light` | the light's state as compact JSON on one line | `ON`, `OFF`, `TOGGLE`, or a JSON object (below) | the effect names, each quoted, `"None"` last (none for a light without effects) |
| `mqtt/status` | `online` / `offline` | read-only | — |

- **An entity that has not published since the device started** reads `OFF` (`switch`,
  `binary_sensor`), the empty string (`text`, `text_sensor`, `select`) or `NaN` (`sensor`, `number`),
  and keeps reading so until it publishes. Most entities publish while the device starts, before
  `HELLO`, so `GET *` after `HELLO` returns real values; one that publishes later announces itself
  with an `EVT`.
- **Numbers** are written in plain decimal for magnitudes from 0.0001 up to 10^16, and in exponent
  form outside that range (`1e-05`, `1e+16`); `inf` and `-inf` can appear. A number's `step` is not enforced:
  any value within `[min, max]` is accepted.
- **The longest text a `SET` can carry** is what fits in the 256-byte request: 256 bytes minus
  `SET `, the address, the space after it and the line ending. For `text/label` that is 240 bytes
  with `\n`, 239 with `\r\n`.
- **A `light` `SET` with a JSON object** takes the fields of the MQTT light command: `state`,
  `brightness`, `color`, `color_temp`, `white`, `effect`, `transition`, `flash`. The device checks
  that the value is a JSON object and hands it to the light exactly as an MQTT command; fields the
  light does not understand are ignored, as on MQTT. For a light, `OK` therefore means "accepted",
  not "changed": the `EVT` that follows is the light's actual state.
- **Any other entity type** (a lock, a cover, a fan…) is listed `ro` with no domain, `GET` and `SET`
  answer `ERR unsupported-type`, and its changes produce no `EVT`. It is listed so a client can see
  that it exists.
- A `SET` is applied exactly as the matching MQTT command is, through the entity's own call, so the
  new state is also published on MQTT. Values are checked first — a switch or button word, a
  number within `[min, max]`, a text within its length bounds, a select option from the list, a
  light value that is `ON`/`OFF`/`TOGGLE` or a JSON object — and a value that fails the check is
  answered with `ERR bad-value` and nothing is written.

## What is exposed

The serial API offers what MQTT offers:

| Entity | On serial |
|---|---|
| an entity the firmware marks `internal` | hidden — not listed; `GET` and `SET` answer `ERR denied`, unless a visible entity has the same address, which then wins |
| `secret_ota` | hidden — it holds a credential |
| an entity whose object id starts with `ota_` | listed `ro`, readable and announced; `SET` answers `ERR denied` |
| everything else | listed, readable, announced and, for the writable types, writable |

"Everything else" includes, when the firmware has them, the device's system controls
(`button/system_restart`, `button/system_shutdown`, `button/system_factory_reset`) and
`button/location_request`. The `ota_` prefix is reserved for the device's firmware-update
entities, so a serial client cannot write them.

Two MQTT features have no serial counterpart: the `ota` command on the system command topic, and
the Wi-Fi scan that `location_request` publishes on the telemetry topic. Pressing
`location_request` over serial starts the scan; its result still goes to MQTT only.

**There is no authentication.** Physical access to the connector is the trust boundary: whoever can
reach the cable can reach the outputs the device drives.

## Errors

`ERR <code>` or `ERR <code> <detail>`. The detail — usually the address — is for a person reading a
terminal; a client branches on the code alone. The set of codes is closed:

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

The verbs, the device lines, the value encodings and the error codes above are a public contract.
A change a version-1 client could not handle raises the protocol version in the `HELLO` banner. A
client should check that number and refuse a version it does not know.

## Example

Recorded on a bench device. The `LIST` reply is shortened; periodic sensor `EVT` lines are left out.

```text
→ HELLO
← HELLO serial_api 1 bench-serial-d68abc 1.0.2-hardening
→ LIST
← ENT binary_sensor relay_feedback ro "Relay Feedback"
← ENT light strip rw "Strip" "Pulse" "Rainbow" "None"
← ENT sensor temperature ro "Temperature"
← ENT switch relay rw "Relay"
← ENT button system_restart rw "System Restart"
← ENT text_sensor ota_status ro "OTA status"
← ENT number volume rw "Volume" 0 100 5
← ENT text label rw "Label" 0 16
← ENT text ota_https_prod ro "OTA HTTPS prod" 0 255
← ENT select mode rw "Mode" "Easy" "Hard Mode"
← ENT lock door_lock ro "Door Lock"
← ENT mqtt status ro "MQTT status"
← END
→ SET switch/relay ON
← EVT switch/relay ON
← OK
← EVT binary_sensor/relay_feedback ON
→ SET switch/relay ON
← OK
→ SET number/volume 101
← ERR bad-value number/volume
→ SET light/strip {"state":"ON","brightness":128,"color":{"r":255,"g":0,"b":0}}
← EVT light/strip {"effect":"None","effect_index":0,"effect_count":2,"color_mode":"rgb","state":"ON","brightness":128,"color":{"r":255,"g":0,"b":0}}
← OK
→ GET switch/relay
← STATE switch/relay ON
→ SET text/ota_https_prod https://example.com/fw.bin
← ERR denied text/ota_https_prod
→ GET lock/door_lock
← ERR unsupported-type lock/door_lock
```

The second `SET switch/relay ON` changes nothing, so it is answered `OK` with no `EVT`. The relay
feedback `EVT` is a GPIO input that reads the relay's pin back: an entity the client did not touch,
reported because it changed.

## A client in Python

```python
import serial  # pyserial

KEYWORDS = {"HELLO", "ENT", "STATE", "EVT", "OK", "END", "PONG", "ERR"}
state = {}

port = serial.Serial("/dev/ttyUSB0", 115200, timeout=1)  # leave DTR and RTS at their defaults
port.write(b"\nHELLO\nGET *\n")  # the bare \n flushes any noise left in the device's input

while True:
    line = port.readline().decode("utf-8", "replace").rstrip("\r\n")
    word, _, rest = line.partition(" ")
    if word not in KEYWORDS:
        continue                      # boot text or noise
    if word in ("STATE", "EVT"):
        address, _, value = rest.partition(" ")
        state[address] = value        # current value of every entity
        if word == "EVT":
            print("now:", address, value)
    elif word == "HELLO":
        print("device:", rest)        # an unrequested HELLO means the device restarted:
                                      # send GET * and re-send your outputs
    elif word == "ERR":
        print("error:", rest)
```

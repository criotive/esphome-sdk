# Serial API — enabling it in firmware

This page is for whoever writes the device's ESPHome configuration. The protocol itself — what a
client sends and receives — is in [`serial-api.md`](serial-api.md).

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
including entities a room adds later. Which entities a client can see and write is fixed by the
exposure rule in [`serial-api.md`](serial-api.md#what-is-exposed).

## Choosing the pins

The line is **115200 baud, 8N1, no flow control**. Two pin choices exist, and each costs something:

| | Pins | Cable | Gives up |
|---|---|---|---|
| **The board's USB port** (UART0) | `"1"` / `"3"` | a USB cable | SW1 on `hds_v1_1` (set `hds_v1_1_sw1_enabled: "false"`); CAN on `hds_v1_1`, whose CAN RX is GPIO3; the logger's serial console; `improv_serial.yaml`, so Wi-Fi comes from a station slot, `wifi_ap.yaml` or `improv_ble.yaml` |
| **A slot pair** | any free pair, e.g. `"${slot_1_audio_tx_dat}"` / `"${slot_1_audio_rx}"` | a 3.3 V USB-serial adapter: **TX, RX and GND only** | nothing — CAN, SW1 and the console keep their pins |

`esphome config` rejects a configuration in which the serial API's pins collide with SW1, with a
`canbus:` block's configured pins, or — on UART0 — with the logger's console or `improv_serial`.
The check runs in the component itself, so it applies to every room that imports the module.

A slot pair is usually the better choice: the client sees no boot text, and nothing on the cable
can reset the board. Tell the client's developer which pins carry TX and RX, and that the board's
TX goes to the adapter's RX and the other way round.

## What the firmware must not do

- **No space in `firmware_version`.** The `HELLO` banner is five space-separated fields, and the
  version is the last; a space would add a field a client does not expect.
- **Keep MQTT or the web server in the build** if the device has lights. ESPHome compiles the light
  JSON encoding only alongside one of them; without it, lights are listed `ro` and answer
  `ERR unsupported-type`. Every device on the criotive SDK has MQTT, so this only matters for a
  configuration that drops `criotive_mqtt.yaml`.

## Supported ESPHome releases

The component builds on ESPHome 2026.9.x and on 2026.10.x, whose controller registry changed
(v0.6.1). The protocol is identical on both.

## Keeping the contract

The verbs, the device lines, the value encodings and the error codes in
[`serial-api.md`](serial-api.md) are a public contract. **No CI gate covers them**:
`check-contract-diff.sh` reads only the substitutions in `modules/` and `hardware/`, so a change to
the wire format is caught by review and by `tests/unit/test_serial_api_protocol.py` alone. An
incompatible change raises the protocol version the `HELLO` banner reports.

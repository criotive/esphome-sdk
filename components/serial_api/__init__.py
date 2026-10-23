"""Serial API: the device's entities on a UART, as a self-describing line protocol."""

import inspect
from typing import Any

import esphome.codegen as cg
import esphome.config_validation as cv
import esphome.final_validate as fv
from esphome.components import uart
from esphome.const import (
    CONF_BINARY_SENSOR,
    CONF_ID,
    CONF_NUMBER,
    CONF_PIN,
    CONF_RX_PIN,
    CONF_TX_PIN,
    CONF_UART_ID,
)
from esphome.core import CORE

CODEOWNERS = ["Arthur Komatsu"]
DEPENDENCIES = ["uart"]

CONF_FIRMWARE_VERSION = "firmware_version"

serial_api_ns = cg.esphome_ns.namespace("serial_api")
SerialAPI = serial_api_ns.class_("SerialAPI", uart.UARTDevice, cg.Component)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(SerialAPI),
            cv.Optional(CONF_FIRMWARE_VERSION): cv.string,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)

CONF_UART = "uart"
CONF_CANBUS = "canbus"
CONF_LOGGER = "logger"
CONF_IMPROV_SERIAL = "improv_serial"

# UART0 is the USB port on these boards. On HDS v1.1 GPIO1 is also SW1 and GPIO3 is CAN RX.
_UART0_TX_GPIO = 1
_UART0_RX_GPIO = 3
_SW1_FLAG = "hds_v1_1_sw1_enabled"


def _pin_number(pin: Any) -> int | None:
    """Pin-schema dicts carry the number under CONF_NUMBER; esp32_can's pins resolve to a bare int."""
    if isinstance(pin, dict):
        return pin.get(CONF_NUMBER)
    if isinstance(pin, int):
        return pin
    return None


def _serial_api_uart_pins(config: dict[str, Any], full_config: Any) -> set[int]:
    uart_ref = config.get(CONF_UART_ID)
    hub = uart_ref if isinstance(uart_ref, dict) else None
    if hub is None:
        for candidate in full_config.get(CONF_UART, []):
            if candidate.get(CONF_ID) == uart_ref:
                hub = candidate
                break
    if hub is None:
        return set()
    return {
        n
        for n in (_pin_number(hub.get(CONF_TX_PIN)), _pin_number(hub.get(CONF_RX_PIN)))
        if n is not None
    }


def _validate_pin_conflicts(config: dict[str, Any]) -> dict[str, Any]:
    """Fails `esphome config` for every consumer, including a room that imports the module by URL and
    never runs the SDK's scripts. Compares the configured pins, never the board name."""
    full_config = fv.full_config.get()
    serial_pins = _serial_api_uart_pins(config, full_config)
    if not serial_pins:
        return config

    if _UART0_TX_GPIO in serial_pins:
        for bin_sensor in full_config.get(CONF_BINARY_SENSOR, []):
            if _pin_number(bin_sensor.get(CONF_PIN)) == _UART0_TX_GPIO:
                raise cv.Invalid(
                    f"serial_api drives GPIO{_UART0_TX_GPIO}, but a binary_sensor (SW1) is already on "
                    f"GPIO{_UART0_TX_GPIO}. GPIO{_UART0_TX_GPIO} is UART0 TX; SW1 and the serial API "
                    f"cannot share it. Disable SW1 with `{_SW1_FLAG}: false`, or move the serial API "
                    f"to the room's slot UART pins."
                )

    if serial_pins & {_UART0_TX_GPIO, _UART0_RX_GPIO}:
        if CONF_IMPROV_SERIAL in full_config:
            raise cv.Invalid(
                "serial_api is on UART0 (GPIO1/GPIO3), which improv_serial also reads. Drop "
                "improv_serial.yaml, or move the serial API to the room's slot UART pins."
            )
        logger = full_config.get(CONF_LOGGER) or {}
        if logger.get("baud_rate", 0) != 0 and logger.get("hardware_uart", "UART0") == "UART0":
            raise cv.Invalid(
                "serial_api is on UART0 (GPIO1/GPIO3), which the logger's serial console also uses. "
                'Set `logger_baud_rate: "0"`, or move the serial API to the room\'s slot UART pins.'
            )

    can_pins: set[int] = set()
    for can in full_config.get(CONF_CANBUS, []):
        for opt in (CONF_TX_PIN, CONF_RX_PIN):
            n = _pin_number(can.get(opt))
            if n is not None:
                can_pins.add(n)
    collision = serial_pins & can_pins
    if collision:
        pin = sorted(collision)[0]
        raise cv.Invalid(
            f"serial_api drives GPIO{pin}, but a canbus: block already uses GPIO{pin}. CAN owns its "
            f"pins; move the serial API to free pins (typically the room's slot UART). This compares "
            f"the configured CAN tx_pin/rx_pin, not the board name."
        )
    return config


FINAL_VALIDATE_SCHEMA = cv.All(
    uart.final_validate_device_schema("serial_api", require_tx=True, require_rx=True),
    _validate_pin_conflicts,
)


async def to_code(config: dict[str, Any]) -> None:
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if CONF_FIRMWARE_VERSION in config:
        cg.add(var.set_firmware_version(config[CONF_FIRMWARE_VERSION]))

    # As components/api does; MQTT is not a Controller. ESPHome 2026.10 takes the controller and
    # generates the state-update dispatch to it; 2026.9 only counts it to size the registry.
    if inspect.signature(CORE.register_controller).parameters:
        CORE.register_controller(var)
    else:
        CORE.register_controller()

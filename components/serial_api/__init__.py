"""Serial API: the device's entities on a UART, as a self-describing line protocol."""

from typing import Any

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID
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

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "serial_api", require_tx=True, require_rx=True
)


async def to_code(config: dict[str, Any]) -> None:
    """Generate component code."""
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if CONF_FIRMWARE_VERSION in config:
        cg.add(var.set_firmware_version(config[CONF_FIRMWARE_VERSION]))

    # Sizes the ControllerRegistry, as components/api does; MQTT is not a Controller.
    CORE.register_controller()

"""Debug logs on demand: a system command opens a bounded window in which log lines go to MQTT.

The binary carries log statements up to `logger: level:`, and the runtime level follows the link:
while MQTT is down only warnings and errors reach the serial console, which is how a device that
cannot connect explains why over USB; once connected it is quiet at `level`, and a window raises it.
Nothing is persisted: a reboot always comes back offline, so a forgotten window cannot outlive the
next power cycle.
"""

from typing import Any

import esphome.codegen as cg
from esphome.components import logger
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_LEVEL

CODEOWNERS = ["Junio Teixeira"]
DEPENDENCIES = ["logger", "mqtt"]

debug_logs_ns = cg.esphome_ns.namespace("debug_logs")
DebugLogs = debug_logs_ns.class_("DebugLogs", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(DebugLogs),
        # The level a connected device logs at when no window is open: logger_level.
        cv.Required(CONF_LEVEL): logger.is_log_level,
    },
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config: dict[str, Any]) -> None:
    """Register the component and the logger callback slot it subscribes with."""
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_quiet_level(logger.LOG_LEVELS[config[CONF_LEVEL]]))
    # The logger sizes its callback list at codegen; a callback added without a slot is dropped.
    logger.request_log_listener()

"""The `linear_motor` switch platform.

```yaml
switch:
  - platform: linear_motor
    name: "Motor Linear da gaveta"
    id: motor_linear
    pin_a: ${slot_2_h_bridge_out_1}
    pin_b: ${slot_2_h_bridge_out_2}
    travel_time: 10s        # retract_time overrides it for the return stroke
```

`linear_motor.is_moving` is a condition, for holding a room reset open until the
stroke has finished:

```yaml
      - wait_until:
          condition:
            not:
              linear_motor.is_moving: motor_linear
          timeout: 15s
```

`restore_mode` is pinned to `ALWAYS_OFF` rather than left to the schema default,
which is the same value today: boot drives to the retracted end, and open-loop
that is the only way to reach a known position.
"""

from typing import Any

from esphome import automation, pins
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import CONF_ID, CONF_PIN_A, CONF_PIN_B

from .. import linear_motor_ns

LinearMotorSwitch = linear_motor_ns.class_(
    "LinearMotorSwitch",
    switch.Switch,
    cg.Component,
)
MovingCondition = linear_motor_ns.class_("MovingCondition", automation.Condition)

CONF_TRAVEL_TIME = "travel_time"
CONF_RETRACT_TIME = "retract_time"

# A zero-width stroke validates and never moves the actuator: `set_timeout(_, 0)`
# defers to the next loop.
_TRAVEL_TIME = cv.All(
    cv.positive_not_null_time_period,
    cv.positive_time_period_milliseconds,
)

CONFIG_SCHEMA = (
    switch.switch_schema(
        LinearMotorSwitch,
        block_inverted=True,
        default_restore_mode="ALWAYS_OFF",
    )
    .extend(
        {
            # Plain GPIOPins, so expander pins reach this through
            # PIN_SCHEMA_REGISTRY. Two outputs on the same pin are rejected by
            # ESPHome's own pin registry, which knows about `allow_other_uses`
            # and about two expanders sharing a pin number.
            cv.Required(CONF_PIN_A): pins.gpio_output_pin_schema,
            cv.Required(CONF_PIN_B): pins.gpio_output_pin_schema,
            cv.Required(CONF_TRAVEL_TIME): _TRAVEL_TIME,
            cv.Optional(CONF_RETRACT_TIME): _TRAVEL_TIME,
        },
    )
    .extend(cv.COMPONENT_SCHEMA)
)


@automation.register_condition(
    "linear_motor.is_moving",
    MovingCondition,
    automation.maybe_simple_id({cv.Required(CONF_ID): cv.use_id(LinearMotorSwitch)}),
)
async def is_moving_to_code(config, condition_id, template_arg, args):  # noqa: ANN001, ANN201, ARG001
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(condition_id, template_arg, parent)


async def to_code(config: dict[str, Any]) -> None:
    var = await switch.new_switch(config)
    await cg.register_component(var, config)

    cg.add(var.set_pin_a(await cg.gpio_pin_expression(config[CONF_PIN_A])))
    cg.add(var.set_pin_b(await cg.gpio_pin_expression(config[CONF_PIN_B])))

    travel = config[CONF_TRAVEL_TIME]
    cg.add(var.set_extend_time(travel))
    cg.add(var.set_retract_time(config.get(CONF_RETRACT_TIME, travel)))

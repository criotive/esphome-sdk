"""A DC linear actuator on an H-bridge, presented as a switch.

Open-loop: driven for a fixed time per direction, then held. The switch state
is the last direction commanded, not a measurement.

A `switch` and not a `cover` because the platform maps `cover` to an opaque
Object variable, and puzzle binding requires physical type `switch`.

C++ lives under `switch/`, as upstream keeps `template/switch/`: ESPHome
collects a platform's sources from the platform package.
"""

import esphome.codegen as cg

CODEOWNERS = ["Arthur Komatsu"]

linear_motor_ns = cg.esphome_ns.namespace("linear_motor")

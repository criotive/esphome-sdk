"""Per-device MQTT identity, read from a flash partition instead of compiled into the image.

Everything that distinguishes one device from another — broker, credentials, client id, topic
prefix and the CA that validates the broker — lives in a record written into a partition of its own
at flash time. The image itself is identical for every device in a fleet, which is the whole point:
without this, an artifact is bound to the one device whose credentials were substituted into it.

A partition of our own rather than NVS, verified against ESPHome 2026.7.0-dev:
`ESP32Preferences::open()` runs `nvs_flash_erase()` unattended at boot when `nvs_open("esphome", …)`
fails, and `ESP32Preferences::reset()` — what a `factory_reset` button calls — *is*
`nvs_flash_erase()`. That erases only the partition labelled `nvs`, so a dedicated partition
survives both, and the Wi-Fi the user provisioned into NVS survives a credential rewrite.

The partition is 128 KB although the record needs 8. That costs nothing measurable:
`_get_app_partition_size()` subtracts the custom partitions and then aligns `app0`/`app1` DOWN to
64 KB, so every size up to 0x20000 leaves the app slots at exactly the same value. Asking for 8 KB
strands the other 120 KB in unreachable flash for no gain.
"""

from typing import Any

import esphome.codegen as cg
import esphome.config_validation as cv
import esphome.final_validate as fv
from esphome.const import CONF_ID
from esphome.core import CORE

# `add_partition` first shipped in ESPHome 2026.4.0 (esphome#7682). Importing it at module scope on
# an older release raises ImportError before any validator can run, and the user sees a Python
# traceback that never mentions partitions. Absorb it here so the schema can say what is wrong.
try:
    from esphome.components.esp32 import CONF_PARTITIONS, add_partition
except ImportError:  # pragma: no cover - depends on the host's ESPHome version
    CONF_PARTITIONS = "partitions"
    add_partition = None

CODEOWNERS = ["Junio Teixeira"]
DEPENDENCIES = ["esp32", "mqtt"]
AUTO_LOAD = ["json"]

PARTITION_NAME = "ciotcfg"
PARTITION_TYPE = "data"
# 0x40 is in the application-defined data range: ESP-IDF assigns no meaning to it, so nothing in the
# framework treats this partition as a filesystem it may format. Passed as an int on purpose —
# `_validate_partition` applies its subtype whitelist (nvs, spiffs, fat, …) only to strings.
PARTITION_SUBTYPE = 0x40
PARTITION_SIZE = 0x20000

MIN_ESPHOME_VERSION = "2026.4.0"

ciotcfg_ns = cg.esphome_ns.namespace("ciotcfg")
CiotCfg = ciotcfg_ns.class_("CiotCfg", cg.Component)


def _require_partition_api(config: dict[str, Any]) -> dict[str, Any]:
    """Refuse a build whose ESPHome cannot register a custom partition."""
    if add_partition is None:
        raise cv.Invalid(
            f"ciotcfg needs esphome.components.esp32.add_partition, which first shipped in "
            f"ESPHome {MIN_ESPHOME_VERSION}. Pin this project to {MIN_ESPHOME_VERSION} or later. "
            f"On an older release the partition is never added to the table, so the device flashes "
            f"cleanly and then boots with no credentials.",
        )

    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(CiotCfg),
        },
    ).extend(cv.COMPONENT_SCHEMA),
    _require_partition_api,
)


def _csv_has_partition_row(csv: str) -> bool:
    """True when a non-comment row names `ciotcfg` with the data type and subtype the device reads."""
    for line in csv.splitlines():
        fields = [field.strip() for field in line.split("#", 1)[0].split(",")]
        if len(fields) < 3 or fields[0] != PARTITION_NAME:
            continue
        try:
            subtype = int(fields[2], 0)
        except ValueError:
            return False
        return fields[1] == PARTITION_TYPE and subtype == PARTITION_SUBTYPE
    return False


def _require_partition_row(config: dict[str, Any]) -> dict[str, Any]:
    """Refuse a config that supplies its own partition CSV without a `ciotcfg` row.

    `esp32: partitions:` accepts a list OR a file path. Given a path, ESPHome registers it as an
    extra build file, and `copy_files()` then skips `get_partition_csv()` entirely — which is the
    only place `add_partition()`'s registrations are ever read. Every component-registered partition
    is dropped, with no warning and no error from the esp32 component: the build succeeds, the flash
    succeeds, and the device boots to a table that has no `ciotcfg` in it.

    So the component has to catch it itself. The zigbee component does the same for `zb_fct`, for
    the same reason.
    """
    esp32_config = fv.full_config.get().get("esp32", {})
    partitions = esp32_config.get(CONF_PARTITIONS)

    if partitions is None:
        return config

    if isinstance(partitions, list):
        # add_partition raises a bare ValueError on a duplicate name, at codegen, as a traceback.
        if any(entry.get("name") == PARTITION_NAME for entry in partitions):
            raise cv.Invalid(
                f"esp32: partitions: already lists '{PARTITION_NAME}'. The ciotcfg component "
                f"registers that partition itself; remove the row."
            )
        return config

    csv = CORE.relative_config_path(partitions).read_text(encoding="utf8")
    if not _csv_has_partition_row(csv):
        raise cv.Invalid(
            f"'{partitions}' is used as the partition table verbatim, and it has no "
            f"'{PARTITION_NAME}' {PARTITION_TYPE} partition of subtype 0x{PARTITION_SUBTYPE:X}. ESPHome silently drops every partition a component "
            f"registers when the table comes from a file, so ciotcfg would have nowhere to read "
            f"from. Add this row:\n"
            f"  {PARTITION_NAME}, {PARTITION_TYPE}, 0x{PARTITION_SUBTYPE:X}, , 0x{PARTITION_SIZE:X},",
        )

    return config


FINAL_VALIDATE_SCHEMA = _require_partition_row


async def to_code(config: dict[str, Any]) -> None:
    """Register the partition and the component that reads it."""
    add_partition(PARTITION_NAME, PARTITION_TYPE, PARTITION_SUBTYPE, PARTITION_SIZE)

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

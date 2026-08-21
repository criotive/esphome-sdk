#!/usr/bin/env bash
#
# check-serial-api-pins.sh — the serial_api pin contract fails for the RIGHT reason.
#
# Runs `esphome config` over each serial_api negative fixture and asserts a non-zero exit AND the
# expected error text. The text is the oracle: an unreachable git ref, or ESPHome's generic pin-reuse
# check, also exits non-zero while proving nothing. The enforcement is the component's own
# FINAL_VALIDATE_SCHEMA; this harness only proves it still speaks. Missing fixtures are a failure.
#
# Usage:
#   check-serial-api-pins.sh [ROOT]     # check ROOT/tests/negative serial_api fixtures (default: repo)
#   check-serial-api-pins.sh --self-test
#
# The esphome binary is taken from $ESPHOME (default: `esphome` on PATH).
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Run `esphome config` on one file with an isolated data dir; print combined output, return its exit.
_esphome_config_out() {
  local file="$1" data_dir out rc
  data_dir="$(mktemp -d)"
  out="$(ESPHOME_DATA_DIR="$data_dir" "${ESPHOME:-esphome}" config "$file" 2>&1)"; rc=$?
  rm -rf "$data_dir"
  printf '%s' "$out"
  return "$rc"
}

# Assert `esphome config <file>` FAILS and its output matches <regex>. Distinguishes the two ways a
# non-zero exit can still be a regression: the config unexpectedly passing (enforcement gone), and the
# config failing for the wrong reason (the trap above).
assert_fails_matching() {
  local file="$1" regex="$2" label="$3" out rc
  if [ ! -f "$file" ]; then
    echo "check-serial-api-pins: FAIL — ${label}: missing fixture ${file}"
    return 1
  fi
  out="$(_esphome_config_out "$file")"; rc=$?
  if [ "$rc" -eq 0 ]; then
    echo "check-serial-api-pins: FAIL — ${label}: ${file} PASSED esphome config, but the pin contract must reject it — the component enforcement is missing"
    return 1
  fi
  if ! grep -qE "$regex" <<<"$out"; then
    echo "check-serial-api-pins: FAIL — ${label}: ${file} failed, but not with /${regex}/ — it died for the wrong reason (e.g. an unreachable external_components ref), proving nothing"
    return 1
  fi
  echo "check-serial-api-pins: ok — ${label}: ${file} rejected, error matches /${regex}/"
  return 0
}

check_root() {
  local root="$1" neg="$1/tests/negative" rc=0
  # The required-pin contract: each omit_ fixture must name the pin it withholds.
  assert_fails_matching "$neg/omit_serial_api_tx_pin.yaml" 'serial_api_tx_pin' "tx_pin omitted" || rc=1
  assert_fails_matching "$neg/omit_serial_api_rx_pin.yaml" 'serial_api_rx_pin' "rx_pin omitted" || rc=1
  # The component pin-conflict rule: the SW1 message names the flag, the CAN message names its own text.
  # Matching these proves the component's FINAL_VALIDATE fired, not ESPHome's generic pin-reuse check.
  assert_fails_matching "$neg/serial_api_pin_conflict_sw1.yaml" 'hds_v1_1_sw1_enabled' "SW1/GPIO1 conflict" || rc=1
  assert_fails_matching "$neg/serial_api_pin_conflict_can.yaml" 'not the board name' "CAN pin conflict" || rc=1
  assert_fails_matching "$neg/serial_api_uart0_logger.yaml" 'logger_baud_rate' "UART0 logger conflict" || rc=1
  assert_fails_matching "$neg/serial_api_uart0_improv.yaml" 'improv_serial also reads' "UART0 improv conflict" || rc=1
  if [ "$rc" -eq 0 ]; then
    echo "check-serial-api-pins: all serial_api pin fixtures fail for the right reason"
  fi
  return "$rc"
}

# ── self-test ───────────────────────────────────────────────────────────────────────────────────
# Proves the harness's own FAIL paths with a STUB esphome, so it needs no real component: (1) a fixture
# that fails naming the pin is accepted; (2) a fixture that PASSES is flagged; (3) a fixture that fails
# with the WRONG text (the ref-fetch trap) is flagged. If any verdict were wrong, the gate would be
# theatre — green while proving nothing.
self_test() {
  local tmp rc=0 stub fixture
  tmp="$(mktemp -d)"
  fixture="$tmp/any.yaml"; : >"$fixture"
  stub="$tmp/esphome"
  cat >"$stub" <<'SH'
#!/usr/bin/env bash
# Stub esphome: ignore args, emit $STUB_OUT, exit $STUB_RC.
printf '%s\n' "${STUB_OUT:-}"
exit "${STUB_RC:-0}"
SH
  chmod +x "$stub"

  if ESPHOME="$stub" STUB_RC=2 STUB_OUT="ZeroDivisionError ... serial_api_tx_pin missing" \
       assert_fails_matching "$fixture" 'serial_api_tx_pin' "selftest expected-text" >/dev/null 2>&1; then
    echo "self-test: PASS — a fixture that fails naming the pin is accepted"
  else
    echo "self-test: FAILED — a correctly-failing fixture was rejected"; rc=1
  fi

  if ESPHOME="$stub" STUB_RC=0 STUB_OUT="INFO Configuration is valid!" \
       assert_fails_matching "$fixture" 'serial_api_tx_pin' "selftest passing" >/dev/null 2>&1; then
    echo "self-test: FAILED — a fixture that PASSED config was not flagged (missing enforcement would slip)"; rc=1
  else
    echo "self-test: PASS — a fixture that passes config is flagged as a violation"
  fi

  if ESPHOME="$stub" STUB_RC=2 STUB_OUT="error: couldn't find remote ref local-test" \
       assert_fails_matching "$fixture" 'serial_api_tx_pin' "selftest wrong-reason" >/dev/null 2>&1; then
    echo "self-test: FAILED — a fixture that died for the WRONG reason was accepted (the ref-fetch trap)"; rc=1
  else
    echo "self-test: PASS — a fixture that fails with the wrong error is flagged"
  fi

  rm -rf "$tmp"
  if [ "$rc" -eq 0 ]; then echo "self-test: all cases passed"; fi
  return "$rc"
}

main() {
  case "${1:-}" in
    --self-test) self_test ;;
    "" ) check_root "$(dirname "$SCRIPT_DIR")" ;;
    *  ) check_root "$1" ;;
  esac
}

main "$@"

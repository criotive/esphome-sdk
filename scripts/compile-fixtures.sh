#!/usr/bin/env bash
#
# compile-fixtures.sh — `esphome compile` each fixture given, then fail if any did. One failure does
# not stop the rest, so a shard reports every broken fixture in a single run.
set -uo pipefail

[ "$#" -gt 0 ] || { echo "usage: compile-fixtures.sh <fixture>..." >&2; exit 2; }
failed=()
for fixture in "$@"; do
  echo "::group::esphome compile ${fixture}"
  esphome compile "$fixture" || failed+=("$fixture")
  echo "::endgroup::"
done
command -v ccache >/dev/null && ccache --show-stats || true
if [ "${#failed[@]}" -gt 0 ]; then
  echo "::error::esphome compile failed: ${failed[*]}"
  exit 1
fi
echo "compiled $# fixture(s)"

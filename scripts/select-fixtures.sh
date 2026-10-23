#!/usr/bin/env bash
#
# select-fixtures.sh — the validate fixtures a change can break, so CI compiles only those.
#
# A fixture is selected when it changed, when it imports (a packages `path:`) a changed modules/ or
# hardware/ file, or when it or a file it imports names a changed component — a component whose
# sources name a changed component counts as changed too. Names match as whole words, so the
# selection errs towards compiling more. A change to the CI definition selects only the smoke
# fixture; anything outside components/, modules/, hardware/ and tests/validate/ selects nothing.
# The release merge compiles every fixture regardless (release-gate.yml), which is what makes a
# narrower selection everywhere else safe.
#
# Usage:
#   select-fixtures.sh <base> <head> [ROOT]   # fixtures affected by base...head, one per line
#   select-fixtures.sh --all [ROOT]           # every fixture
#   select-fixtures.sh --self-test
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SMOKE_FIXTURE="tests/validate/recommended_esp32dev.yaml"

all_fixtures() {
  local root="$1"
  (cd "$root" && find tests/validate -maxdepth 1 \( -name '*.yaml' -o -name '*.yml' \) | sort)
}

imports_of() {
  grep -oE 'path: *(modules|hardware)/[A-Za-z0-9_.-]+\.ya?ml' "$1" | sed -E 's/^path: *//' | sort -u || true
}

# Reads changed paths on stdin and prints the selected fixtures.
select_from_changes() {
  local root="$1" path comp fixture dir grew
  local -A components=() files=() fixtures=() selected=()
  local smoke=0
  while IFS= read -r path || [ -n "$path" ]; do
    case "$path" in
      components/*/*) components["$(cut -d/ -f2 <<<"$path")"]=1 ;;
      modules/*|hardware/*) files["$path"]=1 ;;
      tests/validate/*.yaml|tests/validate/*.yml) fixtures["$path"]=1 ;;
      .github/workflows/*|scripts/select-fixtures.sh|scripts/materialize-sdk-ref.sh) smoke=1 ;;
    esac
  done

  grew=1
  while [ "$grew" -eq 1 ]; do
    grew=0
    for dir in "$root"/components/*/; do
      comp="$(basename "$dir")"
      [ -n "${components[$comp]:-}" ] && continue
      for changed in "${!components[@]}"; do
        if grep -rqw -- "$changed" "$dir"; then
          components["$comp"]=1
          grew=1
          break
        fi
      done
    done
  done

  while IFS= read -r fixture; do
    if [ -n "${fixtures[$fixture]:-}" ]; then
      selected["$fixture"]=1
      continue
    fi
    local sources=("$root/$fixture")
    while IFS= read -r path; do
      [ -z "$path" ] && continue
      if [ -n "${files[$path]:-}" ]; then
        selected["$fixture"]=1
        break
      fi
      [ -f "$root/$path" ] && sources+=("$root/$path")
    done < <(imports_of "$root/$fixture")
    [ -n "${selected[$fixture]:-}" ] && continue
    for comp in "${!components[@]}"; do
      if grep -qw -- "$comp" "${sources[@]}"; then
        selected["$fixture"]=1
        break
      fi
    done
  done < <(all_fixtures "$root")

  if [ "$smoke" -eq 1 ] && [ -f "$root/$SMOKE_FIXTURE" ]; then
    selected["$SMOKE_FIXTURE"]=1
  fi
  for fixture in "${!selected[@]}"; do echo "$fixture"; done | sort
}

self_test() {
  local tmp rc=0
  tmp="$(mktemp -d)"
  mkdir -p "$tmp/components/alpha" "$tmp/components/beta" "$tmp/modules" "$tmp/hardware" \
    "$tmp/tests/validate" "$tmp/docs"
  echo 'void alpha();' >"$tmp/components/alpha/alpha.cpp"
  echo '#include "esphome/components/alpha/alpha.h"' >"$tmp/components/beta/beta.cpp"
  printf 'external_components:\n  - components: [alpha]\n' >"$tmp/modules/ma.yaml"
  printf 'external_components:\n  - components: [beta]\n' >"$tmp/modules/mb.yaml"
  echo 'esp32: {}' >"$tmp/hardware/h.yaml"
  printf 'packages:\n  c:\n    files:\n      - path: modules/ma.yaml\n' >"$tmp/tests/validate/f1.yaml"
  printf 'packages:\n  c:\n    files:\n      - path: hardware/h.yaml\n      - path: modules/core.yaml\n' \
    >"$tmp/tests/validate/f2.yaml"
  printf 'packages:\n  c:\n    files:\n      - path: modules/mb.yaml\n' >"$tmp/tests/validate/recommended_esp32dev.yaml"

  check() {
    local label="$1" changes="$2" want="$3" got
    got="$(printf '%s' "$changes" | select_from_changes "$tmp" | tr '\n' ' ' | sed 's/ $//')"
    if [ "$got" = "$want" ]; then
      echo "self-test: PASS — $label"
    else
      echo "self-test: FAILED — $label: got '$got', want '$want'"
      rc=1
    fi
  }
  local f1=tests/validate/f1.yaml f2=tests/validate/f2.yaml rec=tests/validate/recommended_esp32dev.yaml
  check "a component selects its importers and those of components naming it" \
    'components/alpha/alpha.cpp' "$f1 $rec"
  check "a component named by nothing else selects only its importers" 'components/beta/beta.cpp' "$rec"
  check "a hardware file selects the fixtures importing it" 'hardware/h.yaml' "$f2"
  check "a changed fixture selects itself" "$f1" "$f1"
  check "a deleted fixture selects nothing" 'tests/validate/gone.yaml' ""
  check "docs select nothing" $'docs/x.md\nREADME.md\nscripts/check-sdk-ref.sh' ""
  check "the CI definition selects the smoke fixture" '.github/workflows/validate.yml' "$rec"

  rm -rf "$tmp"
  [ "$rc" -eq 0 ] && echo "self-test: all cases passed"
  return "$rc"
}

main() {
  case "${1:-}" in
    --self-test) self_test ;;
    --all) all_fixtures "${2:-$(dirname "$SCRIPT_DIR")}" ;;
    "") echo "usage: select-fixtures.sh <base> <head> [ROOT] | --all [ROOT] | --self-test" >&2; exit 2 ;;
    *)
      local root="${3:-$(dirname "$SCRIPT_DIR")}"
      git -C "$root" diff --name-only --no-renames "$1...$2" | select_from_changes "$root"
      ;;
  esac
}

main "$@"

#include "linear_motor_switch.h"

#include "esphome/core/log.h"

#include <cinttypes>

namespace esphome {
namespace linear_motor {

static const char *const TAG = "linear_motor.switch";

// One name, so arming a direction's end-of-travel cancels the pending one:
// `set_timeout` is keyed on (component, name).
static const char *const HOLD_TIMEOUT = "hold";

void LinearMotorSwitch::setup() {
  // Level before and after `setup()`, as gpio and hbridge do: `pinMode(OUTPUT)`
  // latches the pad's reset value, so writing only afterwards drives one output
  // low for that window.
  this->pin_a_->digital_write(true);
  this->pin_a_->setup();
  this->pin_a_->digital_write(true);

  this->pin_b_->digital_write(true);
  this->pin_b_->setup();
  this->pin_b_->digital_write(true);

  const optional<bool> initial_state = this->get_initial_state_with_restore_mode();
  if (initial_state.has_value()) {
    if (*initial_state) {
      this->turn_on();
    } else {
      this->turn_off();
    }
  }
}

void LinearMotorSwitch::write_state(bool state) {
  // Runs on every command, not only on a change -- `Switch::turn_on`/`turn_off`
  // call straight through and only `publish_state` dedups. A reset driving an
  // actuator already believed retracted depends on that.
  if (state) {
    this->pin_a_->digital_write(true);
    this->pin_b_->digital_write(false);
  } else {
    this->pin_a_->digital_write(false);
    this->pin_b_->digital_write(true);
  }
  this->moving_ = true;

  const uint32_t travel_ms = state ? this->extend_ms_ : this->retract_ms_;
  ESP_LOGD(TAG, "'%s': driving %s for %" PRIu32 "ms", this->get_name().c_str(),
           state ? "out" : "back", travel_ms);
  // Re-keying cancels the old direction's hold, which would otherwise land
  // mid-travel through this one.
  this->set_timeout(HOLD_TIMEOUT, travel_ms, [this]() { this->hold_(); });

  this->publish_state(state);
}

void LinearMotorSwitch::hold_() {
  this->pin_a_->digital_write(true);
  this->pin_b_->digital_write(true);
  this->moving_ = false;
}

void LinearMotorSwitch::dump_config() {
  LOG_SWITCH("", "Linear Motor Switch", this);
  LOG_PIN("  Pin A: ", this->pin_a_);
  LOG_PIN("  Pin B: ", this->pin_b_);
  ESP_LOGCONFIG(TAG, "  Extend time: %" PRIu32 "ms", this->extend_ms_);
  ESP_LOGCONFIG(TAG, "  Retract time: %" PRIu32 "ms", this->retract_ms_);
}

}  // namespace linear_motor
}  // namespace esphome

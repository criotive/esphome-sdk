#pragma once

#include "esphome/components/switch/switch.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace linear_motor {

/// ON drives out, OFF drives back, each for its own time, then both outputs
/// are driven together and the actuator holds.
class LinearMotorSwitch : public switch_::Switch, public Component {
 public:
  void set_pin_a(GPIOPin *pin) { this->pin_a_ = pin; }
  void set_pin_b(GPIOPin *pin) { this->pin_b_ = pin; }
  void set_extend_time(uint32_t extend_ms) { this->extend_ms_ = extend_ms; }
  void set_retract_time(uint32_t retract_ms) { this->retract_ms_ = retract_ms; }

  /// True while a stroke is still running. A room reset must not report the
  /// room ready until this is false.
  bool is_moving() const { return this->moving_; }

  void setup() override;
  void dump_config() override;

  /// Park the outputs on reboot rather than leaving a stroke driven through it.
  /// Covers the shutdown window only -- the scheduler is already stalled during
  /// the OTA download itself.
  void on_shutdown() override { this->hold_(); }

  float get_setup_priority() const override { return setup_priority::HARDWARE - 2.0f; }

 protected:
  void write_state(bool state) override;

  /// Both outputs driven together — the resting state between movements.
  void hold_();

  GPIOPin *pin_a_{nullptr};
  GPIOPin *pin_b_{nullptr};
  uint32_t extend_ms_{0};
  uint32_t retract_ms_{0};
  bool moving_{false};
};

template<typename... Ts> class MovingCondition : public Condition<Ts...> {
 public:
  explicit MovingCondition(LinearMotorSwitch *parent) : parent_(parent) {}
  bool check(const Ts &...x) override { return this->parent_->is_moving(); }

 protected:
  LinearMotorSwitch *parent_;
};

}  // namespace linear_motor
}  // namespace esphome

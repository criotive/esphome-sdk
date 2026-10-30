#pragma once

#include "esphome/core/component.h"
#include "esphome/core/log.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace esphome::debug_logs {

/// Lines per second a window may publish, and the burst it may spend at once. Log lines share
/// ESPHome's outbound MQTT queue with entity states (mqtt_backend_esp32.h, MQTT_QUEUE_LENGTH 30 in
/// 2026.9.1), and a full queue drops whatever arrives next — a puzzle's state included. The burst
/// leaves most of that queue to the entities.
static constexpr uint32_t LINES_PER_SECOND = 10;
static constexpr uint32_t BURST_LINES = 10;

/// A window never outlives this, whatever the command asked for.
static constexpr int MAX_MINUTES = 120;

/// Longest line published; the logger's own buffer is 512 bytes by default.
static constexpr size_t MAX_LINE = 512;

/// The level while MQTT is down, when only the serial console can show anything. Warnings and errors
/// carry every connection failure ESPHome reports, a few lines per retry; more would block the main
/// loop whenever a burst outgrows the console's 512-byte UART buffer. core.yaml boots at the same
/// level, so warnings from before this component's setup reach the console too.
static constexpr uint8_t OFFLINE_LEVEL = ESPHOME_LOG_LEVEL_WARN;

/// Publishes log lines to `<topic prefix>/debug` while a window is open, and keeps the console at
/// OFFLINE_LEVEL while MQTT is down.
///
/// The window raises the logger's runtime level and closes on its own: at its deadline, on a
/// command, or at the next boot. Lines are QoS 0 and never retained: a retained log topic replays one
/// stale line to every new subscriber.
class DebugLogs : public Component {
 public:
  void setup() override;
  void dump_config() override;

  void set_quiet_level(uint8_t level) { this->quiet_level_ = level; }

  /// Opens a window at `level` for `minutes`, replacing any open one. "NONE" or `minutes` <= 0
  /// closes it instead. The level is capped at the one the binary was compiled with.
  void open(const std::string &level, int minutes);
  void close();

 protected:
  uint8_t idle_level_() const;
  void follow_link_();
  void on_log_(const char *message, size_t length);
  bool take_line_();
  void report_dropped_();
  void publish_status_(const char *format, ...);
  void publish_(const char *payload, size_t length);

  uint8_t quiet_level_{ESPHOME_LOG_LEVEL_NONE};
  bool connected_{false};
  bool open_{false};
  uint32_t milli_lines_{0};
  uint32_t last_refill_ms_{0};
  uint32_t dropped_{0};
  std::string topic_;
  char line_[MAX_LINE];
};

}  // namespace esphome::debug_logs

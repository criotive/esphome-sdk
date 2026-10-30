#include "debug_logs.h"

#include "esphome/components/logger/logger.h"
#include "esphome/components/mqtt/mqtt_client.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <strings.h>

namespace esphome::debug_logs {

static const char *const TAG = "debug_logs";

// Indexed by ESPHome's ESPHOME_LOG_LEVEL_* values.
static const char *const LEVEL_NAMES[] = {"NONE", "ERROR", "WARN", "INFO", "CONFIG", "DEBUG", "VERBOSE", "VERY_VERBOSE"};
static constexpr int LEVEL_COUNT = sizeof(LEVEL_NAMES) / sizeof(LEVEL_NAMES[0]);

static int parse_level(const std::string &name) {
  for (int i = 0; i < LEVEL_COUNT; i++) {
    if (strcasecmp(name.c_str(), LEVEL_NAMES[i]) == 0)
      return i;
  }
  return -1;
}

void DebugLogs::setup() {
  // Called on the main task only: the logger queues other tasks' lines and replays them from its loop.
  logger::global_logger->add_log_callback(
      this, [](void *self, uint8_t level, const char *tag, const char *message, size_t length) {
        static_cast<DebugLogs *>(self)->on_log_(message, length);
      });

  logger::global_logger->set_log_level(this->idle_level_());
  this->set_interval("link", 1000, [this]() { this->follow_link_(); });
}

void DebugLogs::dump_config() {
  ESP_LOGCONFIG(TAG,
                "Debug logs:\n"
                "  Compiled level: %s\n"
                "  Connected level: %s",
                LEVEL_NAMES[ESPHOME_LOG_LEVEL], LEVEL_NAMES[this->quiet_level_]);
}

uint8_t DebugLogs::idle_level_() const {
  if (this->connected_)
    return this->quiet_level_;
  return std::min<uint8_t>(std::max(this->quiet_level_, OFFLINE_LEVEL), ESPHOME_LOG_LEVEL);
}

void DebugLogs::follow_link_() {
  const bool connected = mqtt::global_mqtt_client->is_connected();
  if (connected == this->connected_)
    return;

  this->connected_ = connected;
  // A window only exists to publish, so it ends with the connection rather than leaving the console at
  // its level while offline.
  if (this->open_ && !connected) {
    this->close();
    return;
  }

  if (!this->open_)
    logger::global_logger->set_log_level(this->idle_level_());
}

void DebugLogs::open(const std::string &level, int minutes) {
  const int requested = parse_level(level);
  if (requested < 0) {
    this->publish_status_("debug logs: unknown level '%s'", level.c_str());
    return;
  }

  if (requested == ESPHOME_LOG_LEVEL_NONE || minutes <= 0) {
    this->close();
    return;
  }

  if (ESPHOME_LOG_LEVEL == ESPHOME_LOG_LEVEL_NONE) {
    this->publish_status_("debug logs unavailable: this firmware was compiled without logs");
    return;
  }

  const int applied = std::min(requested, ESPHOME_LOG_LEVEL);
  minutes = std::min(minutes, MAX_MINUTES);

  this->topic_ = mqtt::global_mqtt_client->get_topic_prefix() + "/debug";
  this->open_ = true;
  this->dropped_ = 0;
  this->milli_lines_ = BURST_LINES * 1000;
  this->last_refill_ms_ = millis();
  logger::global_logger->set_log_level(applied);

  this->set_timeout("window", static_cast<uint32_t>(minutes) * 60 * 1000, [this]() { this->close(); });
  this->set_interval("dropped", 5000, [this]() { this->report_dropped_(); });

  this->publish_status_("debug logs on: %s for %d min%s", LEVEL_NAMES[applied], minutes,
                        applied < requested ? " (the most verbose level this firmware was compiled with)" : "");
}

void DebugLogs::close() {
  this->cancel_timeout("window");
  this->cancel_interval("dropped");
  this->report_dropped_();
  logger::global_logger->set_log_level(this->idle_level_());
  if (this->open_)
    this->publish_status_("debug logs off");
  this->open_ = false;
}

void DebugLogs::on_log_(const char *message, size_t length) {
  if (!this->open_ || !mqtt::global_mqtt_client->is_connected())
    return;

  if (!this->take_line_()) {
    this->dropped_++;
    return;
  }

  // The logger wraps each line in ANSI color codes, which mean nothing off a terminal.
  size_t out = 0;
  for (size_t i = 0; i < length && out < MAX_LINE; i++) {
    if (message[i] == '\033') {
      while (i < length && message[i] != 'm')
        i++;
      continue;
    }
    this->line_[out++] = message[i];
  }

  this->publish_(this->line_, out);
}

bool DebugLogs::take_line_() {
  const uint32_t now = millis();
  // Bounded before multiplying, so an hour-long gap cannot overflow the refill.
  const uint32_t elapsed = std::min<uint32_t>(now - this->last_refill_ms_, BURST_LINES * 1000);
  this->last_refill_ms_ = now;
  this->milli_lines_ = std::min<uint32_t>(this->milli_lines_ + elapsed * LINES_PER_SECOND, BURST_LINES * 1000);

  if (this->milli_lines_ < 1000)
    return false;

  this->milli_lines_ -= 1000;
  return true;
}

void DebugLogs::report_dropped_() {
  if (this->dropped_ == 0 || !mqtt::global_mqtt_client->is_connected())
    return;

  this->publish_status_("debug logs: %u lines dropped, over %u lines/s", static_cast<unsigned>(this->dropped_),
                        static_cast<unsigned>(LINES_PER_SECOND));
  this->dropped_ = 0;
}

void DebugLogs::publish_status_(const char *format, ...) {
  char text[160];
  va_list args;
  va_start(args, format);
  const int written = vsnprintf(text, sizeof(text), format, args);
  va_end(args);

  if (written > 0)
    this->publish_(text, std::min<size_t>(written, sizeof(text) - 1));
}

void DebugLogs::publish_(const char *payload, size_t length) {
  if (this->topic_.empty())
    this->topic_ = mqtt::global_mqtt_client->get_topic_prefix() + "/debug";

  mqtt::global_mqtt_client->publish(this->topic_.c_str(), payload, length, 0, false);
}

}  // namespace esphome::debug_logs

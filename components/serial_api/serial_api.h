#pragma once

// Entities are enumerated through the entity_types.h X-macro, so a new entity needs no edit here.
// Values need one adapter per type because ESPHome's per-type accessors differ; a type without one
// falls to the generic template, is listed `ro` and answers ERR unsupported-type.

#include <cmath>
#include <span>
#include <string>
#include <vector>

#include "serial_api_protocol.h"

#include "esphome/components/uart/uart.h"
#include "esphome/core/application.h"
#include "esphome/core/component.h"
// ESPHome 2026.10 removed the Controller base: a controller is any class with the on_*_update()
// members, and ESPHome generates the calls to it instead of registering it at setup.
#if __has_include("esphome/core/controller.h")
#include "esphome/core/controller.h"
#define SERIAL_API_CONTROLLER_BASE
#endif
#include "esphome/core/entity_base.h"
#include "esphome/core/helpers.h"
#include "esphome/core/version.h"
#ifdef USE_CONTROLLER_REGISTRY
#include "esphome/core/controller_registry.h"
#endif
// ESPHome 2026.9.1 compiles light_json_schema.cpp only alongside mqtt or web_server.
#if defined(USE_LIGHT) && defined(USE_JSON) && (defined(USE_MQTT) || defined(USE_WEBSERVER))
#define SERIAL_API_USE_LIGHT
#include "esphome/components/light/light_json_schema.h"
#endif
#ifdef USE_MQTT
#include "esphome/components/mqtt/mqtt_client.h"
#endif

namespace esphome {
namespace serial_api {

enum class ReadResult : uint8_t {
  OK,
  NO_STATE,     // button: GET answers ERR no-state
  UNSUPPORTED,  // GET answers ERR unsupported-type
};

template<class T> inline ReadResult serial_read_state(T * /*e*/, std::string & /*out*/) {
  return ReadResult::UNSUPPORTED;
}

#ifdef USE_SWITCH
inline ReadResult serial_read_state(switch_::Switch *e, std::string &out) {
  out = e->state ? "ON" : "OFF";
  return ReadResult::OK;
}
#endif
#ifdef USE_BINARY_SENSOR
inline ReadResult serial_read_state(binary_sensor::BinarySensor *e, std::string &out) {
  out = e->state ? "ON" : "OFF";
  return ReadResult::OK;
}
#endif
#ifdef USE_SENSOR
inline ReadResult serial_read_state(sensor::Sensor *e, std::string &out) {
  if (!e->has_state() || std::isnan(e->state)) {
    out = protocol::format_state_float(e->state, false);
    return ReadResult::OK;
  }
  // The digits the MQTT state topic carries.
  char buf[VALUE_ACCURACY_MAX_LEN];
  out.assign(buf, value_accuracy_to_buf(buf, e->state, e->get_accuracy_decimals()));
  return ReadResult::OK;
}
#endif
#ifdef USE_NUMBER
inline ReadResult serial_read_state(number::Number *e, std::string &out) {
  out = protocol::format_state_float(e->state, e->has_state());
  return ReadResult::OK;
}
#endif
#ifdef USE_TEXT
inline ReadResult serial_read_state(text::Text *e, std::string &out) {
  out = e->state;
  return ReadResult::OK;
}
#endif
#ifdef USE_TEXT_SENSOR
inline ReadResult serial_read_state(text_sensor::TextSensor *e, std::string &out) {
  out = e->state;
  return ReadResult::OK;
}
#endif
#ifdef USE_SELECT
inline ReadResult serial_read_state(select::Select *e, std::string &out) {
  out = e->current_option().str();
  return ReadResult::OK;
}
#endif
#ifdef USE_BUTTON
inline ReadResult serial_read_state(button::Button * /*e*/, std::string & /*out*/) {
  return ReadResult::NO_STATE;
}
#endif
#ifdef SERIAL_API_USE_LIGHT
// The JSON document the MQTT state topic carries.
inline ReadResult serial_read_state(light::LightState *e, std::string &out) {
  out = json::build_json([e](JsonObject root) { light::LightJSONSchema::dump_json(*e, root); });
  return ReadResult::OK;
}
#endif

template<class T> inline bool serial_is_writable(T * /*e*/) { return false; }
#ifdef USE_SWITCH
inline bool serial_is_writable(switch_::Switch * /*e*/) { return true; }
#endif
#ifdef USE_NUMBER
inline bool serial_is_writable(number::Number * /*e*/) { return true; }
#endif
#ifdef USE_TEXT
inline bool serial_is_writable(text::Text * /*e*/) { return true; }
#endif
#ifdef USE_SELECT
inline bool serial_is_writable(select::Select * /*e*/) { return true; }
#endif
#ifdef USE_BUTTON
inline bool serial_is_writable(button::Button * /*e*/) { return true; }
#endif
#ifdef SERIAL_API_USE_LIGHT
inline bool serial_is_writable(light::LightState * /*e*/) { return true; }
#endif

template<class T> inline std::string serial_domain(T * /*e*/) { return std::string(); }
#ifdef USE_NUMBER
inline std::string serial_domain(number::Number *e) {
  return protocol::format_number_domain(e->traits.get_min_value(), e->traits.get_max_value(),
                                        e->traits.get_step());
}
#endif
#ifdef USE_TEXT
inline std::string serial_domain(text::Text *e) {
  return protocol::format_text_domain(e->traits.get_min_length(), e->traits.get_max_length());
}
#endif
#ifdef USE_SELECT
inline std::string serial_domain(select::Select *e) {
  std::vector<std::string> options;
  for (const char *option : e->traits.get_options())
    options.emplace_back(option);
  return protocol::format_select_domain(options);
}
#endif
#ifdef SERIAL_API_USE_LIGHT
// The effect names, "None" last, as MQTT discovery lists them.
inline std::string serial_domain(light::LightState *e) {
  if (!e->supports_effects())
    return std::string();
  std::vector<std::string> effects;
  for (auto *effect : e->get_effects())
    effects.emplace_back(effect->get_name().str());
  effects.emplace_back("None");
  return protocol::format_select_domain(effects);
}
#endif

// Writes go through each entity's public call API, so a serial write publishes exactly as an MQTT
// command does. Values are validated first: see protocol::ValueCheck.
template<class T> inline protocol::SetOutcome serial_write(T * /*e*/, const std::string & /*value*/) {
  return protocol::SetOutcome::UNSUPPORTED;
}

#ifdef USE_SWITCH
inline protocol::SetOutcome serial_write(switch_::Switch *e, const std::string &value) {
  protocol::SwitchCommand command;
  if (protocol::check_switch_value(value, command) != protocol::ValueCheck::OK)
    return protocol::SetOutcome::BAD_VALUE;
  switch (command) {
    case protocol::SwitchCommand::ON:
      e->turn_on();
      break;
    case protocol::SwitchCommand::OFF:
      e->turn_off();
      break;
    case protocol::SwitchCommand::TOGGLE:
      e->toggle();
      break;
  }
  return protocol::SetOutcome::OK;
}
#endif
#ifdef USE_NUMBER
inline protocol::SetOutcome serial_write(number::Number *e, const std::string &value) {
  float parsed;
  if (protocol::check_number_value(value, e->traits.get_min_value(), e->traits.get_max_value(), parsed) !=
      protocol::ValueCheck::OK)
    return protocol::SetOutcome::BAD_VALUE;
  e->make_call().set_value(parsed).perform();
  return protocol::SetOutcome::OK;
}
#endif
#ifdef USE_TEXT
inline protocol::SetOutcome serial_write(text::Text *e, const std::string &value) {
  if (protocol::check_text_value(value, e->traits.get_min_length(), e->traits.get_max_length()) !=
      protocol::ValueCheck::OK)
    return protocol::SetOutcome::BAD_VALUE;
  e->make_call().set_value(value).perform();
  return protocol::SetOutcome::OK;
}
#endif
#ifdef USE_SELECT
inline protocol::SetOutcome serial_write(select::Select *e, const std::string &value) {
  std::vector<std::string> options;
  for (const char *option : e->traits.get_options())
    options.emplace_back(option);
  if (protocol::check_select_value(value, options) != protocol::ValueCheck::OK)
    return protocol::SetOutcome::BAD_VALUE;
  e->make_call().set_option(value).perform();
  return protocol::SetOutcome::OK;
}
#endif
#ifdef USE_BUTTON
inline protocol::SetOutcome serial_write(button::Button *e, const std::string &value) {
  if (protocol::check_button_value(value) != protocol::ValueCheck::OK)
    return protocol::SetOutcome::BAD_VALUE;
  e->press();
  return protocol::SetOutcome::OK;
}
#endif
#ifdef SERIAL_API_USE_LIGHT
inline protocol::SetOutcome serial_write(light::LightState *e, const std::string &value) {
  protocol::LightCommand command;
  if (protocol::check_light_value(value, command) != protocol::ValueCheck::OK)
    return protocol::SetOutcome::BAD_VALUE;
  switch (command) {
    case protocol::LightCommand::ON:
      e->turn_on().perform();
      break;
    case protocol::LightCommand::OFF:
      e->turn_off().perform();
      break;
    case protocol::LightCommand::TOGGLE:
      e->toggle().perform();
      break;
    case protocol::LightCommand::JSON: {
      bool parsed = json::parse_json(value, [e](JsonObject root) -> bool {
        if (root.isNull())
          return false;
        auto call = e->make_call();
        light::LightJSONSchema::parse_json(*e, call, root);
        call.perform();
        return true;
      });
      if (!parsed)
        return protocol::SetOutcome::BAD_VALUE;
      break;
    }
  }
  return protocol::SetOutcome::OK;
}
#endif
#ifdef USE_SENSOR
inline protocol::SetOutcome serial_write(sensor::Sensor * /*e*/, const std::string & /*value*/) {
  return protocol::SetOutcome::READ_ONLY;
}
#endif
#ifdef USE_BINARY_SENSOR
inline protocol::SetOutcome serial_write(binary_sensor::BinarySensor * /*e*/, const std::string & /*value*/) {
  return protocol::SetOutcome::READ_ONLY;
}
#endif
#ifdef USE_TEXT_SENSOR
inline protocol::SetOutcome serial_write(text_sensor::TextSensor * /*e*/, const std::string & /*value*/) {
  return protocol::SetOutcome::READ_ONLY;
}
#endif

#ifdef SERIAL_API_CONTROLLER_BASE
#define SERIAL_API_CONTROLLER_PARENT , public Controller
#define SERIAL_API_CONTROLLER_OVERRIDE override
#else
#define SERIAL_API_CONTROLLER_PARENT
#define SERIAL_API_CONTROLLER_OVERRIDE
#endif

class SerialAPI : public uart::UARTDevice, public Component SERIAL_API_CONTROLLER_PARENT {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::LATE; }

  // Reported by HELLO; when unset, the project or ESPHome version is reported instead.
  void set_firmware_version(const std::string &version) { this->firmware_version_ = version; }

  // One override per controller-capable entity type. Buttons have none, so they never produce EVT.
  // NOLINTBEGIN(bugprone-macro-parentheses)
#define ENTITY_TYPE_(type, singular, plural, count, upper)
#define ENTITY_CONTROLLER_TYPE_(type, singular, plural, count, upper, callback) \
  void on_##callback(type *obj) SERIAL_API_CONTROLLER_OVERRIDE { this->on_entity_update_(obj, #singular); }
#include "esphome/core/entity_types.h"
#undef ENTITY_TYPE_
#undef ENTITY_CONTROLLER_TYPE_
  // NOLINTEND(bugprone-macro-parentheses)

 protected:
  void process_line_(const std::string &line);
  void handle_list_();
  void handle_get_(const protocol::Command &cmd);
  void handle_get_all_();
  void handle_set_(const protocol::Command &cmd);
  void send_hello_();
  void emit_line_(const std::string &line);
  std::string object_id_of_(EntityBase *entity);
  static std::string default_firmware_version_();

  static bool is_mqtt_status_(const protocol::Command &cmd);
  std::string mqtt_status_address_() const;
  void poll_mqtt_status_();

  template<class T> void list_entity_(T *entity, const char *type_name) {
    std::string object_id = this->object_id_of_(entity);
    protocol::Exposure exposure = protocol::exposure_of(object_id, entity->is_internal());
    if (exposure == protocol::Exposure::HIDDEN)
      return;
    bool writable = exposure == protocol::Exposure::FULL && serial_is_writable(entity);
    this->emit_line_(
        protocol::format_ent(type_name, object_id, writable, entity->get_name().str(), serial_domain(entity)));
  }

  template<class T> void state_entity_(T *entity, const char *type_name) {
    std::string object_id = this->object_id_of_(entity);
    if (protocol::exposure_of(object_id, entity->is_internal()) == protocol::Exposure::HIDDEN)
      return;
    std::string value;
    if (serial_read_state(entity, value) != ReadResult::OK)
      return;
    this->emit_line_(protocol::format_state(std::string(type_name) + "/" + object_id, value));
  }

  template<class T> void respond_get_(T *entity, const char *type_name) {
    std::string object_id = this->object_id_of_(entity);
    std::string address = std::string(type_name) + "/" + object_id;
    if (protocol::exposure_of(object_id, entity->is_internal()) == protocol::Exposure::HIDDEN) {
      this->emit_line_(protocol::format_err(protocol::err::DENIED, address));
      return;
    }
    std::string value;
    switch (serial_read_state(entity, value)) {
      case ReadResult::OK:
        this->emit_line_(protocol::format_state(address, value));
        break;
      case ReadResult::NO_STATE:
        this->emit_line_(protocol::format_err(protocol::err::NO_STATE, address));
        break;
      case ReadResult::UNSUPPORTED:
        this->emit_line_(protocol::format_err(protocol::err::UNSUPPORTED_TYPE, address));
        break;
    }
  }

  template<class T> void respond_set_(T *entity, const char *type_name, const std::string &value) {
    std::string object_id = this->object_id_of_(entity);
    std::string address = std::string(type_name) + "/" + object_id;
    if (protocol::exposure_of(object_id, entity->is_internal()) != protocol::Exposure::FULL) {
      this->emit_line_(protocol::format_err(protocol::err::DENIED, address));
      return;
    }
    this->emit_line_(protocol::format_set_response(serial_write(entity, value), address));
  }

  template<class T> void on_entity_update_(T *entity, const char *type_name) {
    // ESPHome 2026.10 dispatches state updates from boot, before setup() has sent HELLO.
    if (!this->started_ || !this->subscribed_)
      return;
    std::string object_id = this->object_id_of_(entity);
    if (protocol::exposure_of(object_id, entity->is_internal()) == protocol::Exposure::HIDDEN)
      return;
    std::string value;
    if (serial_read_state(entity, value) != ReadResult::OK)
      return;
    this->emit_line_(protocol::format_evt(std::string(type_name) + "/" + object_id, value));
  }

  std::string firmware_version_;
  protocol::LineAssembler assembler_;
  bool started_{false};
  bool subscribed_{true};
  bool mqtt_connected_{false};
};

}  // namespace serial_api
}  // namespace esphome

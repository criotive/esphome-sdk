#include "serial_api.h"

#include "esphome/core/log.h"

namespace esphome {
namespace serial_api {

static const char *const TAG = "serial_api";

void SerialAPI::setup() {
#ifdef USE_CONTROLLER_REGISTRY
  ControllerRegistry::register_controller(this);
#endif
  this->send_hello_();
}

void SerialAPI::loop() {
  this->poll_mqtt_status_();
  uint8_t byte;
  while (this->available()) {
    if (!this->read_byte(&byte))
      break;
    switch (this->assembler_.feed(byte)) {
      case protocol::LineAssembler::Status::LINE_READY:
        this->process_line_(this->assembler_.line());
        this->assembler_.reset();
        break;
      case protocol::LineAssembler::Status::TOO_LONG:
        this->emit_line_(protocol::format_err(protocol::err::PARSE, "line-too-long"));
        break;
      case protocol::LineAssembler::Status::NONE:
        break;
    }
  }
}

void SerialAPI::dump_config() {
  ESP_LOGCONFIG(TAG, "Serial API:");
  ESP_LOGCONFIG(TAG, "  Protocol version: %d", protocol::PROTOCOL_VERSION);
}

void SerialAPI::process_line_(const std::string &line) {
  protocol::Command cmd = protocol::parse_line(line);
  if (!cmd.ok) {
    this->emit_line_(
        protocol::format_err(cmd.error_code, cmd.error_detail ? std::string(cmd.error_detail) : std::string()));
    return;
  }

  switch (cmd.verb) {
    case protocol::Verb::LIST:
      this->handle_list_();
      break;
    case protocol::Verb::GET:
      if (cmd.wildcard)
        this->handle_get_all_();
      else
        this->handle_get_(cmd);
      break;
    case protocol::Verb::SUB:
      this->subscribed_ = true;
      this->emit_line_("OK");
      break;
    case protocol::Verb::UNSUB:
      this->subscribed_ = false;
      this->emit_line_("OK");
      break;
    case protocol::Verb::PING:
      this->emit_line_("PONG");
      break;
    case protocol::Verb::HELLO:
      this->send_hello_();
      break;
    case protocol::Verb::SET:
      this->handle_set_(cmd);
      break;
    case protocol::Verb::UNKNOWN:
      this->emit_line_(protocol::format_err(protocol::err::PARSE));
      break;
  }
}

void SerialAPI::handle_list_() {
  // NOLINTBEGIN(bugprone-macro-parentheses)
#define ENTITY_TYPE_(type, singular, plural, count, upper) \
  for (auto *entity : App.get_##plural()) \
    this->list_entity_(entity, #singular);
#define ENTITY_CONTROLLER_TYPE_(type, singular, plural, count, upper, callback) \
  ENTITY_TYPE_(type, singular, plural, count, upper)
#include "esphome/core/entity_types.h"
#undef ENTITY_TYPE_
#undef ENTITY_CONTROLLER_TYPE_
  // NOLINTEND(bugprone-macro-parentheses)
#ifdef USE_MQTT
  this->emit_line_(protocol::format_ent(protocol::MQTT_STATUS_TYPE, protocol::MQTT_STATUS_OBJECT_ID, false,
                                        protocol::MQTT_STATUS_NAME, ""));
#endif
  this->emit_line_("END");
}

void SerialAPI::handle_get_all_() {
  // NOLINTBEGIN(bugprone-macro-parentheses)
#define ENTITY_TYPE_(type, singular, plural, count, upper) \
  for (auto *entity : App.get_##plural()) \
    this->state_entity_(entity, #singular);
#define ENTITY_CONTROLLER_TYPE_(type, singular, plural, count, upper, callback) \
  ENTITY_TYPE_(type, singular, plural, count, upper)
#include "esphome/core/entity_types.h"
#undef ENTITY_TYPE_
#undef ENTITY_CONTROLLER_TYPE_
  // NOLINTEND(bugprone-macro-parentheses)
#ifdef USE_MQTT
  this->emit_line_(
      protocol::format_state(this->mqtt_status_address_(), protocol::mqtt_status_value(this->mqtt_connected_)));
#endif
  this->emit_line_("END");
}

void SerialAPI::handle_get_(const protocol::Command &cmd) {
#ifdef USE_MQTT
  if (SerialAPI::is_mqtt_status_(cmd)) {
    this->emit_line_(
        protocol::format_state(this->mqtt_status_address_(), protocol::mqtt_status_value(this->mqtt_connected_)));
    return;
  }
#endif
  // `type` is an X-macro parameter, so `cmd.type` inside the macro body would be substituted.
  const std::string &req_type = cmd.type;
  const std::string &req_object_id = cmd.object_id;
  // NOLINTBEGIN(bugprone-macro-parentheses)
#define ENTITY_TYPE_(type, singular, plural, count, upper) \
  if (req_type == #singular) { \
    for (auto *entity : App.get_##plural()) { \
      if (this->object_id_of_(entity) == req_object_id) { \
        this->respond_get_(entity, #singular); \
        return; \
      } \
    } \
  }
#define ENTITY_CONTROLLER_TYPE_(type, singular, plural, count, upper, callback) \
  ENTITY_TYPE_(type, singular, plural, count, upper)
#include "esphome/core/entity_types.h"
#undef ENTITY_TYPE_
#undef ENTITY_CONTROLLER_TYPE_
  // NOLINTEND(bugprone-macro-parentheses)
  this->emit_line_(protocol::format_err(protocol::err::UNKNOWN_ENTITY, cmd.type + "/" + cmd.object_id));
}

bool SerialAPI::is_mqtt_status_(const protocol::Command &cmd) {
  return cmd.type == protocol::MQTT_STATUS_TYPE && cmd.object_id == protocol::MQTT_STATUS_OBJECT_ID;
}

std::string SerialAPI::mqtt_status_address_() const {
  return std::string(protocol::MQTT_STATUS_TYPE) + "/" + protocol::MQTT_STATUS_OBJECT_ID;
}

void SerialAPI::poll_mqtt_status_() {
#ifdef USE_MQTT
  bool connected = mqtt::global_mqtt_client != nullptr && mqtt::global_mqtt_client->is_connected();
  if (connected == this->mqtt_connected_)
    return;
  this->mqtt_connected_ = connected;
  if (this->subscribed_)
    this->emit_line_(protocol::format_evt(this->mqtt_status_address_(), protocol::mqtt_status_value(connected)));
#endif
}

void SerialAPI::handle_set_(const protocol::Command &cmd) {
#ifdef USE_MQTT
  if (SerialAPI::is_mqtt_status_(cmd)) {
    this->emit_line_(protocol::format_set_response(protocol::SetOutcome::READ_ONLY, this->mqtt_status_address_()));
    return;
  }
#endif
  // `type` is an X-macro parameter, so `cmd.type` inside the macro body would be substituted.
  const std::string &req_type = cmd.type;
  const std::string &req_object_id = cmd.object_id;
  const std::string &value = cmd.value;
  // NOLINTBEGIN(bugprone-macro-parentheses)
#define ENTITY_TYPE_(type, singular, plural, count, upper) \
  if (req_type == #singular) { \
    for (auto *entity : App.get_##plural()) { \
      if (this->object_id_of_(entity) == req_object_id) { \
        this->respond_set_(entity, #singular, value); \
        return; \
      } \
    } \
  }
#define ENTITY_CONTROLLER_TYPE_(type, singular, plural, count, upper, callback) \
  ENTITY_TYPE_(type, singular, plural, count, upper)
#include "esphome/core/entity_types.h"
#undef ENTITY_TYPE_
#undef ENTITY_CONTROLLER_TYPE_
  // NOLINTEND(bugprone-macro-parentheses)
  this->emit_line_(protocol::format_err(protocol::err::UNKNOWN_ENTITY, cmd.type + "/" + cmd.object_id));
}

void SerialAPI::send_hello_() {
  std::string version =
      this->firmware_version_.empty() ? SerialAPI::default_firmware_version_() : this->firmware_version_;
  this->emit_line_(protocol::format_hello(App.get_name().str(), version));
}

void SerialAPI::emit_line_(const std::string &line) {
  this->write_array(reinterpret_cast<const uint8_t *>(line.data()), line.size());
  this->write_byte('\n');
}

std::string SerialAPI::object_id_of_(EntityBase *entity) {
  char buffer[OBJECT_ID_MAX_LEN];
  return entity->get_object_id_to(std::span<char, OBJECT_ID_MAX_LEN>(buffer, OBJECT_ID_MAX_LEN)).str();
}

std::string SerialAPI::default_firmware_version_() {
#ifdef ESPHOME_PROJECT_VERSION
  return ESPHOME_PROJECT_VERSION;
#else
  return ESPHOME_VERSION;
#endif
}

}  // namespace serial_api
}  // namespace esphome

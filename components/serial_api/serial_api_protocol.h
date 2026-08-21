#pragma once

// Serial API wire protocol: lexer, formatters and exposure rule. Standard library only, so the host
// unit tests (tests/unit/) build it with plain g++; an esphome/ include here breaks them.

#include <cstdint>
#include <string>
#include <vector>

namespace esphome {
namespace serial_api {
namespace protocol {

// Reported by HELLO. Bump only on an incompatible grammar change.
static constexpr int PROTOCOL_VERSION = 1;

// Inbound line budget, terminator included.
static constexpr size_t MAX_LINE_BYTES = 256;

// The closed error-code set. A client branches on the code, never on the detail.
namespace err {
static constexpr const char *PARSE = "parse";
static constexpr const char *UNKNOWN_ENTITY = "unknown-entity";
static constexpr const char *UNSUPPORTED_TYPE = "unsupported-type";
static constexpr const char *READ_ONLY = "read-only";
static constexpr const char *BAD_VALUE = "bad-value";
static constexpr const char *DENIED = "denied";
static constexpr const char *NO_STATE = "no-state";
}  // namespace err

enum class Verb : uint8_t { LIST, GET, SET, SUB, UNSUB, PING, HELLO, UNKNOWN };

// The result of lexing one assembled line (terminator and an optional trailing CR already removed).
struct Command {
  Verb verb{Verb::UNKNOWN};
  bool ok{false};              // false => reject with `error_code`
  const char *error_code{nullptr};
  const char *error_detail{nullptr};
  bool wildcard{false};        // GET *
  std::string type;            // address type segment (before '/')
  std::string object_id;       // address object_id segment (after '/')
  bool has_value{false};       // SET carries a value (possibly empty)
  std::string value;           // raw SET value, verbatim to end of line
};

// Splits bytes into lines on '\n', dropping one '\r' before it. A line reaching MAX_LINE_BYTES is
// reported TOO_LONG once and discarded up to the next '\n', so the stream resynchronises.
class LineAssembler {
 public:
  enum class Status : uint8_t { NONE, LINE_READY, TOO_LONG };

  Status feed(uint8_t byte);
  const std::string &line() const { return this->line_; }
  void reset();

 protected:
  std::string line_;
  bool overflow_{false};
};

Command parse_line(const std::string &line);

// Escapes only '\' and '"', the grammar's two escapes.
std::string quote(const std::string &s);

bool has_control_byte(const std::string &s);

// Rejects overlong encodings, surrogate halves and code points beyond U+10FFFF.
bool is_valid_utf8(const std::string &s);

// HIDDEN: absent from LIST; GET and SET answer ERR denied. READ_ONLY: listed `ro`; SET answers
// ERR denied. FULL: whatever the entity type allows.
enum class Exposure : uint8_t { HIDDEN, READ_ONLY, FULL };

// Internal entities and the secret_ota credential are hidden. ota_* entities can be read but never
// written, so a serial client cannot start a firmware update. Everything else, system_* included,
// matches what MQTT exposes.
Exposure exposure_of(const std::string &object_id, bool internal);

// The broker connection, offered as the read-only pseudo-entity mqtt/status with the payloads the
// MQTT status topic uses.
static constexpr const char *MQTT_STATUS_TYPE = "mqtt";
static constexpr const char *MQTT_STATUS_OBJECT_ID = "status";
static constexpr const char *MQTT_STATUS_NAME = "MQTT status";
const char *mqtt_status_value(bool connected);

// Response-line formatters. Each returns the exact wire line WITHOUT the trailing '\n'; the caller
// terminates. They take primitives so the host tests can assert byte-exact output.
std::string format_err(const char *code, const std::string &detail);
std::string format_err(const char *code);
std::string format_hello(const std::string &device_name, const std::string &firmware_version);
std::string format_state(const std::string &address, const std::string &value);
std::string format_evt(const std::string &address, const std::string &value);

// ENT line. `domain` is the already-formatted, type-specific suffix (empty for types without one).
std::string format_ent(const std::string &type, const std::string &object_id, bool writable,
                       const std::string &name, const std::string &domain);

// Type-specific ENT domain suffixes.
std::string format_number_domain(float min_value, float max_value, float step);
std::string format_text_domain(int min_length, int max_length);
std::string format_select_domain(const std::vector<std::string> &options);

// Render a state float: "NaN" when unpublished, otherwise a compact decimal with no exponent and no
// trailing zeros. Shared by sensor and number so both serialise identically.
std::string format_state_float(float value, bool has_state);

}  // namespace protocol
}  // namespace serial_api
}  // namespace esphome

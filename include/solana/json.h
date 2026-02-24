#pragma once
/**
 * Lightweight JSON parser optimized for microcontrollers.
 * Zero-copy where possible, minimal heap allocations.
 * Supports: objects, arrays, strings, numbers, booleans, null.
 */

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace solana {
namespace json {

enum class Type { Null, Bool, Number, String, Array, Object };

/**
 * A lightweight JSON value. Uses a flat string view approach
 * to avoid deep copies. The source JSON string must outlive
 * all JsonValue instances referencing it.
 */
class JsonValue {
public:
  JsonValue() : _type(Type::Null), _src(nullptr), _start(0), _end(0) {}

  Type type() const { return _type; }
  bool isNull() const { return _type == Type::Null; }
  bool isBool() const { return _type == Type::Bool; }
  bool isNumber() const { return _type == Type::Number; }
  bool isString() const { return _type == Type::String; }
  bool isArray() const { return _type == Type::Array; }
  bool isObject() const { return _type == Type::Object; }

  // Value accessors
  bool asBool() const { return _boolVal; }
  int64_t asInt() const;
  uint64_t asUint() const;
  double asDouble() const;
  std::string asString() const;

  // Object access — O(n) scan, sufficient for small RPC responses
  JsonValue operator[](const char *key) const;
  JsonValue operator[](const std::string &key) const {
    return (*this)[key.c_str()];
  }
  bool hasKey(const char *key) const;

  // Array access
  JsonValue operator[](size_t index) const;
  size_t arraySize() const;

  // Get raw JSON text of this value
  std::string raw() const;

  // Parse a JSON string. Returns the root value.
  static JsonValue parse(const std::string &json);
  static JsonValue parse(const char *json, size_t len);

private:
  Type _type;
  const char *_src;
  size_t _start;
  size_t _end;
  bool _boolVal = false;

  // Internal parser
  static JsonValue parseValue(const char *src, size_t &pos, size_t len);
  static void skipWhitespace(const char *src, size_t &pos, size_t len);
  static std::string parseStringContent(const char *src, size_t &pos,
                                        size_t len);
  static size_t findMatchingBrace(const char *src, size_t start, size_t len,
                                  char open, char close);
};

} // namespace json
} // namespace solana

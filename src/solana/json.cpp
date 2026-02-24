#include "solana/json.h"
#include <cstdlib>
#include <stdexcept>

namespace solana {
namespace json {

void JsonValue::skipWhitespace(const char *src, size_t &pos, size_t len) {
  while (pos < len && (src[pos] == ' ' || src[pos] == '\t' ||
                       src[pos] == '\n' || src[pos] == '\r'))
    ++pos;
}

std::string JsonValue::parseStringContent(const char *src, size_t &pos,
                                          size_t len) {
  // pos should be right after opening "
  std::string result;
  result.reserve(32); // reasonable default for RPC keys/values
  while (pos < len && src[pos] != '"') {
    if (src[pos] == '\\' && pos + 1 < len) {
      ++pos;
      switch (src[pos]) {
      case '"':
        result += '"';
        break;
      case '\\':
        result += '\\';
        break;
      case '/':
        result += '/';
        break;
      case 'n':
        result += '\n';
        break;
      case 'r':
        result += '\r';
        break;
      case 't':
        result += '\t';
        break;
      case 'b':
        result += '\b';
        break;
      case 'f':
        result += '\f';
        break;
      default:
        result += src[pos];
        break;
      }
    } else {
      result += src[pos];
    }
    ++pos;
  }
  if (pos < len)
    ++pos; // skip closing "
  return result;
}

size_t JsonValue::findMatchingBrace(const char *src, size_t start, size_t len,
                                    char open, char close) {
  int depth = 1;
  size_t pos = start;
  bool inString = false;
  while (pos < len && depth > 0) {
    if (src[pos] == '"' && (pos == 0 || src[pos - 1] != '\\'))
      inString = !inString;
    if (!inString) {
      if (src[pos] == open)
        ++depth;
      else if (src[pos] == close)
        --depth;
    }
    if (depth > 0)
      ++pos;
  }
  return pos;
}

JsonValue JsonValue::parseValue(const char *src, size_t &pos, size_t len) {
  skipWhitespace(src, pos, len);
  if (pos >= len)
    return JsonValue();

  JsonValue val;
  val._src = src;
  val._start = pos;

  char c = src[pos];

  if (c == '"') {
    // String
    val._type = Type::String;
    val._start = pos;
    ++pos; // skip opening "
    // Find end of string (handle escapes)
    while (pos < len && src[pos] != '"') {
      if (src[pos] == '\\')
        ++pos; // skip escaped char
      ++pos;
    }
    if (pos < len)
      ++pos; // skip closing "
    val._end = pos;
  } else if (c == '{') {
    // Object
    val._type = Type::Object;
    val._start = pos;
    ++pos;
    val._end = findMatchingBrace(src, pos, len, '{', '}') + 1;
    pos = val._end;
  } else if (c == '[') {
    // Array
    val._type = Type::Array;
    val._start = pos;
    ++pos;
    val._end = findMatchingBrace(src, pos, len, '[', ']') + 1;
    pos = val._end;
  } else if (c == 't' && pos + 3 < len &&
             std::strncmp(src + pos, "true", 4) == 0) {
    val._type = Type::Bool;
    val._boolVal = true;
    val._end = pos + 4;
    pos += 4;
  } else if (c == 'f' && pos + 4 < len &&
             std::strncmp(src + pos, "false", 5) == 0) {
    val._type = Type::Bool;
    val._boolVal = false;
    val._end = pos + 5;
    pos += 5;
  } else if (c == 'n' && pos + 3 < len &&
             std::strncmp(src + pos, "null", 4) == 0) {
    val._type = Type::Null;
    val._end = pos + 4;
    pos += 4;
  } else if (c == '-' || (c >= '0' && c <= '9')) {
    // Number
    val._type = Type::Number;
    val._start = pos;
    if (c == '-')
      ++pos;
    while (pos < len && src[pos] >= '0' && src[pos] <= '9')
      ++pos;
    if (pos < len && src[pos] == '.') {
      ++pos;
      while (pos < len && src[pos] >= '0' && src[pos] <= '9')
        ++pos;
    }
    if (pos < len && (src[pos] == 'e' || src[pos] == 'E')) {
      ++pos;
      if (pos < len && (src[pos] == '+' || src[pos] == '-'))
        ++pos;
      while (pos < len && src[pos] >= '0' && src[pos] <= '9')
        ++pos;
    }
    val._end = pos;
  } else {
    // Unknown, return null
    val._type = Type::Null;
    val._end = pos;
  }

  return val;
}

JsonValue JsonValue::parse(const std::string &json) {
  return parse(json.c_str(), json.size());
}

JsonValue JsonValue::parse(const char *json, size_t len) {
  size_t pos = 0;
  return parseValue(json, pos, len);
}

int64_t JsonValue::asInt() const {
  if (_type != Type::Number || !_src)
    return 0;
  return std::strtoll(_src + _start, nullptr, 10);
}

uint64_t JsonValue::asUint() const {
  if (_type != Type::Number || !_src)
    return 0;
  return std::strtoull(_src + _start, nullptr, 10);
}

double JsonValue::asDouble() const {
  if (_type != Type::Number || !_src)
    return 0.0;
  return std::strtod(_src + _start, nullptr);
}

std::string JsonValue::asString() const {
  if (_type != Type::String || !_src)
    return "";
  // Skip quotes and parse escape sequences
  size_t pos = _start + 1; // skip opening "
  size_t end = _end - 1;   // before closing "
  std::string result;
  result.reserve(end - pos);
  while (pos < end) {
    if (_src[pos] == '\\' && pos + 1 < end) {
      ++pos;
      switch (_src[pos]) {
      case '"':
        result += '"';
        break;
      case '\\':
        result += '\\';
        break;
      case 'n':
        result += '\n';
        break;
      case 'r':
        result += '\r';
        break;
      case 't':
        result += '\t';
        break;
      default:
        result += _src[pos];
        break;
      }
    } else {
      result += _src[pos];
    }
    ++pos;
  }
  return result;
}

std::string JsonValue::raw() const {
  if (!_src || _end <= _start)
    return "";
  return std::string(_src + _start, _end - _start);
}

JsonValue JsonValue::operator[](const char *key) const {
  if (_type != Type::Object || !_src)
    return JsonValue();
  size_t pos = _start + 1; // skip {
  size_t end = _end - 1;   // before }
  size_t keyLen = std::strlen(key);

  while (pos < end) {
    skipWhitespace(_src, pos, end);
    if (pos >= end || _src[pos] != '"')
      break;
    // Parse key
    ++pos; // skip "
    size_t keyStart = pos;
    while (pos < end && _src[pos] != '"') {
      if (_src[pos] == '\\')
        ++pos;
      ++pos;
    }
    bool match = (pos - keyStart == keyLen) &&
                 std::strncmp(_src + keyStart, key, keyLen) == 0;
    if (pos < end)
      ++pos; // skip closing "

    skipWhitespace(_src, pos, end);
    if (pos < end && _src[pos] == ':')
      ++pos; // skip :

    // Parse value
    JsonValue val = parseValue(_src, pos, end);

    if (match)
      return val;

    // Skip comma
    skipWhitespace(_src, pos, end);
    if (pos < end && _src[pos] == ',')
      ++pos;
  }
  return JsonValue();
}

bool JsonValue::hasKey(const char *key) const {
  auto val = (*this)[key];
  return !val.isNull() || val._src != nullptr;
}

JsonValue JsonValue::operator[](size_t index) const {
  if (_type != Type::Array || !_src)
    return JsonValue();
  size_t pos = _start + 1; // skip [
  size_t end = _end - 1;   // before ]
  size_t i = 0;

  while (pos < end) {
    skipWhitespace(_src, pos, end);
    if (pos >= end)
      break;

    JsonValue val = parseValue(_src, pos, end);
    if (i == index)
      return val;
    ++i;

    skipWhitespace(_src, pos, end);
    if (pos < end && _src[pos] == ',')
      ++pos;
  }
  return JsonValue();
}

size_t JsonValue::arraySize() const {
  if (_type != Type::Array || !_src)
    return 0;
  size_t pos = _start + 1;
  size_t end = _end - 1;
  size_t count = 0;

  while (pos < end) {
    skipWhitespace(_src, pos, end);
    if (pos >= end)
      break;
    parseValue(_src, pos, end);
    ++count;
    skipWhitespace(_src, pos, end);
    if (pos < end && _src[pos] == ',')
      ++pos;
  }
  return count;
}

} // namespace json
} // namespace solana

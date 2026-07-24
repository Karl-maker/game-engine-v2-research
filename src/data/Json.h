#pragma once

// Author: Karl-Johan Bailey
//
// Json (minimal)
// - Small JSON value type + parser intended for data-driven config.
// - Supports: null, bool, number, string, array, object.

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace data {

struct JsonValue final {
  struct Null final {};
  using Array = std::vector<JsonValue>;
  using Object = std::unordered_map<std::string, JsonValue>;
  using Storage = std::variant<Null, bool, double, std::string, Array, Object>;

  Storage value{};

  JsonValue() : value(Null{}) {}
  JsonValue(Null) : value(Null{}) {}
  JsonValue(bool b) : value(b) {}
  JsonValue(double n) : value(n) {}
  JsonValue(std::string s) : value(std::move(s)) {}
  JsonValue(const char* s) : value(std::string(s ? s : "")) {}
  JsonValue(Array a) : value(std::move(a)) {}
  JsonValue(Object o) : value(std::move(o)) {}

  bool isNull() const { return std::holds_alternative<Null>(value); }
  bool isBool() const { return std::holds_alternative<bool>(value); }
  bool isNumber() const { return std::holds_alternative<double>(value); }
  bool isString() const { return std::holds_alternative<std::string>(value); }
  bool isArray() const { return std::holds_alternative<Array>(value); }
  bool isObject() const { return std::holds_alternative<Object>(value); }

  const bool* tryBool() const { return std::get_if<bool>(&value); }
  const double* tryNumber() const { return std::get_if<double>(&value); }
  const std::string* tryString() const { return std::get_if<std::string>(&value); }
  const Array* tryArray() const { return std::get_if<Array>(&value); }
  const Object* tryObject() const { return std::get_if<Object>(&value); }

  bool* tryBool() { return std::get_if<bool>(&value); }
  double* tryNumber() { return std::get_if<double>(&value); }
  std::string* tryString() { return std::get_if<std::string>(&value); }
  Array* tryArray() { return std::get_if<Array>(&value); }
  Object* tryObject() { return std::get_if<Object>(&value); }
};

struct JsonParseResult final {
  bool ok = false;
  JsonValue value{};
  std::string error;
  std::size_t errorOffset = 0;
};

JsonParseResult parseJson(std::string_view text);

const JsonValue* getObjectKey(const JsonValue::Object& obj, const char* key);

}  // namespace data


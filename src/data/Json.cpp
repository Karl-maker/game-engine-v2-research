#include "data/Json.h"

// Author: Karl-Johan Bailey

#include <cctype>
#include <cerrno>
#include <cstring>
#include <cstdlib>

namespace data {

namespace {

struct Parser final {
  std::string_view src;
  std::size_t i = 0;

  char peek() const { return (i < src.size()) ? src[i] : '\0'; }
  bool eof() const { return i >= src.size(); }

  void skipWs() {
    while (!eof()) {
      const unsigned char c = static_cast<unsigned char>(src[i]);
      if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
        i += 1;
      } else {
        break;
      }
    }
  }

  bool consume(char c) {
    if (peek() != c) return false;
    i += 1;
    return true;
  }

  JsonParseResult error(const char* msg) const {
    JsonParseResult r;
    r.ok = false;
    r.error = msg ? msg : "parse error";
    r.errorOffset = i;
    return r;
  }

  JsonParseResult parseValue() {
    skipWs();
    const char c = peek();
    if (c == '{') return parseObject();
    if (c == '[') return parseArray();
    if (c == '"') return parseString();
    if (c == '-' || (c >= '0' && c <= '9')) return parseNumber();
    if (c == 't') return parseLiteral("true", JsonValue(true));
    if (c == 'f') return parseLiteral("false", JsonValue(false));
    if (c == 'n') return parseLiteral("null", JsonValue(JsonValue::Null{}));
    return error("unexpected token");
  }

  JsonParseResult parseLiteral(const char* lit, JsonValue v) {
    const std::size_t len = std::strlen(lit);
    if (src.substr(i, len) != lit) return error("invalid literal");
    i += len;
    JsonParseResult r;
    r.ok = true;
    r.value = std::move(v);
    return r;
  }

  JsonParseResult parseString() {
    if (!consume('"')) return error("expected string");
    std::string out;
    while (!eof()) {
      const char c = peek();
      if (c == '"') {
        i += 1;
        JsonParseResult r;
        r.ok = true;
        r.value = JsonValue(std::move(out));
        return r;
      }
      if (c == '\\') {
        i += 1;
        if (eof()) return error("unterminated escape");
        const char e = peek();
        i += 1;
        switch (e) {
          case '"': out.push_back('"'); break;
          case '\\': out.push_back('\\'); break;
          case '/': out.push_back('/'); break;
          case 'b': out.push_back('\b'); break;
          case 'f': out.push_back('\f'); break;
          case 'n': out.push_back('\n'); break;
          case 'r': out.push_back('\r'); break;
          case 't': out.push_back('\t'); break;
          // Minimal parser: unicode escapes not supported (yet).
          default: return error("unsupported escape");
        }
        continue;
      }
      if (static_cast<unsigned char>(c) < 0x20) return error("invalid string character");
      out.push_back(c);
      i += 1;
    }
    return error("unterminated string");
  }

  JsonParseResult parseNumber() {
    const std::size_t start = i;
    if (consume('-')) {
    }
    if (consume('0')) {
      // ok
    } else {
      if (!std::isdigit(static_cast<unsigned char>(peek()))) return error("invalid number");
      while (std::isdigit(static_cast<unsigned char>(peek()))) i += 1;
    }

    if (consume('.')) {
      if (!std::isdigit(static_cast<unsigned char>(peek()))) return error("invalid number");
      while (std::isdigit(static_cast<unsigned char>(peek()))) i += 1;
    }

    if (peek() == 'e' || peek() == 'E') {
      i += 1;
      if (peek() == '+' || peek() == '-') i += 1;
      if (!std::isdigit(static_cast<unsigned char>(peek()))) return error("invalid number");
      while (std::isdigit(static_cast<unsigned char>(peek()))) i += 1;
    }

    const std::string_view sv = src.substr(start, i - start);
    errno = 0;
    char* end = nullptr;
    const std::string tmp(sv);
    const double v = std::strtod(tmp.c_str(), &end);
    if (errno != 0 || end == tmp.c_str()) return error("invalid number");

    JsonParseResult r;
    r.ok = true;
    r.value = JsonValue(v);
    return r;
  }

  JsonParseResult parseArray() {
    if (!consume('[')) return error("expected '['");
    JsonValue::Array arr;
    skipWs();
    if (consume(']')) {
      JsonParseResult r;
      r.ok = true;
      r.value = JsonValue(std::move(arr));
      return r;
    }
    while (true) {
      auto v = parseValue();
      if (!v.ok) return v;
      arr.push_back(std::move(v.value));
      skipWs();
      if (consume(']')) break;
      if (!consume(',')) return error("expected ',' or ']'");
    }
    JsonParseResult r;
    r.ok = true;
    r.value = JsonValue(std::move(arr));
    return r;
  }

  JsonParseResult parseObject() {
    if (!consume('{')) return error("expected '{'");
    JsonValue::Object obj;
    skipWs();
    if (consume('}')) {
      JsonParseResult r;
      r.ok = true;
      r.value = JsonValue(std::move(obj));
      return r;
    }
    while (true) {
      skipWs();
      auto k = parseString();
      if (!k.ok) return k;
      const auto* ks = k.value.tryString();
      if (!ks) return error("invalid object key");
      skipWs();
      if (!consume(':')) return error("expected ':'");
      auto v = parseValue();
      if (!v.ok) return v;
      obj[*ks] = std::move(v.value);
      skipWs();
      if (consume('}')) break;
      if (!consume(',')) return error("expected ',' or '}'");
    }
    JsonParseResult r;
    r.ok = true;
    r.value = JsonValue(std::move(obj));
    return r;
  }
};

}  // namespace

JsonParseResult parseJson(std::string_view text) {
  Parser p;
  p.src = text;
  p.i = 0;
  auto r = p.parseValue();
  if (!r.ok) return r;
  p.skipWs();
  if (!p.eof()) return p.error("trailing characters");
  r.ok = true;
  return r;
}

const JsonValue* getObjectKey(const JsonValue::Object& obj, const char* key) {
  if (!key) return nullptr;
  auto it = obj.find(key);
  if (it == obj.end()) return nullptr;
  return &it->second;
}

}  // namespace data

#include "helpers.h"

#include <cstdio>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <cstdarg>

#if defined(USE_ESP8266)
#include <osapi.h>
#include <user_interface.h>
// for xt_rsil()/xt_wsr_ps()
#include <Arduino.h>
#elif defined(USE_ESP32_FRAMEWORK_ARDUINO)
#include <Esp.h>
#elif defined(USE_ESP_IDF)
#include "esp_system.h"
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
#elif defined(USE_RP2040)
#if defined(USE_WIFI)
#include <WiFi.h>
#endif
#include <hardware/structs/rosc.h>
#include <hardware/sync.h>
#endif

#ifdef USE_ESP32_IGNORE_EFUSE_MAC_CRC
#include "esp_efuse.h"
#include "esp_efuse_table.h"
#endif

namespace esphome {

// STL backports

#if _GLIBCXX_RELEASE < 7
std::string to_string(int value) { return str_snprintf("%d", 32, value); }                   // NOLINT
std::string to_string(long value) { return str_snprintf("%ld", 32, value); }                 // NOLINT
std::string to_string(long long value) { return str_snprintf("%lld", 32, value); }           // NOLINT
std::string to_string(unsigned value) { return str_snprintf("%u", 32, value); }              // NOLINT
std::string to_string(unsigned long value) { return str_snprintf("%lu", 32, value); }        // NOLINT
std::string to_string(unsigned long long value) { return str_snprintf("%llu", 32, value); }  // NOLINT
std::string to_string(float value) { return str_snprintf("%f", 32, value); }
std::string to_string(double value) { return str_snprintf("%f", 32, value); }
std::string to_string(long double value) { return str_snprintf("%Lf", 32, value); }
#endif

// Strings

bool str_equals_case_insensitive(const std::string &a, const std::string &b) {
  return strcasecmp(a.c_str(), b.c_str()) == 0;
}
bool str_startswith(const std::string &str, const std::string &start) { return str.rfind(start, 0) == 0; }
bool str_endswith(const std::string &str, const std::string &end) {
  return str.rfind(end) == (str.size() - end.size());
}
std::string str_truncate(const std::string &str, size_t length) {
  return str.length() > length ? str.substr(0, length) : str;
}
std::string str_until(const char *str, char ch) {
  char *pos = strchr(str, ch);
  return pos == nullptr ? std::string(str) : std::string(str, pos - str);
}
std::string str_until(const std::string &str, char ch) { return str.substr(0, str.find(ch)); }
// wrapper around std::transform to run safely on functions from the ctype.h header
// see https://en.cppreference.com/w/cpp/string/byte/toupper#Notes
template<int (*fn)(int)> std::string str_ctype_transform(const std::string &str) {
  std::string result;
  result.resize(str.length());
  std::transform(str.begin(), str.end(), result.begin(), [](unsigned char ch) { return fn(ch); });
  return result;
}
std::string str_lower_case(const std::string &str) { return str_ctype_transform<std::tolower>(str); }
std::string str_upper_case(const std::string &str) { return str_ctype_transform<std::toupper>(str); }
std::string str_snake_case(const std::string &str) {
  std::string result;
  result.resize(str.length());
  std::transform(str.begin(), str.end(), result.begin(), ::tolower);
  std::replace(result.begin(), result.end(), ' ', '_');
  return result;
}
std::string str_sanitize(const std::string &str) {
  std::string out;
  std::copy_if(str.begin(), str.end(), std::back_inserter(out), [](const char &c) {
    return c == '-' || c == '_' || (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
  });
  return out;
}
std::string str_snprintf(const char *fmt, size_t len, ...) {
  std::string str;
  va_list args;

  str.resize(len);
  va_start(args, len);
  size_t out_length = vsnprintf(&str[0], len + 1, fmt, args);
  va_end(args);

  if (out_length < len)
    str.resize(out_length);

  return str;
}
std::string str_sprintf(const char *fmt, ...) {
  std::string str;
  va_list args;

  va_start(args, fmt);
  size_t length = vsnprintf(nullptr, 0, fmt, args);
  va_end(args);

  str.resize(length);
  va_start(args, fmt);
  vsnprintf(&str[0], length + 1, fmt, args);
  va_end(args);

  return str;
}

}  // namespace esphome

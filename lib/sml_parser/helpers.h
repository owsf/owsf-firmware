#pragma once

#include <cmath>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#ifdef USE_ESP32
#include <esp_heap_caps.h>
#endif

#define HOT __attribute__((hot))
#define ESPDEPRECATED(msg, when) __attribute__((deprecated(msg)))
#define ALWAYS_INLINE __attribute__((always_inline))
#define PACKED __attribute__((packed))

// Various functions can be constexpr in C++14, but not in C++11 (because their body isn't just a return statement).
// Define a substitute constexpr keyword for those functions, until we can drop C++11 support.
#if __cplusplus >= 201402L
#define constexpr14 constexpr
#else
#define constexpr14 inline  // constexpr implies inline
#endif

namespace esphome {

/// @name STL backports
///@{

// Backports for various STL features we like to use. Pull in the STL implementation wherever available, to avoid
// ambiguity and to provide a uniform API.

// std::to_string() from C++11, available from libstdc++/g++ 8
// See https://github.com/espressif/esp-idf/issues/1445
#if _GLIBCXX_RELEASE >= 8
using std::to_string;
#else
std::string to_string(int value);                 // NOLINT
std::string to_string(long value);                // NOLINT
std::string to_string(long long value);           // NOLINT
std::string to_string(unsigned value);            // NOLINT
std::string to_string(unsigned long value);       // NOLINT
std::string to_string(unsigned long long value);  // NOLINT
std::string to_string(float value);
std::string to_string(double value);
std::string to_string(long double value);
#endif

// std::is_trivially_copyable from C++11, implemented in libstdc++/g++ 5.1 (but minor releases can't be detected)
#if _GLIBCXX_RELEASE >= 6
using std::is_trivially_copyable;
#else
// Implementing this is impossible without compiler intrinsics, so don't bother. Invalid usage will be detected on
// other variants that use a newer compiler anyway.
// NOLINTNEXTLINE(readability-identifier-naming)
template<typename T> struct is_trivially_copyable : public std::integral_constant<bool, true> {};
#endif

// std::make_unique() from C++14
#if __cpp_lib_make_unique >= 201304
using std::make_unique;
#else
template<typename T, typename... Args> std::unique_ptr<T> make_unique(Args &&...args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
#endif

// std::enable_if_t from C++14
#if __cplusplus >= 201402L
using std::enable_if_t;
#else
template<bool B, class T = void> using enable_if_t = typename std::enable_if<B, T>::type;
#endif

// std::clamp from C++17
#if __cpp_lib_clamp >= 201603
using std::clamp;
#else
template<typename T, typename Compare> constexpr const T &clamp(const T &v, const T &lo, const T &hi, Compare comp) {
  return comp(v, lo) ? lo : comp(hi, v) ? hi : v;
}
template<typename T> constexpr const T &clamp(const T &v, const T &lo, const T &hi) {
  return clamp(v, lo, hi, std::less<T>{});
}
#endif

// std::is_invocable from C++17
#if __cpp_lib_is_invocable >= 201703
using std::is_invocable;
#else
// https://stackoverflow.com/a/37161919/8924614
template<class T, class... Args> struct is_invocable {  // NOLINT(readability-identifier-naming)
  template<class U> static auto test(U *p) -> decltype((*p)(std::declval<Args>()...), void(), std::true_type());
  template<class U> static auto test(...) -> decltype(std::false_type());
  static constexpr auto value = decltype(test<T>(nullptr))::value;  // NOLINT
};
#endif

// std::bit_cast from C++20
#if __cpp_lib_bit_cast >= 201806
using std::bit_cast;
#else
/// Convert data between types, without aliasing issues or undefined behaviour.
template<
    typename To, typename From,
    enable_if_t<sizeof(To) == sizeof(From) && is_trivially_copyable<From>::value && is_trivially_copyable<To>::value,
                int> = 0>
To bit_cast(const From &src) {
  To dst;
  memcpy(&dst, &src, sizeof(To));
  return dst;
}
#endif

// std::byteswap from C++23
template<typename T> constexpr14 T byteswap(T n) {
  T m;
  for (size_t i = 0; i < sizeof(T); i++)
    reinterpret_cast<uint8_t *>(&m)[i] = reinterpret_cast<uint8_t *>(&n)[sizeof(T) - 1 - i];
  return m;
}
template<> constexpr14 uint8_t byteswap(uint8_t n) { return n; }
template<> constexpr14 uint16_t byteswap(uint16_t n) { return __builtin_bswap16(n); }
template<> constexpr14 uint32_t byteswap(uint32_t n) { return __builtin_bswap32(n); }
template<> constexpr14 uint64_t byteswap(uint64_t n) { return __builtin_bswap64(n); }
template<> constexpr14 int8_t byteswap(int8_t n) { return n; }
template<> constexpr14 int16_t byteswap(int16_t n) { return __builtin_bswap16(n); }
template<> constexpr14 int32_t byteswap(int32_t n) { return __builtin_bswap32(n); }
template<> constexpr14 int64_t byteswap(int64_t n) { return __builtin_bswap64(n); }

///@}

/// @name Mathematics
///@{

/// Linearly interpolate between \p start and \p end by \p completion (between 0 and 1).
float lerp(float completion, float start, float end);

/// Remap \p value from the range (\p min, \p max) to (\p min_out, \p max_out).
template<typename T, typename U> T remap(U value, U min, U max, T min_out, T max_out) {
  return (value - min) * (max_out - min_out) / (max - min) + min_out;
}

/// Calculate a CRC-8 checksum of \p data with size \p len.
uint8_t crc8(uint8_t *data, uint8_t len);

/// Calculate a CRC-16 checksum of \p data with size \p len.
uint16_t crc16(const uint8_t *data, uint8_t len);

/// Calculate a FNV-1 hash of \p str.
uint32_t fnv1_hash(const std::string &str);

/// Return a random 32-bit unsigned integer.
uint32_t random_uint32();
/// Return a random float between 0 and 1.
float random_float();
/// Generate \p len number of random bytes.
bool random_bytes(uint8_t *data, size_t len);

///@}

/// @name Bit manipulation
///@{

/// Encode a 16-bit value given the most and least significant byte.
constexpr uint16_t encode_uint16(uint8_t msb, uint8_t lsb) {
  return (static_cast<uint16_t>(msb) << 8) | (static_cast<uint16_t>(lsb));
}
/// Encode a 32-bit value given four bytes in most to least significant byte order.
constexpr uint32_t encode_uint32(uint8_t byte1, uint8_t byte2, uint8_t byte3, uint8_t byte4) {
  return (static_cast<uint32_t>(byte1) << 24) | (static_cast<uint32_t>(byte2) << 16) |
         (static_cast<uint32_t>(byte3) << 8) | (static_cast<uint32_t>(byte4));
}
/// Encode a 24-bit value given three bytes in most to least significant byte order.
constexpr uint32_t encode_uint24(uint8_t byte1, uint8_t byte2, uint8_t byte3) {
  return ((static_cast<uint32_t>(byte1) << 16) | (static_cast<uint32_t>(byte2) << 8) | (static_cast<uint32_t>(byte3)));
}

/// Encode a value from its constituent bytes (from most to least significant) in an array with length sizeof(T).
template<typename T, enable_if_t<std::is_unsigned<T>::value, int> = 0>
constexpr14 T encode_value(const uint8_t *bytes) {
  T val = 0;
  for (size_t i = 0; i < sizeof(T); i++) {
    val <<= 8;
    val |= bytes[i];
  }
  return val;
}
/// Encode a value from its constituent bytes (from most to least significant) in an std::array with length sizeof(T).
template<typename T, enable_if_t<std::is_unsigned<T>::value, int> = 0>
constexpr14 T encode_value(const std::array<uint8_t, sizeof(T)> bytes) {
  return encode_value<T>(bytes.data());
}
/// Decode a value into its constituent bytes (from most to least significant).
template<typename T, enable_if_t<std::is_unsigned<T>::value, int> = 0>
constexpr14 std::array<uint8_t, sizeof(T)> decode_value(T val) {
  std::array<uint8_t, sizeof(T)> ret{};
  for (size_t i = sizeof(T); i > 0; i--) {
    ret[i - 1] = val & 0xFF;
    val >>= 8;
  }
  return ret;
}

/// Reverse the order of 8 bits.
inline uint8_t reverse_bits(uint8_t x) {
  x = ((x & 0xAA) >> 1) | ((x & 0x55) << 1);
  x = ((x & 0xCC) >> 2) | ((x & 0x33) << 2);
  x = ((x & 0xF0) >> 4) | ((x & 0x0F) << 4);
  return x;
}
/// Reverse the order of 16 bits.
inline uint16_t reverse_bits(uint16_t x) {
  return (reverse_bits(static_cast<uint8_t>(x & 0xFF)) << 8) | reverse_bits(static_cast<uint8_t>((x >> 8) & 0xFF));
}
/// Reverse the order of 32 bits.
inline uint32_t reverse_bits(uint32_t x) {
  return (reverse_bits(static_cast<uint16_t>(x & 0xFFFF)) << 16) |
         reverse_bits(static_cast<uint16_t>((x >> 16) & 0xFFFF));
}

/// Convert a value between host byte order and big endian (most significant byte first) order.
template<typename T> constexpr14 T convert_big_endian(T val) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  return byteswap(val);
#else
  return val;
#endif
}

/// Convert a value between host byte order and little endian (least significant byte first) order.
template<typename T> constexpr14 T convert_little_endian(T val) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  return val;
#else
  return byteswap(val);
#endif
}

///@}

/// @name Strings
///@{

/// Compare strings for equality in case-insensitive manner.
bool str_equals_case_insensitive(const std::string &a, const std::string &b);

/// Check whether a string starts with a value.
bool str_startswith(const std::string &str, const std::string &start);
/// Check whether a string ends with a value.
bool str_endswith(const std::string &str, const std::string &end);

/// Convert the value to a string (added as extra overload so that to_string() can be used on all stringifiable types).
inline std::string to_string(const std::string &val) { return val; }

/// Truncate a string to a specific length.
std::string str_truncate(const std::string &str, size_t length);

/// Extract the part of the string until either the first occurrence of the specified character, or the end
/// (requires str to be null-terminated).
std::string str_until(const char *str, char ch);
/// Extract the part of the string until either the first occurrence of the specified character, or the end.
std::string str_until(const std::string &str, char ch);

/// Convert the string to lower case.
std::string str_lower_case(const std::string &str);
/// Convert the string to upper case.
std::string str_upper_case(const std::string &str);
/// Convert the string to snake case (lowercase with underscores).
std::string str_snake_case(const std::string &str);

/// Sanitizes the input string by removing all characters but alphanumerics, dashes and underscores.
std::string str_sanitize(const std::string &str);

/// snprintf-like function returning std::string of maximum length \p len (excluding null terminator).
std::string __attribute__((format(printf, 1, 3))) str_snprintf(const char *fmt, size_t len, ...);

/// sprintf-like function returning std::string.
std::string __attribute__((format(printf, 1, 2))) str_sprintf(const char *fmt, ...);

///@}

}  // namespace esphome

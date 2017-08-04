#ifndef UTILS_H
#define UTILS_H

#include <cstdlib>
#include <stdint.h>
#include <limits>

#if defined(__linux__) || defined(__unix__)
#define __LINUX__
#else
#undef __LINUX__
#endif

#if (defined(_WIN32) && defined(_WIN64)) || (defined(__APPLE__) && defined(__LP64__)) || (defined(__LINUX__) && (defined(__x86_64__) || defined(__x86_64)))
#define __64BIT__
#else
#undef __64BIT__
#endif

#ifndef _WIN32
#ifdef __APPLE__
#define INTERNAL_UNUSED_PARAMETER(x) #x
#define UNREFERENCED_PARAMETER(x) _Pragma(INTERNAL_UNUSED_PARAMETER(unused(x)))
#else
#define UNREFERENCED_PARAMETER(x) ((void)(x))
#endif
#endif

#ifdef __APPLE__
#define COMMAND_KEY_PLUS "\xE2\x8C\x98" // UTF-8 encoding of U+2318 "PLACE OF INTEREST SIGN"
#define ALT_KEY_PLUS "\xE2\x8C\xA5" // UTF-8 encoding of U+2325 "OPTION KEY"
#define SHIFT_KEY_PLUS "\xE2\x87\xA7" // UTF-8 encoding of U+21E7 "UPWARDS WHITE ARROW"
#define COMMAND_SHIFT_KEYS_PLUS SHIFT_KEY_PLUS COMMAND_KEY_PLUS
#define ENTER_KEY_NAME "\xE2\x8C\xA4" // UTF-8 encoding of U+2324 "UP ARROWHEAD BETWEEN TWO HORIZONTAL BARS"
#else
#define COMMAND_KEY_PLUS "Ctrl+"
#define ALT_KEY_PLUS "Alt+"
#define SHIFT_KEY_PLUS "Shift+"
#define COMMAND_SHIFT_KEYS_PLUS COMMAND_KEY_PLUS SHIFT_KEY_PLUS
#define ENTER_KEY_NAME "Enter"
#endif

typedef uint8_t size8_t;
typedef uint16_t size16_t;
typedef uint32_t size32_t;
typedef uint64_t size64_t;

const size32_t NULL_INDEX = std::numeric_limits<size32_t>::max();

#endif

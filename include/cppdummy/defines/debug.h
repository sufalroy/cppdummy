#pragma once

#include "compiler.h"
#include <cstdlib>
#include <print>
#include <source_location>


#if defined(NDEBUG) || defined(CPPDUMMY_RELEASE)
#define CPPDUMMY_BUILD_RELEASE 1
#define CPPDUMMY_BUILD_DEBUG 0
#else
#define CPPDUMMY_BUILD_RELEASE 0
#define CPPDUMMY_BUILD_DEBUG 1
#endif

#if CPPDUMMY_BUILD_DEBUG
#ifdef CPPDUMMY_PLATFORM_WINDOWS
#define CPPDUMMY_DEBUG_BREAK() __debugbreak()
#else
#include <signal.h>
#define CPPDUMMY_DEBUG_BREAK() raise(SIGTRAP)
#endif
#else
#define CPPDUMMY_DEBUG_BREAK() ((void)0)
#endif


#if CPPDUMMY_BUILD_DEBUG

#define CPPDUMMY_ASSERT(condition, message)                                                              \
  do {                                                                                                   \
    if (!(condition)) [[unlikely]] {                                                                     \
      auto loc = std::source_location::current();                                                        \
      std::println(stderr, "ASSERTION FAILED: {}", message);                                             \
      std::println(stderr, "  Condition: {}", #condition);                                               \
      std::println(stderr, "  Location: {}:{} in {}", loc.file_name(), loc.line(), loc.function_name()); \
      CPPDUMMY_DEBUG_BREAK();                                                                            \
      std::abort();                                                                                      \
    }                                                                                                    \
  } while (0)

#define CPPDUMMY_ASSERT_MSG(condition, format, ...)                                                      \
  do {                                                                                                   \
    if (!(condition)) [[unlikely]] {                                                                     \
      auto loc = std::source_location::current();                                                        \
      std::println(stderr, "ASSERTION FAILED: " format, __VA_ARGS__);                                    \
      std::println(stderr, "  Condition: {}", #condition);                                               \
      std::println(stderr, "  Location: {}:{} in {}", loc.file_name(), loc.line(), loc.function_name()); \
      CPPDUMMY_DEBUG_BREAK();                                                                            \
      std::abort();                                                                                      \
    }                                                                                                    \
  } while (0)

#define CPPDUMMY_VERIFY(condition) CPPDUMMY_ASSERT(condition, "Verification failed")
#define CPPDUMMY_UNREACHABLE() CPPDUMMY_ASSERT(false, "Unreachable code executed")

#else

#define CPPDUMMY_ASSERT(condition, message) ((void)0)
#define CPPDUMMY_ASSERT_MSG(condition, format, ...) ((void)0)
#define CPPDUMMY_VERIFY(condition) ((void)(condition))

#ifdef CPPDUMMY_COMPILER_MSVC
#define CPPDUMMY_UNREACHABLE() __assume(0)
#else
#define CPPDUMMY_UNREACHABLE() __builtin_unreachable()
#endif

#endif

#define CPPDUMMY_STATIC_ASSERT(condition, message) static_assert(condition, message)


#if CPPDUMMY_BUILD_DEBUG
#define CPPDUMMY_DEBUG_ONLY(code) \
  do {                            \
    code                          \
  } while (0)
#else
#define CPPDUMMY_DEBUG_ONLY(code) ((void)0)
#endif

#define CPPDUMMY_UNUSED(x) ((void)(x))

#define CPPDUMMY_TODO(message) CPPDUMMY_PRAGMA(message("TODO: " message))
#define CPPDUMMY_FIXME(message) CPPDUMMY_PRAGMA(message("FIXME: " message))
#define CPPDUMMY_HACK(message) CPPDUMMY_PRAGMA(message("HACK: " message))
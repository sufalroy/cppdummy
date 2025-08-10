#pragma once

#if defined(_MSC_VER)
#define CPPDUMMY_COMPILER_MSVC 1
#define CPPDUMMY_COMPILER_CLANG 0
#define CPPDUMMY_COMPILER_GCC 0
#define CPPDUMMY_COMPILER_NAME "MSVC"
#define CPPDUMMY_COMPILER_VERSION _MSC_VER
#elif defined(__clang__)
#define CPPDUMMY_COMPILER_MSVC 0
#define CPPDUMMY_COMPILER_CLANG 1
#define CPPDUMMY_COMPILER_GCC 0
#define CPPDUMMY_COMPILER_NAME "Clang"
#define CPPDUMMY_COMPILER_VERSION (__clang_major__ * 100 + __clang_minor__)
#elif defined(__GNUC__)
#define CPPDUMMY_COMPILER_MSVC 0
#define CPPDUMMY_COMPILER_CLANG 0
#define CPPDUMMY_COMPILER_GCC 1
#define CPPDUMMY_COMPILER_NAME "GCC"
#define CPPDUMMY_COMPILER_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)
#else
#error "Unsupported compiler!"
#endif

#if __cplusplus < 202002L
#error "C++20 or later is required!"
#endif

#if CPPDUMMY_COMPILER_MSVC
#define CPPDUMMY_PRAGMA(x) __pragma(x)
#define CPPDUMMY_FORCE_INLINE __forceinline
#define CPPDUMMY_NEVER_INLINE __declspec(noinline)
#define CPPDUMMY_RESTRICT __restrict
#define CPPDUMMY_THREAD_LOCAL __declspec(thread)
#define CPPDUMMY_DEPRECATED(msg) __declspec(deprecated(msg))
#define CPPDUMMY_ALIGN(n) __declspec(align(n))
#define CPPDUMMY_PACKED
#define CPPDUMMY_LIKELY(x) (x)
#define CPPDUMMY_UNLIKELY(x) (x)
#define CPPDUMMY_FUNCTION_SIGNATURE __FUNCSIG__
#else
#define CPPDUMMY_PRAGMA(x) _Pragma(#x)
#define CPPDUMMY_FORCE_INLINE inline __attribute__((always_inline))
#define CPPDUMMY_NEVER_INLINE __attribute__((noinline))
#define CPPDUMMY_RESTRICT __restrict__
#define CPPDUMMY_THREAD_LOCAL __thread
#define CPPDUMMY_DEPRECATED(msg) __attribute__((deprecated(msg)))
#define CPPDUMMY_ALIGN(n) __attribute__((aligned(n)))
#define CPPDUMMY_PACKED __attribute__((packed))
#define CPPDUMMY_LIKELY(x) __builtin_expect(!!(x), 1)
#define CPPDUMMY_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define CPPDUMMY_FUNCTION_SIGNATURE __PRETTY_FUNCTION__
#endif

#define CPPDUMMY_NODISCARD [[nodiscard]]
#define CPPDUMMY_MAYBE_UNUSED [[maybe_unused]]
#define CPPDUMMY_NORETURN [[noreturn]]
#define CPPDUMMY_FALLTHROUGH [[fallthrough]]

#if CPPDUMMY_COMPILER_MSVC
#define CPPDUMMY_PUSH_WARNING_STATE CPPDUMMY_PRAGMA(warning(push))
#define CPPDUMMY_POP_WARNING_STATE CPPDUMMY_PRAGMA(warning(pop))
#define CPPDUMMY_DISABLE_WARNING(id) CPPDUMMY_PRAGMA(warning(disable : id))
#else
#define CPPDUMMY_PUSH_WARNING_STATE CPPDUMMY_PRAGMA(GCC diagnostic push)
#define CPPDUMMY_POP_WARNING_STATE CPPDUMMY_PRAGMA(GCC diagnostic pop)
#define CPPDUMMY_DISABLE_WARNING(id) CPPDUMMY_PRAGMA(GCC diagnostic ignored id)
#endif

#define CPPDUMMY_PURE __attribute__((pure))
#define CPPDUMMY_CONST __attribute__((const))


#define CPPDUMMY_API
#define CPPDUMMY_INTERNAL static
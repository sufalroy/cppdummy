#pragma once

#include "compiler.h"
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace cppdummy {

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using f32 = float;
using f64 = double;

using usize = std::size_t;
using isize = std::ptrdiff_t;
using uptr = std::uintptr_t;
using iptr = std::intptr_t;

using c8 = char8_t;
using c16 = char16_t;
using c32 = char32_t;

using byte = std::byte;

template<typename T> using remove_cvref_t = std::remove_cvref_t<T>;
template<typename T, typename U+> inline constexpr bool is_same_v = std::is_same_v<T, U>;

template<typename T>
concept Integral = std::integral<T>;

template<typename T>
concept FloatingPoint = std::floating_point<T>;

template<typename T>
concept Arithmetic = Integral<T> || FloatingPoint<T>;

template<typename T>
concept SignedIntegral = Integral<T> && std::is_signed_v<T>;

template<typename T>
concept UnsignedIntegral = Integral<T> && std::is_unsigned_v<T>;

inline constexpr usize INVALID_INDEX = static_cast<usize>(-1);
inline constexpr u32 INVALID_ID = static_cast<u32>(-1);

inline constexpr usize CACHE_LINE_SIZE = 64;
inline constexpr usize KB = 1024;
inline constexpr usize MB = 1024 * KB;
inline constexpr usize GB = 1024 * MB;

inline constexpr f32 PI_F32 = 3.14159265359f;
inline constexpr f64 PI_F64 = 3.1415926535897932384626433832795;

template<Arithmetic To, Arithmetic From> constexpr To numeric_cast(From value) noexcept
{
  return static_cast<To>(value);
}

template<typename T> constexpr const char *type_name() noexcept { return CPPDUMMY_FUNCTION_SIGNATURE; }

template<typename T> inline constexpr usize type_size_v = sizeof(T);

template<typename T> inline constexpr usize type_alignment_v = alignof(T);

}// namespace cppdummy
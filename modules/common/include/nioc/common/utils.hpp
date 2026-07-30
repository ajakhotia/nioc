////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <charconv>
#include <format>
#include <memory>
#include <nioc/common/exception.hpp>
#include <nioc/common/typeTraits.hpp>
#include <stdexcept>
#include <string>
#include <system_error>

namespace nioc::common
{

/// @brief Render @p integer as a lowercase hexadecimal string with a `0x` prefix.
///
/// The inverse of @ref fromHexString: `fromHexString<Integer>(hexString(value)) == value`. Example:
/// `hexString(255U)` yields `"0xff"`. Ids are stored and displayed this way so a value survives
/// text exactly and a person can match it by eye.
template<typename Integer>
[[nodiscard]] std::string hexString(const Integer integer)
{
  return std::format("0x{:x}", integer);
}

/// @brief Parse a hexadecimal string, as produced by @ref hexString, into an @p Integer.
///
/// The inverse of @ref hexString. Symmetric with it: as `hexString` is keyed on the value's type,
/// so is this, and the type is named explicitly since it cannot be deduced from @p text, e.g.
/// `fromHexString<std::uint64_t>(text)`. A leading `0x` (as `hexString` emits) is accepted.
///
/// @tparam Integer The integer type to parse into.
///
/// @param text Hexadecimal digits, optionally `0x`-prefixed.
///
/// @throws std::invalid_argument if @p text is not wholly a hexadecimal integer.
///
/// @throws std::out_of_range if the value does not fit in @p Integer.
template<typename Integer>
[[nodiscard]] Integer fromHexString(const std::string& text)
{
  // std::from_chars does not accept the `0x` prefix that hexString emits, so step past it if
  // present.
  const auto start = (text.starts_with("0x") or text.starts_with("0X")) ? 2 : 0;

  auto value = Integer{};
  const auto* const first = std::to_address(text.begin() + start);
  const auto* const last = std::to_address(text.end());
  const auto [stopped, error] = std::from_chars(first, last, value, 16);

  if(error == std::errc::result_out_of_range)
  {
    throwException<std::out_of_range>(
        "Hexadecimal value '{}' overflows {}.",
        text,
        prettyName<Integer>());
  }
  if(error != std::errc{} or stopped != last)
  {
    throwException<std::invalid_argument>(
        "Cannot parse '{}' as a hexadecimal {}.",
        text,
        prettyName<Integer>());
  }

  return value;
}

/// @brief Returns the running program's name: the filename part of `argV[0]`, with any leading
/// directory path stripped.
///
/// Example:
///
///     int main(int argC, char** argV)
///     {
///       const std::string name = nioc::common::programName(argC, argV); // e.g. "myApp"
///     }
///
/// @param argC Argument count, passed straight through from `main`. When `<= 0`, the result is an
/// empty string and @p argV is never dereferenced.
///
/// @param argV Argument vector, passed straight through from `main`. May be `nullptr` only when
/// @p argC `<= 0`.
///
/// @return The basename of `argV[0]`, or an empty string when @p argC `<= 0`.
std::string programName(int argC, const char* const* argV);

} // namespace nioc::common

////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <filesystem>
#include <nioc/common/utils.hpp>

namespace nioc::common
{

std::string programName(const int argC, const char* const* const argV)
{
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  return (argC > 0) ? std::filesystem::path{argV[0]}.filename().string() : std::string{};
}

} // namespace nioc::common

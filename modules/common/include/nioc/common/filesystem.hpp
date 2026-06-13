////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <filesystem>
#include <nioc/common/exception.hpp>
#include <stdexcept>

namespace nioc::common
{

/// @brief Checks that @p path exists and is a directory.
///
/// @return @p path unchanged, so you can chain the call.
/// @throws std::invalid_argument If @p path does not exist or is not a directory.
inline std::filesystem::path requireExistingDirectory(std::filesystem::path path)
{
  if(not std::filesystem::exists(path))
  {
    throwException<std::invalid_argument>("Directory does not exist: {}", path.string());
  }
  if(not std::filesystem::is_directory(path))
  {
    throwException<std::invalid_argument>("Path is not a directory: {}", path.string());
  }
  return path;
}

/// @brief Checks that @p path exists, is a directory, and is empty.
///
/// @return @p path unchanged, so you can chain the call.
/// @throws std::invalid_argument If @p path does not exist, is not a directory, or is not empty.
inline std::filesystem::path requireEmptyDirectory(std::filesystem::path path)
{
  path = requireExistingDirectory(std::move(path));
  if(not std::filesystem::is_empty(path))
  {
    throwException<std::invalid_argument>("Directory is not empty: {}", path.string());
  }
  return path;
}

} // namespace nioc::common

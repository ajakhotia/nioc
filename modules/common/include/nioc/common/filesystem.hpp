////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <filesystem>
#include <stdexcept>

namespace nioc::common
{

/// @brief Asserts that @p path exists and is a directory.
///
/// @return The validated path, so the call can be chained into a member initializer.
/// @throws std::invalid_argument If @p path does not exist or is not a directory.
inline std::filesystem::path requireExistingDirectory(std::filesystem::path path)
{
  if(not std::filesystem::exists(path))
  {
    throw std::invalid_argument("[nioc::common] Directory does not exist: " + path.string());
  }
  if(not std::filesystem::is_directory(path))
  {
    throw std::invalid_argument("[nioc::common] Path is not a directory: " + path.string());
  }
  return path;
}

/// @brief Asserts that @p path exists, is a directory, and contains no entries.
///
/// @return The validated path, so the call can be chained into a member initializer.
/// @throws std::invalid_argument If @p path does not exist, is not a directory, or is not empty.
inline std::filesystem::path requireEmptyDirectory(std::filesystem::path path)
{
  path = requireExistingDirectory(std::move(path));
  if(not std::filesystem::is_empty(path))
  {
    throw std::invalid_argument("[nioc::common] Directory is not empty: " + path.string());
  }
  return path;
}

} // namespace nioc::common

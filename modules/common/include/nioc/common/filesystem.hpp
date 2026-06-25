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

/// @brief Returns @p path if it names an existing directory; throws otherwise.
///
/// Use as a guard at an API boundary to reject bad inputs early. It returns the path, so it reads
/// well inline:
///
///     auto dir = requireExistingDirectory(arg);
///
/// @param path The directory to check. Passed and returned by value.
///
/// @return @p path, unchanged.
///
/// @throws std::invalid_argument if @p path does not exist or is not a directory.
///
/// @throws std::filesystem::filesystem_error if a filesystem status query fails (for example, a
/// permission error while looking up the path).
///
/// @see requireEmptyDirectory
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

/// @brief Returns @p path if it names an existing directory that is empty; throws otherwise.
///
/// Use to claim a directory as a fresh output location before writing into it.
///
/// @param path The directory to check. Passed and returned by value.
///
/// @return @p path, unchanged.
///
/// @throws std::invalid_argument if @p path does not exist, is not a directory, or is not empty.
///
/// @throws std::filesystem::filesystem_error if a filesystem status query fails (for example, a
/// permission error while looking up the path).
///
/// @see requireExistingDirectory
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

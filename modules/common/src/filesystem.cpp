////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#include <filesystem>
#include <nioc/common/exception.hpp>
#include <nioc/common/filesystem.hpp>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace nioc::common
{

std::filesystem::path requireExistingDirectory(std::filesystem::path path)
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

std::filesystem::path requireExistingFile(std::filesystem::path path)
{
  if(not std::filesystem::exists(path))
  {
    throwException<std::invalid_argument>("File does not exist: {}", path.string());
  }
  if(not std::filesystem::is_regular_file(path))
  {
    throwException<std::invalid_argument>("Path is not a regular file: {}", path.string());
  }
  return path;
}

std::filesystem::path requireEmptyDirectory(std::filesystem::path path)
{
  path = requireExistingDirectory(std::move(path));
  if(not std::filesystem::is_empty(path))
  {
    throwException<std::invalid_argument>("Directory is not empty: {}", path.string());
  }
  return path;
}

ScratchDirectory::ScratchDirectory(std::filesystem::path path): mPath{std::move(path)}
{
  std::filesystem::remove_all(mPath);
  std::filesystem::create_directories(mPath);
}

ScratchDirectory::~ScratchDirectory()
{
  auto errorCode = std::error_code{};
  std::filesystem::remove_all(mPath, errorCode);
}

const std::filesystem::path& ScratchDirectory::path() const noexcept
{
  return mPath;
}

} // namespace nioc::common

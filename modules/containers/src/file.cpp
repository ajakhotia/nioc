////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <cerrno>
#include <fcntl.h>
#include <nioc/common/exception.hpp>
#include <nioc/containers/file.hpp>
#include <nioc/logger/logger.hpp>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <system_error>
#include <unistd.h>
#include <utility>

namespace nioc::containers
{

File File::create(std::filesystem::path path, const std::size_t size)
{
  std::filesystem::create_directories(path.parent_path());

  constexpr auto kFlags = O_RDWR | O_CREAT | O_TRUNC | O_CLOEXEC;
  constexpr auto kMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg): open is the POSIX file API.
  const auto descriptor = ::open(path.c_str(), kFlags, kMode);
  auto file = File{std::move(path), descriptor};
  if(file.mDescriptor < 0)
  {
    common::throwException<std::runtime_error>(
        "Unable to create {}: {}",
        file.mPath.string(),
        std::generic_category().message(errno));
  }
  if(::ftruncate(file.mDescriptor, static_cast<off_t>(size)) != 0)
  {
    common::throwException<std::runtime_error>(
        "Unable to size {}: {}",
        file.mPath.string(),
        std::generic_category().message(errno));
  }
  return file;
}

File File::open(std::filesystem::path path)
{
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg): open is the POSIX file API.
  const auto descriptor = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
  auto file = File{std::move(path), descriptor};
  if(file.mDescriptor < 0)
  {
    common::throwException<std::runtime_error>(
        "Unable to open {}: {}",
        file.mPath.string(),
        std::generic_category().message(errno));
  }
  return file;
}

File::File(const int descriptor) noexcept: File{{}, descriptor} {}

File::File(std::filesystem::path path, const int descriptor) noexcept:
  mPath{std::move(path)},
  mDescriptor{descriptor}
{
}

File::File(File&& other) noexcept:
  mPath{std::move(other.mPath)},
  mDescriptor{std::exchange(other.mDescriptor, -1)}
{
}

File::~File()
{
  if(mDescriptor >= 0)
  {
    static_cast<void>(::close(mDescriptor));
  }
}

int File::nativeHandle() const noexcept
{
  return mDescriptor;
}

const std::filesystem::path& File::path() const noexcept
{
  return mPath;
}

std::size_t File::size() const
{
  struct stat status{};
  if(::fstat(mDescriptor, &status) != 0)
  {
    common::throwException<std::runtime_error>(
        "Unable to stat {}: {}",
        describe(),
        std::generic_category().message(errno));
  }
  return static_cast<std::size_t>(status.st_size);
}

void File::resize(const std::size_t size) const noexcept
{
  if(::ftruncate(mDescriptor, static_cast<off_t>(size)) != 0)
  {
    logger::error("Unable to resize {}: {}", describe(), std::generic_category().message(errno));
  }
}

std::string File::describe() const
{
  return mPath.empty() ? "descriptor " + std::to_string(mDescriptor) : mPath.string();
}

} // namespace nioc::containers

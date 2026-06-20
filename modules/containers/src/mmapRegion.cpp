////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <cerrno>
#include <fcntl.h>
#include <nioc/common/exception.hpp>
#include <nioc/containers/mmapRegion.hpp>
#include <nioc/logger/logger.hpp>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/stat.h>
#include <system_error>
#include <unistd.h>
#include <utility>

namespace nioc::containers
{
namespace
{

int openForWriting(const std::filesystem::path& path, const std::size_t size)
{
  std::filesystem::create_directories(path.parent_path());

  constexpr auto kFlags = O_RDWR | O_CREAT | O_TRUNC | O_CLOEXEC;
  constexpr auto kMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg): open is the POSIX file API.
  const auto fileDescriptor = ::open(path.c_str(), kFlags, kMode);
  if(fileDescriptor < 0)
  {
    common::throwException<std::runtime_error>(
        "Unable to create {}: {}",
        path.string(),
        std::generic_category().message(errno));
  }

  if(::ftruncate(fileDescriptor, static_cast<off_t>(size)) != 0)
  {
    const auto errorNumber = errno;
    static_cast<void>(::close(fileDescriptor));
    common::throwException<std::runtime_error>(
        "Unable to size {}: {}",
        path.string(),
        std::generic_category().message(errorNumber));
  }

  return fileDescriptor;
}

int openForReading(const std::filesystem::path& path)
{
  constexpr auto kFlags = O_RDONLY | O_CLOEXEC;

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg): open is the POSIX file API.
  const auto fileDescriptor = ::open(path.c_str(), kFlags);
  if(fileDescriptor < 0)
  {
    common::throwException<std::runtime_error>(
        "Unable to open {}: {}",
        path.string(),
        std::generic_category().message(errno));
  }

  return fileDescriptor;
}

std::size_t fileSize(const int fileDescriptor, const std::filesystem::path& path)
{
  struct stat status{};
  if(::fstat(fileDescriptor, &status) != 0)
  {
    const auto errorNumber = errno;
    static_cast<void>(::close(fileDescriptor));
    common::throwException<std::runtime_error>(
        "Unable to stat {}: {}",
        path.string(),
        std::generic_category().message(errorNumber));
  }

  return static_cast<std::size_t>(status.st_size);
}

std::span<std::byte> mapMemory(
    const int fileDescriptor,
    const std::size_t size,
    const bool writable,
    const std::filesystem::path& path)
{
  const auto protection = writable ? PROT_READ | PROT_WRITE : PROT_READ;
  void* const address = ::mmap(nullptr, size, protection, MAP_SHARED, fileDescriptor, 0);
  if(address == MAP_FAILED)
  {
    const auto errorNumber = errno;
    static_cast<void>(::close(fileDescriptor));
    common::throwException<std::runtime_error>(
        "Unable to map {}: {}",
        path.string(),
        std::generic_category().message(errorNumber));
  }

  return {static_cast<std::byte*>(address), size};
}

} // namespace

MmapRegion::MmapRegion(std::filesystem::path path, const std::size_t size):
  mPath{std::move(path)},
  mFileDescriptor{openForWriting(mPath, size)},
  mBytes{mapMemory(mFileDescriptor, size, true, mPath)}
{
}

MmapRegion::MmapRegion(std::filesystem::path path):
  mPath{std::move(path)},
  mFileDescriptor{openForReading(mPath)},
  mBytes{mapMemory(mFileDescriptor, fileSize(mFileDescriptor, mPath), false, mPath)}
{
}

MmapRegion::~MmapRegion()
{
  static_cast<void>(::munmap(mBytes.data(), mBytes.size()));
  static_cast<void>(::close(mFileDescriptor));
}

std::span<std::byte> MmapRegion::bytes() noexcept
{
  return mBytes;
}

std::span<const std::byte> MmapRegion::bytes() const noexcept
{
  return mBytes;
}

bool MmapRegion::empty() const noexcept
{
  return mBytes.empty();
}

std::size_t MmapRegion::size() const noexcept
{
  return mBytes.size();
}

void MmapRegion::resize(const std::size_t size) noexcept
{
  if(::ftruncate(mFileDescriptor, static_cast<off_t>(size)) != 0)
  {
    const auto errorNumber = errno;
    logger::error(
        "Unable to resize {}: {}",
        mPath.string(),
        std::generic_category().message(errorNumber));
  }
}

} // namespace nioc::containers

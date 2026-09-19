////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <cerrno>
#include <functional>
#include <iterator>
#include <memory>
#include <nioc/common/exception.hpp>
#include <nioc/containers/mapping.hpp>
#include <nioc/logger/logger.hpp>
#include <stdexcept>
#include <sys/mman.h>
#include <system_error>
#include <unistd.h>
#include <utility>

namespace nioc::containers
{
namespace
{

/// @brief mmap @p byteCount bytes of @p file at @p offset with @p protection, or throw.
std::span<std::byte> map(
    const File& file,
    const std::size_t byteCount,
    const std::int64_t offset,
    const int protection)
{
  if(byteCount == 0)
  {
    // mmap rejects a zero-length request; an empty range is simply an empty mapping.
    return {};
  }

  void* const address =
      ::mmap(nullptr, byteCount, protection, MAP_SHARED, file.nativeHandle(), offset);
  if(address == MAP_FAILED)
  {
    common::throwException<std::runtime_error>(
        "Unable to map {} bytes of {} at offset {}: {}",
        byteCount,
        file.describe(),
        offset,
        std::generic_category().message(errno));
  }
  return {static_cast<std::byte*>(address), byteCount};
}

} // namespace

Mapping Mapping::readOnly(const File& file, const std::size_t byteCount, const std::int64_t offset)
{
  return Mapping{map(file, byteCount, offset, PROT_READ)};
}

Mapping Mapping::readWrite(const File& file, const std::size_t byteCount, const std::int64_t offset)
{
  return Mapping{map(file, byteCount, offset, PROT_READ | PROT_WRITE)};
}

Mapping::Mapping(const std::span<std::byte> bytes) noexcept: mBytes{bytes} {}

Mapping::Mapping(Mapping&& other) noexcept: mBytes{std::exchange(other.mBytes, {})} {}

Mapping::~Mapping()
{
  if(not mBytes.empty())
  {
    static_cast<void>(::munmap(mBytes.data(), mBytes.size()));
  }
}

std::span<std::byte> Mapping::bytes() noexcept
{
  return mBytes;
}

std::span<const std::byte> Mapping::bytes() const noexcept
{
  return mBytes;
}

bool Mapping::empty() const noexcept
{
  return mBytes.empty();
}

std::size_t Mapping::size() const noexcept
{
  return mBytes.size();
}

void Mapping::evict(const std::span<const std::byte> range) const noexcept
{
  // std::less_equal gives a total order over unrelated pointers, so a foreign span is rejected
  // rather than compared with unspecified results.
  const auto mapped = std::span<const std::byte>{mBytes};
  const auto notAfter = std::less_equal<const std::byte*>{};
  if(range.empty() or
     mapped.empty() or
     not notAfter(mapped.data(), range.data()) or
     not notAfter(std::to_address(range.end()), std::to_address(mapped.end())))
  {
    return;
  }

  // Shrink to the pages wholly inside the range: first page boundary at or after the start, last
  // page boundary at or before the end.
  const auto pageSize = static_cast<std::size_t>(::sysconf(_SC_PAGESIZE));
  const auto begin = static_cast<std::size_t>(std::distance(mapped.begin(), range.begin()));
  const auto end = begin + range.size();
  const auto firstPage = ((begin + pageSize - 1) / pageSize) * pageSize;
  const auto lastPage = (end / pageSize) * pageSize;
  if(firstPage >= lastPage)
  {
    return;
  }

  // glibc's posix_madvise silently ignores DONTNEED, so the raw madvise(2) is required.
  if(::madvise(mBytes.subspan(firstPage).data(), lastPage - firstPage, MADV_DONTNEED) != 0)
  {
    logger::debug(
        "Evicting {} bytes at {} was not applied: {}",
        lastPage - firstPage,
        static_cast<const void*>(mBytes.data()),
        std::generic_category().message(errno));
  }
}

} // namespace nioc::containers

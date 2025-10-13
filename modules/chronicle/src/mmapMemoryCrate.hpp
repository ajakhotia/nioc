////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "utils.hpp"
#include <boost/iostreams/device/mapped_file.hpp>
#include <nioc/chronicle/memoryCrate.hpp>

namespace nioc::chronicle
{


class MemoryCrate::MmapMemoryCrate
{
public:
  using MappedFile = boost::iostreams::mapped_file_source;

  using MappedFilePtr = std::shared_ptr<MappedFile>;

  MmapMemoryCrate(MappedFilePtr mappedFilePtr, const IndexEntry& indexEntry);

  MmapMemoryCrate(const MmapMemoryCrate&) = default;

  MmapMemoryCrate(MmapMemoryCrate&&) = default;

  ~MmapMemoryCrate() = default;

  MmapMemoryCrate& operator=(const MmapMemoryCrate&) = delete;

  MmapMemoryCrate& operator=(MmapMemoryCrate&&) = delete;

  [[nodiscard]] const std::span<const std::byte>& span() const noexcept;

private:
  MappedFilePtr mMappedFilePtr;

  std::span<const std::byte> mSpan;
};


} // namespace nioc::chronicle

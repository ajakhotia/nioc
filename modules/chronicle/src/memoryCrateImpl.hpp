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


class MemoryCrate::MemoryCrateImpl
{
public:
  using MappedFile = boost::iostreams::mapped_file_source;

  using MappedFilePtr = std::shared_ptr<MappedFile>;

  MemoryCrateImpl(MappedFilePtr mappedFilePtr, const IndexEntry& index);

  MemoryCrateImpl(const MemoryCrateImpl&) = default;

  MemoryCrateImpl(MemoryCrateImpl&&) = default;

  ~MemoryCrateImpl() = default;

  MemoryCrateImpl& operator=(const MemoryCrateImpl&) = default;

  MemoryCrateImpl& operator=(MemoryCrateImpl&&) = default;

  [[nodiscard]] const std::span<const std::byte>& span() const noexcept;

private:
  MappedFilePtr mMappedFilePtr;

  std::span<const std::byte> mSpan;
};


} // namespace nioc::chronicle

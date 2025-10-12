////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <filesystem>
#include <fstream>
#include <span>
#include <vector>

namespace nioc::chronicle
{

class StreamChannelWriter
{
public:
  static constexpr auto kDefaultMaxFileSizeInBytes = 128ULL * 1024ULL * 1024ULL;

  using ConstByteSpan = std::span<const std::byte>;

  explicit StreamChannelWriter(
      std::filesystem::path logRoot,
      std::uint64_t maxFileSizeInBytes = kDefaultMaxFileSizeInBytes);

  StreamChannelWriter(const StreamChannelWriter&) = delete;

  StreamChannelWriter(StreamChannelWriter&&) noexcept = default;

  ~StreamChannelWriter() = default;

  StreamChannelWriter& operator=(const StreamChannelWriter&) = delete;

  StreamChannelWriter& operator=(StreamChannelWriter&&) noexcept = default;

  void writeFrame(const ConstByteSpan& data);

  void writeFrame(const std::vector<ConstByteSpan>& dataCollection);

private:
  std::filesystem::path mLogRoot;

  std::ofstream mIndexFile;

  std::uint64_t mMaxFileSizeInBytes;

  std::uint64_t mRollCounter;

  std::ofstream mActiveLogRoll;

  void rollCheckAndIndex(std::uint64_t requiredSizeInBytes);

  std::filesystem::path nextRollFilePath();
};

} // namespace nioc::chronicle

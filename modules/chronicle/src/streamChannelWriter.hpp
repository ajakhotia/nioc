////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <filesystem>
#include <fstream>
#include <nioc/chronicle/channelWriter.hpp>
#include <span>

namespace nioc::chronicle
{

class StreamChannelWriter final: public ChannelWriter
{
public:
  static constexpr auto kDefaultMaxFileSizeInBytes = 128ULL * 1024ULL * 1024ULL;

  using ConstByteSpan = std::span<const std::byte>;

  explicit StreamChannelWriter(
      std::filesystem::path logRoot,
      std::uint64_t maxFileSizeInBytes = kDefaultMaxFileSizeInBytes);

  StreamChannelWriter(const StreamChannelWriter&) = delete;

  StreamChannelWriter(StreamChannelWriter&&) noexcept = delete;

  ~StreamChannelWriter() override = default;

  StreamChannelWriter& operator=(const StreamChannelWriter&) = delete;

  StreamChannelWriter& operator=(StreamChannelWriter&&) noexcept = delete;

  void writeFrame(const ConstByteSpan& data) override;

  void writeFrame(std::span<const ConstByteSpan> dataCollection) override;

private:
  const std::filesystem::path mLogRoot;

  const std::uint64_t mMaxFileSizeInBytes;

  std::ofstream mIndexFile;

  std::uint64_t mRollCounter;

  std::ofstream mActiveLogRoll;

  void rollCheckAndIndex(std::uint64_t requiredSizeInBytes);

  std::filesystem::path nextRollFilePath();
};

} // namespace nioc::chronicle

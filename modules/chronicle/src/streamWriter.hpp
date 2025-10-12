////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "streamChannelWriter.hpp"
#include <nioc/chronicle/chronicle.hpp>

#include <nioc/common/locked.hpp>
#include <unordered_map>

namespace nioc::chronicle
{

class Writer::StreamWriter
{
public:
  using LockedChannel = common::Locked<StreamChannelWriter>;

  using ChannelPtrMap = std::unordered_map<ChannelId, std::unique_ptr<LockedChannel>>;

  explicit StreamWriter(std::filesystem::path logRoot, std::size_t maxFileSizeInBytes);

  StreamWriter(const StreamWriter&) = delete;

  StreamWriter(StreamWriter&&) noexcept = delete;

  ~StreamWriter() = default;

  StreamWriter& operator=(const StreamWriter&) = delete;

  StreamWriter& operator=(StreamWriter&&) noexcept = delete;

  void write(ChannelId channelId, const std::span<const std::byte>& data);

  void write(ChannelId channelId, const std::vector<std::span<const std::byte>>& data);

  [[nodiscard]] const std::filesystem::path& path() const noexcept;

private:
  const std::filesystem::path mLogDirectory;

  const std::size_t mMaxFileSizeInBytes;

  common::Locked<std::ofstream> mLockedSequenceFile;

  common::Locked<ChannelPtrMap> mLockedChannelPtrMap;

  LockedChannel& acquireChannel(ChannelId channelId);
};

} // namespace nioc::chronicle

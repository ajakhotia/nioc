////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <span>

namespace nioc::chronicle
{

/// @brief Interface for writing data to a channel.
///
/// Channel writers handle the storage details of chronicle data.
class ChannelWriter
{
public:
  using ConstByteSpan = std::span<const std::byte>;

  ChannelWriter() = default;

  ChannelWriter(const ChannelWriter&) = delete;

  ChannelWriter(ChannelWriter&&) noexcept = delete;

  virtual ~ChannelWriter() = default;

  ChannelWriter& operator=(const ChannelWriter&) = delete;

  ChannelWriter& operator=(ChannelWriter&&) noexcept = delete;

  /// @brief Writes a single data frame.
  /// @param data Data to write.
  virtual void writeFrame(const ConstByteSpan& data) = 0;

  /// @brief Writes multiple data spans as a single frame.
  /// @param dataCollection Data spans to write as one frame.
  virtual void writeFrame(std::span<const ConstByteSpan> dataCollection) = 0;
};

} // namespace nioc::chronicle

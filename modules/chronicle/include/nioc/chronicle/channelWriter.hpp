////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <span>

namespace nioc::chronicle
{

/// @brief Interface for writing chronicle data to a channel.
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

  /// @brief Writes one frame.
  /// @param data Bytes to write.
  virtual void writeFrame(const ConstByteSpan& data) = 0;

  /// @brief Writes several byte spans joined into one frame.
  /// @param dataCollection Byte spans to write as one frame.
  virtual void writeFrame(std::span<const ConstByteSpan> dataCollection) = 0;
};

} // namespace nioc::chronicle

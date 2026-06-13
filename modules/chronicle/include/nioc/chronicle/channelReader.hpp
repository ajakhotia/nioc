////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "memoryCrate.hpp"

namespace nioc::chronicle
{

/// @brief Reads frames from a channel.
class ChannelReader
{
public:
  ChannelReader() = default;

  ChannelReader(const ChannelReader&) = delete;

  ChannelReader(ChannelReader&&) noexcept = delete;

  virtual ~ChannelReader() = default;

  ChannelReader& operator=(const ChannelReader&) = delete;

  ChannelReader& operator=(ChannelReader&&) noexcept = delete;

  /// @brief Reads the next frame from the channel.
  /// @return Memory crate containing the frame data.
  /// @throws std::runtime_error When no more frames remain in the channel.
  [[nodiscard]] virtual MemoryCrate read() = 0;
};

} // namespace nioc::chronicle

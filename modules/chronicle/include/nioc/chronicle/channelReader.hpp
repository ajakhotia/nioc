////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "memoryCrate.hpp"

namespace nioc::chronicle
{

/// @brief Interface for reading data from a channel.
///
/// Channel readers handle the retrieval details of chronicle data.
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
  [[nodiscard]] virtual MemoryCrate read() = 0;
};

} // namespace nioc::chronicle

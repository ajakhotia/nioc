////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <nioc/containers/mmapArray.hpp>
#include <nioc/containers/tape.hpp>
#include <span>
#include <utility>

namespace nioc::chronicle
{

class Channel;

/// @brief A claimed, writable region of a channel to build one frame into.
///
/// Write the frame into @ref span, then hand the reservation to @ref Crate to record the frame and
/// obtain a view over it (`Crate{std::move(reservation), usedSize}`). A reservation that is dropped
/// without being made into a crate records nothing. Mint one with @ref Channel::reserve.
class Reservation
{
public:
  /// @brief Constructs an empty reservation; its @ref span is empty and it records nothing.
  Reservation() noexcept = default;

  Reservation(const Reservation&) = delete;

  Reservation(Reservation&&) noexcept = default;

  ~Reservation() = default;

  Reservation& operator=(const Reservation&) = delete;

  Reservation& operator=(Reservation&&) noexcept = default;

  /// @brief Returns the writable region to build the frame into.
  [[nodiscard]] std::span<std::byte> span() const noexcept
  {
    return mSpan;
  }

private:
  friend class Channel;
  friend class Crate;

  Reservation(
      Channel& channel,
      std::shared_ptr<containers::Tape<containers::MmapArray<std::byte>>> rollPtr,
      const std::uint64_t rollId,
      const std::span<std::byte> span):
    mChannel{&channel},
    mRollPtr{std::move(rollPtr)},
    mRollId{rollId},
    mSpan{span}
  {
  }

  Channel* mChannel{nullptr};
  std::shared_ptr<containers::Tape<containers::MmapArray<std::byte>>> mRollPtr;
  std::uint64_t mRollId{0ULL};
  std::span<std::byte> mSpan;
};

} // namespace nioc::chronicle

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

namespace nioc::chronicle
{

class Channel;
class Crate;

/// @brief A claimed, writable region of a channel to build one frame into.
///
/// Write the frame into @ref span, then @ref commit it to get a @ref Crate viewing the frame. A
/// reservation dropped without being committed records nothing. Mint one with @ref
/// Channel::reserve.
class Reservation
{
public:
  /// @brief Constructs an empty reservation; its @ref span is empty and it records nothing.
  Reservation() noexcept = delete;

  Reservation(const Reservation&) = delete;

  Reservation(Reservation&&) noexcept = default;

  /// @brief Rewinds the claim off the channel unless it was consumed (made into a @ref Crate or
  /// resized away), so an abandoned reservation records nothing.
  ~Reservation();

  Reservation& operator=(const Reservation&) = delete;

  /// @brief Rewinds this reservation's claim (if unconsumed) before taking over @p other's.
  Reservation& operator=(Reservation&& other) noexcept;

  /// @brief Returns the writable region to build the frame into.
  [[nodiscard]] std::span<std::byte> span() const noexcept;

  /// @brief Resizes this reservation to @p newSize writable bytes.
  ///
  /// Routes through the channel (see @ref Channel::modify): the current claim is released and a new
  /// one of @p newSize is taken, on a fresh roll if it no longer fits the current one. No bytes are
  /// carried over — @ref span's contents are abandoned, so copy out anything worth keeping first.
  ///
  /// @param newSize New writable size in bytes.
  void modify(std::size_t newSize);

  /// @brief Commits the first @p usedSize bytes as a frame and returns a @ref Crate viewing them.
  ///
  /// The channel reclaims the reserved space the frame did not use and appends the frame's place to
  /// its timeline; the returned crate takes over keeping the roll mapped. Spends the reservation.
  ///
  /// @param usedSize Number of leading bytes of @ref span that hold the frame.
  [[nodiscard]] Crate commit(std::size_t usedSize) &&;

private:
  friend class Channel;

  Reservation(
      Channel& channel,
      std::shared_ptr<containers::Tape<containers::MmapArray<std::byte>>> rollPtr,
      std::uint64_t rollId,
      std::span<std::byte> span);

  Channel* mChannelPtr;
  std::shared_ptr<containers::Tape<containers::MmapArray<std::byte>>> mRollPtr;
  std::uint64_t mRollId{0ULL};
  std::span<std::byte> mSpan;
};

} // namespace nioc::chronicle

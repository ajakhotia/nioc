////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstddef>
#include <memory>
#include <span>
#include <utility>

namespace nioc::chronicle
{

class Reservation;

/// @brief An immutable, contiguous view of bytes that keeps its backing alive.
///
/// Cheap to copy; all copies share the same bytes. The backing storage is opaque to the holder. The
/// bytes stay valid for as long as any copy of the crate lives.
class Crate
{
public:
  /// @brief Constructs an empty crate.
  Crate() noexcept = default;

  /// @brief Constructs a crate over @p span, keeping @p backing alive for as long as it lives.
  ///
  /// @param backing Owner of the storage the @p span points into.
  ///
  /// @param span The bytes this crate exposes.
  Crate(std::shared_ptr<const void> backing, const std::span<const std::byte> span) noexcept:
    mBacking{std::move(backing)},
    mSpan{span}
  {
  }

  /// @brief Records the first @p usedSize bytes of @p reservation as a frame and views them.
  ///
  /// This is how a reservation becomes a finished frame: the channel reclaims the reserved space
  /// the frame did not use and appends the frame's place to its timeline, and the crate takes over
  /// keeping the roll mapped. The reservation is spent afterward.
  ///
  /// @param reservation A reservation whose frame has been built into its @ref Reservation::span.
  ///
  /// @param usedSize Number of leading bytes of the reservation that hold the frame.
  explicit Crate(Reservation reservation, std::size_t usedSize);

  /// @brief Returns a read-only view of the bytes.
  [[nodiscard]] std::span<const std::byte> span() const noexcept
  {
    return mSpan;
  }

private:
  std::shared_ptr<const void> mBacking;
  std::span<const std::byte> mSpan;
};

} // namespace nioc::chronicle

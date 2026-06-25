////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstddef>
#include <memory>
#include <span>

namespace nioc::chronicle
{

/// @brief A read-only view over a contiguous byte range that owns a share of the memory it points
/// into, keeping those bytes alive for as long as the crate (or any copy) exists.
///
/// A Crate pairs a `std::span<const std::byte>` with a `std::shared_ptr` to the storage behind it.
/// The producer can drop its own handle to the memory; the bytes stay valid because the crate holds
/// a shared owner. Copies are cheap: they share the same bytes and storage instead of duplicating
/// them. Copy and pass by value freely.
///
/// Example:
///
///     auto buffer = std::make_shared<std::vector<std::byte>>(readBytes());
///     Crate crate{buffer, std::span<const std::byte>{*buffer}};
///     buffer.reset();            // Producer drops its handle.
///     auto bytes = crate.span(); // Bytes are still valid; crate keeps the storage alive.
///
/// @see span
class Crate
{
public:
  /// @brief Construct an empty crate: span() is empty and no storage is owned.
  Crate() noexcept = default;

  /// @brief Bind a read-only byte view to the storage that keeps it alive.
  ///
  /// @param backing Shared owner of the underlying memory; retained for the crate's lifetime. The
  /// pointee type is erased, so any allocation can back the bytes.
  ///
  /// @param span The bytes to expose. Must point into memory kept alive by @p backing.
  Crate(std::shared_ptr<const void> backing, std::span<const std::byte> span) noexcept;

  /// @brief Return the exposed bytes.
  ///
  /// The result is empty for a default-constructed crate. It stays valid while this crate, or any
  /// copy of it, lives.
  [[nodiscard]] std::span<const std::byte> span() const noexcept;

private:
  /// Shared owner of the underlying memory. Holds a share of the storage so the exposed bytes stay
  /// valid for the crate's lifetime; null for a default-constructed crate.
  std::shared_ptr<const void> mBacking;

  /// The read-only byte view returned by span(). Points into memory kept alive by mBacking; empty
  /// for a default-constructed crate.
  std::span<const std::byte> mSpan;
};

} // namespace nioc::chronicle

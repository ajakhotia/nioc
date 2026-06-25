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

/// @brief A move-only, single-use write handle to a byte slot inside a channel's active roll (the
/// memory-mapped data file the channel currently appends records to).
///
/// Get one from Channel::reserve. Write into the bytes from span(), then either commit() to
/// publish a record or let the handle go out of scope to throw the slot away.
///
/// Example:
///
///     Reservation res = channel.reserve(64);
///     std::memcpy(res.span().data(), payload, payloadSize);
///     Crate crate = std::move(res).commit(payloadSize); // publishes the record
///
/// Exactly one live Reservation ever refers to a given slot, so no locking is needed. A
/// moved-from handle owns nothing and must not be used.
///
/// @see Channel, Crate
class Reservation
{
public:
  Reservation() noexcept = delete;

  Reservation(const Reservation&) = delete;

  Reservation(Reservation&&) noexcept = default;

  /// @brief Abandons the slot, returning it to the roll without publishing a record.
  ///
  /// A no-op after the handle has been moved-from or committed.
  ~Reservation();

  Reservation& operator=(const Reservation&) = delete;

  /// @brief Swaps ownership with @p other.
  ///
  /// Any slot this handle held passes to @p other and is abandoned when @p other is destroyed.
  Reservation& operator=(Reservation&& other) noexcept;

  /// @brief Returns the writable bytes of the slot.
  ///
  /// The size is the requested reserve size rounded up to a machine word, so it may exceed what
  /// you asked for. The span is valid until this handle is committed, passed to modify(), or
  /// destroyed; modify() may relocate the bytes, so re-read span() after calling it.
  [[nodiscard]] std::span<std::byte> span() const noexcept;

  /// @brief Resizes the slot to hold @p newSize usable bytes.
  ///
  /// Shrinking (or no change) keeps the slot's start and contents, returning the freed tail to the
  /// roll; span() still reports the old, larger extent, so treat only the first @p newSize bytes as
  /// yours. Growing throws away the old slot and claims a fresh one, possibly on a new roll, losing
  /// any bytes already written. Always re-read span() afterward.
  void modify(std::size_t newSize);

  /// @brief Publishes the first @p usedSize bytes as a record and returns a Crate viewing them.
  ///
  /// Consumes the handle (rvalue-qualified; call as `std::move(res).commit(n)`). Adds a timeline
  /// entry for the record and, when no later reservation has claimed past this slot, returns the
  /// unused tail to the roll.
  ///
  /// @param usedSize Bytes to publish. Must not exceed span().size().
  ///
  /// @throws std::runtime_error if the channel's timeline is full.
  [[nodiscard]] Crate commit(std::size_t usedSize) &&;

private:
  friend class Channel;

  /// @brief Binds a fresh handle to a slot the channel has just claimed. Called only by Channel.
  ///
  /// @param channel The owning channel, used to abandon or commit the slot later. Must outlive
  /// this handle.
  ///
  /// @param rollPtr The roll that backs the slot. Shared so the bytes stay alive even if the
  /// channel rolls over to a new roll.
  ///
  /// @param rollId Identifies which roll the slot lives on, so a later commit or modify can tell
  /// whether the active roll has since changed.
  ///
  /// @param span The writable bytes of the slot, already rounded up to a machine word.
  Reservation(
      Channel& channel,
      std::shared_ptr<containers::Tape<containers::MmapArray<std::byte>>> rollPtr,
      std::uint64_t rollId,
      std::span<std::byte> span);

  /// The owning channel. Null once the handle has been moved-from.
  Channel* mChannelPtr;

  /// The roll backing the slot, kept alive for this handle's lifetime.
  std::shared_ptr<containers::Tape<containers::MmapArray<std::byte>>> mRollPtr;

  /// Identifies the roll the slot lives on, so commit and modify can detect a roll-over.
  std::uint64_t mRollId{0ULL};

  /// The writable bytes of the slot. Empty once the handle has been moved-from.
  std::span<std::byte> mSpan;
};

} // namespace nioc::chronicle

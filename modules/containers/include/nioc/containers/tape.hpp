////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <atomic>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <memory>
#include <nioc/common/exception.hpp>
#include <nioc/logger/logger.hpp>
#include <ranges>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace nioc::containers
{

/// @brief A fixed-capacity, append-only buffer that hands disjoint slices to concurrent writers.
///
/// Think of it as a tape with a cursor. The cursor starts at 0 and only moves forward as callers
/// reserve slots with claim or emplace. Every reservation returns its own region; no two overlap.
/// The cursor moves backward only when rewind gives back the unused tail of the latest claim.
/// Capacity is fixed at construction and never grows (shrink_to_fit can lower it).
///
/// The tape reserves space; it does not construct, destroy, or zero elements. Filling a claimed
/// slot is the caller's job, and element access is unchecked.
///
/// Example:
///
///     Tape<std::vector<int>> tape(1000); // capacity 1000 ints
///     std::span<int> slot = tape.claim(3);
///     slot[0] = 1; slot[1] = 2; slot[2] = 3;
///
/// The reservation methods (claim, emplace, rewind, size, capacity, empty, full) are safe to call
/// from many threads at once. The cursor does NOT publish element contents: claim only reserves
/// bytes, so a reader that sees a grown size() must establish its own happens-before with the
/// writer before reading those bytes. The tape cannot be copied or moved; it is pinned to its
/// storage.
///
/// @tparam Storage A contiguous, sized range of trivially-copyable elements, owned by the tape.
///
/// @see claim, emplace, rewind
template<typename Storage>
  requires std::ranges::contiguous_range<Storage> and
           std::ranges::sized_range<Storage> and
           std::is_trivially_copyable_v<std::ranges::range_value_t<Storage>>
class Tape
{
public:
  using value_type = std::ranges::range_value_t<Storage>;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = value_type*;
  using const_pointer = const value_type*;
  using iterator = pointer;
  using const_iterator = const_pointer;

  /// @brief Build the backing storage in place by forwarding @p args to its constructor.
  ///
  /// The storage's element count becomes the tape's fixed capacity; the cursor starts at 0.
  ///
  /// @tparam Args Types of the arguments forwarded to the @p Storage constructor.
  ///
  /// @param args Arguments forwarded to the @p Storage constructor.
  template<typename... Args>
    requires std::constructible_from<Storage, Args...>
  explicit Tape(Args&&... args): mStorage(std::forward<Args>(args)...)
  {
  }

  Tape(const Tape&) = delete;

  Tape(Tape&&) noexcept = delete;

  ~Tape() = default;

  Tape& operator=(const Tape&) = delete;

  Tape& operator=(Tape&&) noexcept = delete;

  /// @brief Pointer to the first element of the backing buffer. Const through a const tape.
  [[nodiscard]] auto data(this auto&& self) noexcept
  {
    return std::ranges::data(self.mStorage);
  }

  /// @brief Reference to the element at @p index. Const through a const tape.
  ///
  /// @param index Element offset. Unchecked; must be < size().
  [[nodiscard]] decltype(auto) operator[](this auto&& self, const size_type index) noexcept
  {
    return self.data()[index]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  }

  /// @brief Reference to the element at @p index, bounds-checked. Const through a const tape.
  ///
  /// @param index Element offset.
  ///
  /// @throws std::out_of_range if @p index is not less than `size()`.
  [[nodiscard]] decltype(auto) at(this auto&& self, const size_type index)
  {
    if(index >= self.size())
    {
      common::throwException<std::out_of_range>(
          "Index {} is out of range for a tape of size {}.",
          index,
          self.size());
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
    return std::forward<decltype(self)>(self)[index];
  }

  /// @brief Iterator to the start of the claimed prefix [0, size()). Const through a const tape.
  ///
  /// Concurrent claims can extend the range while you iterate, so snapshot size() and iterate
  /// single-threaded, or stop other writers first.
  [[nodiscard]] auto begin(this auto&& self) noexcept
  {
    return self.data();
  }

  /// @brief Past-the-end iterator at the current cursor position. Const through a const tape.
  [[nodiscard]] auto end(this auto&& self) noexcept
  {
    return self.data() + self.size(); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  }

  /// @brief Const iterator to the start of the claimed prefix.
  [[nodiscard]] const_iterator cbegin() const noexcept
  {
    return begin();
  }

  /// @brief Const past-the-end iterator at the current cursor position.
  [[nodiscard]] const_iterator cend() const noexcept
  {
    return end();
  }

  /// @brief True when no slots have been claimed yet (size() == 0).
  [[nodiscard]] bool empty() const noexcept
  {
    return size() == 0;
  }

  /// @brief True when the cursor has reached capacity; any further non-zero claim will fail.
  [[nodiscard]] bool full() const noexcept
  {
    return size() == capacity();
  }

  /// @brief Number of slots claimed so far, i.e. the cursor position.
  ///
  /// Read atomically. Under concurrent claims it may already be stale when it returns.
  [[nodiscard]] size_type size() const noexcept
  {
    return mCursor.load(std::memory_order_relaxed);
  }

  /// @brief Total slot capacity, fixed at construction (until shrink_to_fit).
  [[nodiscard]] size_type capacity() const noexcept
  {
    return std::ranges::size(mStorage);
  }

  /// @brief Drop unclaimed capacity by resizing the storage down to size().
  ///
  /// Afterward capacity() equals size(). Only available when @p Storage has a resize member.
  /// NOT thread-safe: call it with no concurrent reservations. It invalidates every pointer, span,
  /// and iterator previously obtained from this tape.
  void shrink_to_fit() noexcept
    requires requires(Storage& storage, size_type count) { storage.resize(count); }
  {
    mStorage.resize(size());
  }

  /// @brief Reserve @p count contiguous slots and return a writable span over them.
  ///
  /// The returned region is disjoint from every other reservation. Thread-safe. If fewer than
  /// @p count slots remain, returns an empty span and leaves the cursor unchanged. The slots are
  /// uninitialized: fill them, then arrange your own happens-before before signaling any reader.
  ///
  /// @param count Number of slots to reserve. Must be non-zero.
  ///
  /// @return A span over the reserved slots, or an empty span if not enough room.
  ///
  /// @throws std::invalid_argument if @p count is zero.
  [[nodiscard]] std::span<value_type> claim(const size_type count = 1)
  {
    if(count == 0)
    {
      common::throwException<std::invalid_argument>(
          "Cannot claim {} slots; count must be non-zero.",
          count);
    }

    // Atomically advance the cursor; the RMW makes reservations disjoint without a lock. The cursor
    // only reserves space — making the slot's contents visible to a reader is the caller's concern.
    auto current = mCursor.load(std::memory_order_relaxed);
    while(true)
    {
      if(count > capacity() - current)
      {
        return {};
      }

      if(mCursor.compare_exchange_weak(current, current + count, std::memory_order_relaxed))
      {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic): reserved slots.
        return {std::ranges::data(mStorage) + current, count};
      }
    }
  }

  /// @brief Give back the unused tail of a just-claimed @p slot, keeping only its first
  /// @p usedCount elements.
  ///
  /// Thread-safe. Succeeds only when @p slot is the most recent claim and the cursor still sits at
  /// its end (no other claim has advanced past it); then the reclaimed tail becomes available to
  /// later claims. Otherwise the reservation stands and this returns false.
  ///
  /// @param slot A span returned by a prior claim on this tape.
  ///
  /// @param usedCount Number of leading elements to keep. Must be <= slot.size(); if larger, the
  /// call logs an error and returns false.
  ///
  /// @return True if the cursor moved back to release the tail; false otherwise.
  [[nodiscard]] bool rewind(const std::span<value_type> slot, const size_type usedCount) noexcept
  {
    if(usedCount > slot.size())
    {
      logger::error("Rewind usedCount {} exceeds slot size {}.", usedCount, slot.size());
      return false;
    }

    const auto slotStart = static_cast<size_type>(std::distance(data(), slot.data()));
    auto slotEnd = slotStart + slot.size();
    return mCursor.compare_exchange_strong(
        slotEnd,
        slotStart + usedCount,
        std::memory_order_relaxed);
  }

  /// @brief Claim one slot and construct an element in it from @p args.
  ///
  /// Thread-safe. Making the new element visible to readers is still your responsibility.
  ///
  /// @tparam Args Types of the arguments forwarded to the @p value_type constructor.
  ///
  /// @param args Arguments forwarded to the @p value_type constructor.
  ///
  /// @return Pointer to the constructed element, or nullptr if the tape is full.
  ///
  /// @see claim
  template<typename... Args>
  [[nodiscard]] pointer emplace(Args&&... args) noexcept(
      std::is_nothrow_constructible_v<value_type, Args...>)
  {
    const auto slot = claim();
    if(slot.empty())
    {
      return nullptr;
    }

    return std::construct_at(slot.data(), std::forward<Args>(args)...);
  }

  /// @brief Reference to the backing storage object itself. Const through a const tape.
  ///
  /// Bypasses the cursor and exposes the whole buffer, including unclaimed slots. Use it for
  /// storage-level operations, not for reading claimed contents.
  [[nodiscard]] auto& storage(this auto&& self) noexcept
  {
    return self.mStorage;
  }

private:
  /// The backing buffer that owns the slots. Its element count is the tape's fixed capacity.
  Storage mStorage;

  /// The cursor: the number of slots claimed so far, and the offset of the next free slot. Advanced
  /// atomically by claim and emplace, so concurrent reservations stay disjoint without a lock;
  /// moved back only by rewind. Accessed with relaxed ordering, so it orders the reservation but
  /// does NOT publish the claimed slots' contents.
  std::atomic<size_type> mCursor{0};
};

} // namespace nioc::containers

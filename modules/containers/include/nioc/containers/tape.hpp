////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <atomic>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <memory>
#include <ranges>
#include <span>
#include <type_traits>
#include <utility>

namespace nioc::containers
{

/// @brief A fixed-capacity, append-only container that fills a contiguous storage front to back.
///
/// Adapts a fixed-extent contiguous storage (such as @ref MmapArray, `std::array`, or a pre-sized
/// `std::vector`) into an append-only container, the way `std::stack` adapts a container. A cursor
/// marks how much has been written; @ref claim and @ref emplace reserve the next slots at the tail
/// until the storage is full. @ref begin "begin()"..@ref end "end()" iterate the written region
/// `[0, size())`; the whole storage, including the unwritten tail, stays reachable through
/// @ref storage.
///
/// Reservations are atomic, so several threads may append at once. A reservation that does not fit
/// returns an empty span (@ref claim) or a null pointer (@ref emplace); the caller then moves on to
/// a fresh tape. Iterating the written region while other threads are appending sees a racing
/// snapshot, so iterate only once appending has stopped.
///
/// @tparam Storage A fixed-extent contiguous, sized range whose element type is trivially copyable.
template<typename Storage>
  requires std::ranges::contiguous_range<Storage> and std::ranges::sized_range<Storage> and
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

  /// @brief Constructs the storage in place from @p args; the tape starts empty.
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

  /// @brief Returns a pointer to the first element.
  [[nodiscard]] auto data(this auto&& self) noexcept
  {
    return std::ranges::data(self.mStorage);
  }

  /// @brief Returns a reference to the written element at @p index.
  ///
  /// @param index Element position, less than @ref size. Out-of-range access is undefined.
  [[nodiscard]] decltype(auto) operator[](this auto&& self, const size_type index) noexcept
  {
    return self.data()[index];  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  }

  /// @brief Returns an iterator to the first element.
  [[nodiscard]] auto begin(this auto&& self) noexcept
  {
    return self.data();
  }

  /// @brief Returns an iterator one past the last written element.
  [[nodiscard]] auto end(this auto&& self) noexcept
  {
    return self.data() + self.size(); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  }

  /// @brief Returns a const iterator to the first element.
  [[nodiscard]] const_iterator cbegin() const noexcept
  {
    return begin();
  }

  /// @brief Returns a const iterator one past the last written element.
  [[nodiscard]] const_iterator cend() const noexcept
  {
    return end();
  }

  /// @brief Returns whether nothing has been written yet.
  [[nodiscard]] bool empty() const noexcept
  {
    return size() == 0;
  }

  /// @brief Returns whether the tape has no room left.
  [[nodiscard]] bool full() const noexcept
  {
    return size() == capacity();
  }

  /// @brief Returns the number of written elements.
  [[nodiscard]] size_type size() const noexcept
  {
    return mCursor.load(std::memory_order_relaxed);
  }

  /// @brief Returns the maximum number of elements the storage holds.
  [[nodiscard]] size_type capacity() const noexcept
  {
    return std::ranges::size(mStorage);
  }

  /// @brief Trims the storage to the written size, releasing the unwritten tail.
  ///
  /// Resizes the storage down to @ref size elements, the way `std::vector::shrink_to_fit` trims
  /// spare capacity. Storage that keeps a persistent mapping (such as @ref MmapArray) trims its
  /// backing file but leaves the mapping in place, so pointers and spans into the written region
  /// stay valid. Available only when the storage can be resized. Not safe to call concurrently with
  /// @ref claim or @ref emplace.
  void shrink_to_fit() noexcept
    requires requires(Storage& storage, size_type count) { storage.resize(count); }
  {
    mStorage.resize(size());
  }

  /// @brief Reserves @p count contiguous slots at the tail.
  ///
  /// @param count Number of slots to reserve; at least one.
  ///
  /// @return A span over the reserved slots, or an empty span if they do not fit.
  [[nodiscard]] std::span<value_type> claim(const size_type count = 1) noexcept
  {
    assert(count != 0); // an empty span means "full", so a zero-slot claim would be ambiguous.

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

  /// @brief Gives the unused tail of a claimed slot back to the tape.
  ///
  /// Winds the cursor back so the next @ref claim reuses the space beyond the first @p usedCount
  /// elements of @p slot — but only while @p slot is still at the tail, i.e. nothing has been
  /// claimed since. Once a later claim has moved the cursor on, the tail is stranded and this does
  /// nothing. Single-producer use only: not safe to call concurrently with @ref claim or
  /// @ref emplace.
  ///
  /// @param slot A span previously returned by @ref claim from this tape.
  ///
  /// @param usedCount Number of leading elements of @p slot to keep; at most @c slot.size().
  void rewind(const std::span<value_type> slot, const size_type usedCount) noexcept
  {
    assert(usedCount <= slot.size());

    const auto slotStart = static_cast<size_type>(std::distance(data(), slot.data()));
    auto slotEnd = slotStart + slot.size();
    mCursor.compare_exchange_strong(slotEnd, slotStart + usedCount, std::memory_order_relaxed);
  }

  /// @brief Constructs one element in place at the tail from @p args.
  ///
  /// @return A pointer to the new element, or null if the tape is full.
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

  /// @brief Returns the underlying storage, including the unwritten tail.
  ///
  /// @note Resizing the storage through this reference invalidates every pointer, span, and
  /// iterator the tape has handed out.
  [[nodiscard]] auto& storage(this auto&& self) noexcept
  {
    return self.mStorage;
  }

private:
  Storage mStorage;
  std::atomic<size_type> mCursor{0};
};

} // namespace nioc::containers

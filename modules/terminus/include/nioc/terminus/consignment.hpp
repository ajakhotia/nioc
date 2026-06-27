////////////////////////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2026.
//  Project  : niocRosBridge
//  Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <atomic>
#include <nioc/chronicle/crate.hpp>

namespace nioc::terminus
{

/// @brief A move-only handle that owns one crate of work and counts itself against a shared in-flight
/// counter for its whole lifetime.
///
/// While it lives it adds one to the counter; on destruction it removes one and wakes every waiter
/// (such as `Port::awaitQuiescence`) when the count reaches zero. Move transfers the count, leaving
/// the moved-from handle disengaged. Own and destroy each consignment on one thread; the counter may
/// be shared across threads and must outlive every consignment built from it.
///
/// @see Port::awaitQuiescence, chronicle::Crate
class Consignment
{
public:
  /// @brief Take ownership of @p crate and add one to @p counter, which must outlive this handle.
  Consignment(chronicle::Crate crate, std::atomic_uint32_t& counter);

  Consignment(const Consignment&) = delete;

  /// @brief Steal @p other's crate and in-flight count, leaving it disengaged.
  Consignment(Consignment&& other) noexcept;

  Consignment& operator=(const Consignment&) = delete;

  /// @brief Release this handle's count, then steal @p other's. Self-assignment is a no-op.
  Consignment& operator=(Consignment&& other) noexcept;

  /// @brief Remove this handle's count, unless moved from, waking waiters when it reaches zero.
  ~Consignment();

  /// @brief Return the carried crate, valid only while this handle lives.
  [[nodiscard]] const chronicle::Crate& crate() const noexcept;

private:
  /// The crate of work this handle carries.
  chronicle::Crate mCrate;

  /// The shared in-flight counter, or null once moved from.
  std::atomic_uint32_t* mCounter;
};

} // namespace nioc::terminus

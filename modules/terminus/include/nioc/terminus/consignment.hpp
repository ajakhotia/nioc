////////////////////////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2026.
//  Project  : niocRosBridge
//  Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <atomic>
#include <nioc/chronicle/crate.hpp>
#include <nioc/common/raiiToken.hpp>
#include <utility>

namespace nioc::terminus
{

/// @brief A move-only handle that owns one crate of work and counts itself as in-flight for its
/// whole lifetime against a shared counter.
///
/// Models one unit of work handed to a subscriber. While the consignment lives, it adds one to a
/// shared in-flight counter, and on destruction it removes one and wakes waiters when the count
/// reaches zero. A consumer (such as `Port::awaitQuiescence`) can wait on that counter to learn
/// when no consignments are left to process, so keeping one alive delays quiescence. Read the
/// crate, then let the consignment leave scope to mark the work done.
///
/// Copying is disabled; move transfers the ownership through the token member. Not internally
/// synchronized: own and destroy each consignment on a single thread, though many consignments may
/// share one counter across threads.
///
/// The referenced counter must outlive every consignment built from it.
///
/// Example:
///
///     // Delivered to a subscriber callback by value.
///     void onMessage(Consignment consignment)
///     {
///       read(consignment.crate());
///     } // consignment leaves the scope here, marking the work done.
///
/// @see Port::awaitQuiescence, chronicle::Crate, common::RaiiToken
class Consignment
{
public:
  /// @brief Take ownership of a @p crate and add one to the @p counter now, removing it on
  /// destruction.
  ///
  /// Destroying the consignment that drops the count to zero notifies all waiters on the counter.
  /// Does not throw.
  ///
  /// @param crate The unit of work this consignment owns and exposes through crate().
  ///
  /// @param counter Held by reference; must outlive this consignment.
  Consignment(chronicle::Crate crate, std::atomic_uint32_t& counter):
    mCrate{std::move(crate)},
    mToken{Acquire{counter}, Release{counter}}
  {
  }

  /// @brief Return the carried crate.
  ///
  /// The reference is valid only while this consignment lives.
  [[nodiscard]] const chronicle::Crate& crate() const noexcept
  {
    return mCrate;
  }

private:
  /// @brief The acquire action run once at construction: adds one to the shared in-flight counter.
  ///
  /// Paired with Release through the token member to bracket the consignment's lifetime.
  struct Acquire
  {
    /// The shared in-flight counter, held by reference, must outlive this functor.
    std::atomic_uint32_t& mConsignmentCounter;

    /// @brief Add one to the counter. Does not throw.
    void operator()() const noexcept
    {
      mConsignmentCounter.fetch_add(1, std::memory_order_relaxed);
    }
  };

  /// @brief The release action run once at destruction: removes one from the shared in-flight
  /// counter and wakes all waiters when the count reaches zero.
  ///
  /// Paired with Acquire through the token member to bracket the consignment's lifetime.
  struct Release
  {
    /// The shared in-flight counter, held by a reference, must outlive this functor.
    std::atomic_uint32_t& mConsignmentCounter;

    /// @brief Remove one from the counter and, if it was the last in-flight unit, notify every
    /// waiter blocked on the counter.
    void operator()() const noexcept
    {
      if(mConsignmentCounter.fetch_sub(1, std::memory_order_release) == 1)
      {
        mConsignmentCounter.notify_all();
      }
    }
  };

  /// The crate of work this consignment carries, exposed through crate().
  chronicle::Crate mCrate;

  /// Bracket that runs Acquire now and Release exactly once when this consignment is destroyed or
  /// moved-from, keeping the in-flight count consistent across moves.
  common::RaiiToken<Release> mToken;
};

} // namespace nioc::terminus

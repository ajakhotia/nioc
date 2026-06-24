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

/// @brief Holds one delivered frame and counts as in flight while it lives.
///
/// The shared in-flight counter must outlive every consignment built against it.
class Consignment
{
public:
  /// @param crate Frame to carry to a subscriber.
  ///
  /// @param counter Shared in-flight counter; must outlive this consignment.
  Consignment(chronicle::Crate crate, std::atomic_uint32_t& counter):
    mCrate{std::move(crate)},
    mToken{Acquire{counter}, Release{counter}}
  {
  }

  /// @brief Returns the carried frame.
  [[nodiscard]] const chronicle::Crate& crate() const noexcept
  {
    return mCrate;
  }

private:
  /// @brief Guard entry action: raises the in-flight counter.
  struct Acquire
  {
    std::atomic_uint32_t& mConsignmentCounter;

    void operator()() const noexcept
    {
      mConsignmentCounter.fetch_add(1);
    }
  };

  /// @brief Guard exit action: lowers the in-flight counter and wakes waiters at zero.
  struct Release
  {
    std::atomic_uint32_t& mConsignmentCounter;

    void operator()() const noexcept
    {
      if(mConsignmentCounter.fetch_sub(1, std::memory_order_acq_rel) == 1)
      {
        mConsignmentCounter.notify_all();
      }
    }
  };

  chronicle::Crate mCrate;
  common::RaiiToken<Release> mToken;
};

} // namespace nioc::terminus

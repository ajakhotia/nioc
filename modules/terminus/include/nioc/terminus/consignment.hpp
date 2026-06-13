////////////////////////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2026.
//  Project  : niocRosBridge
//  Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "msgBase.hpp"
#include <atomic>
#include <nioc/common/raiiToken.hpp>

namespace nioc::terminus
{

/// @brief Holds one message and counts itself as in flight while it lives.
///
/// Construction adds one to a shared counter; destruction subtracts one. The counter always equals
/// the number of live consignments built against it. The counter must outlive every consignment
/// built against it.
class Consignment
{
public:
  /// @param msgBasePtr Message to hold.
  ///
  /// @param counter In-flight counter. Raised now, lowered on destruction. Must outlive this
  /// consignment.
  Consignment(ConstMsgBasePtr msgBasePtr, std::atomic_uint32_t& counter):
    mMsgBasePtr{std::move(msgBasePtr)},
    mToken{Acquire{counter}, Release{counter}}
  {
  }

  /// @brief Returns the held message.
  [[nodiscard]] const ConstMsgBasePtr& msg() const noexcept
  {
    return mMsgBasePtr;
  }

private:
  /// @brief Guard entry action: raises the in-flight counter.
  struct Acquire
  {
    /// @brief Counter to raise. Must outlive this action.
    std::atomic_uint32_t& mConsignmentCounter;

    /// @brief Adds one to the counter.
    void operator()() const noexcept
    {
      mConsignmentCounter.fetch_add(1);
    }
  };

  /// @brief Guard exit action: lowers the in-flight counter.
  struct Release
  {
    /// @brief Counter to lower. Must outlive this action.
    std::atomic_uint32_t& mConsignmentCounter;

    /// @brief Subtracts one. Wakes threads waiting on the counter when it hits zero.
    void operator()() const noexcept
    {
      if(mConsignmentCounter.fetch_sub(1, std::memory_order_acq_rel) == 1)
      {
        mConsignmentCounter.notify_all();
      }
    }
  };

  ConstMsgBasePtr mMsgBasePtr;
  common::RaiiToken<Release> mToken;
};

} // namespace nioc::terminus

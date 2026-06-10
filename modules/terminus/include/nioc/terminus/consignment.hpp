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

/// @brief A message bundled with a token that keeps it counted as in flight for as long as it
/// lives.
///
/// A consignment carries one message and, alongside it, an RAII guard over a shared
/// `std::atomic_uint32_t`. The guard raises the counter by one when the consignment is constructed
/// and lowers it by one when the consignment is destroyed, so the counter always equals the number
/// of live consignments built against it.
///
/// The referenced counter must outlive every consignment built against it.
class Consignment
{
public:
  /// @brief Builds a consignment over @p msgBasePtr and @p counter.
  ///
  /// @param msgBasePtr Message the consignment carries.
  ///
  /// @param counter In-flight counter raised on construction and lowered on destruction; must
  /// outlive the consignment.
  Consignment(ConstMsgBasePtr msgBasePtr, std::atomic_uint32_t& counter):
    mMsgBasePtr{std::move(msgBasePtr)},
    mToken{Acquire{counter}, Release{counter}}
  {
  }

  /// @brief Returns the message this consignment carries.
  [[nodiscard]] const ConstMsgBasePtr& msg() const noexcept
  {
    return mMsgBasePtr;
  }

private:
  /// @brief Function object that raises the in-flight counter; serves as the guard's entry action.
  struct Acquire
  {
    /// @brief Counter raised on each invocation. Must outlive this action.
    std::atomic_uint32_t& mConsignmentCounter;

    /// @brief Adds one to the counter.
    void operator()() const noexcept
    {
      mConsignmentCounter.fetch_add(1);
    }
  };

  /// @brief Function object that lowers the in-flight counter; serves as the guard's exit action.
  struct Release
  {
    /// @brief Counter lowered on each invocation. Must outlive this action.
    std::atomic_uint32_t& mConsignmentCounter;

    /// @brief Subtracts one from the counter and wakes threads waiting on it once it reaches zero.
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

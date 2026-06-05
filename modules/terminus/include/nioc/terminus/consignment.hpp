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

/// @brief A message paired with a token that counts it as in flight while it lives.
///
/// Construction increments a shared counter through @ref Acquire; destruction decrements it through
/// @ref Release. The counter therefore reports how many consignments are still alive, letting a
/// producer track delivery without waiting on the consumer.
struct Consignment
{
  /// @brief Increments the in-flight counter. Used as the token's entry action.
  struct Acquire
  {
    /// @brief Counter incremented on each invocation.
    std::atomic_uint32_t* mConsignmentCounterPtr;

    /// @brief Adds one to the counter.
    void operator()() noexcept
    {
      counter().fetch_add(1);
    }

  private:
    [[nodiscard]] const std::atomic_uint32_t& counter() const noexcept
    {
      return *mConsignmentCounterPtr;
    }

    // NOLINTNEXTLINE(readability-make-member-function-const)
    [[nodiscard]] std::atomic_uint32_t& counter() noexcept
    {
      return *mConsignmentCounterPtr;
    }
  };

  /// @brief Decrements the in-flight counter. Used as the token's exit action.
  struct Release
  {
    /// @brief Counter decremented on each invocation.
    std::atomic_uint32_t* mConsignmentCounterPtr;

    /// @brief Subtracts one from the counter.
    void operator()() noexcept
    {
      counter().fetch_sub(1);
    }

  private:
    [[nodiscard]] const std::atomic_uint32_t& counter() const noexcept
    {
      return *mConsignmentCounterPtr;
    }

    // NOLINTNEXTLINE(readability-make-member-function-const)
    [[nodiscard]] std::atomic_uint32_t& counter() noexcept
    {
      return *mConsignmentCounterPtr;
    }
  };

  /// @brief The carried message.
  ConstMsgBasePtr mMsgBasePtr;

  /// @brief Holds the counter decrement that runs when this consignment is destroyed.
  common::RaiiToken<Release> mToken;

  /// @brief Bundles a message and counts it in flight.
  ///
  /// Runs @p acquire now and stores @p release to run at destruction.
  ///
  /// @param msgBasePtr Message to carry.
  /// @param acquire    Action run now; increments the in-flight counter.
  /// @param release    Action stored and run at destruction; decrements the in-flight counter.
  Consignment(ConstMsgBasePtr msgBasePtr, Acquire acquire, const Release release):
    mMsgBasePtr(std::move(msgBasePtr)),
    mToken(acquire, release)
  {
  }
};

} // namespace nioc::terminus

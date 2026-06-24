////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "draft.hpp"
#include "message.hpp"
#include "port.hpp"
#include <chrono>
#include <cstddef>
#include <nioc/chronicle/channel.hpp>
#include <utility>

namespace nioc::terminus
{

/// @brief A producer's handle for one topic.
///
/// Mint one with @ref Port::publisher at construction and keep it. @ref draft returns a @ref Draft
/// to build the next message into; @ref publish delivers the message to subscribers and records it.
///
/// @tparam Schema_ Cap'n Proto schema of the messages.
template<typename Schema_>
class Publisher
{
public:
  using Schema = Schema_;

  /// @brief Returns a @ref Draft to build the next message into.
  ///
  /// @param reservationOverride Bytes to reserve, or 0 to let the publisher size the reservation
  /// from the last frame.
  [[nodiscard]] Draft<Schema> draft(const std::size_t reservationOverride = 0U)
  {
    const auto arrivalTimestamp = std::chrono::steady_clock::now();
    const auto sequenceNumber = ++mSequenceNumber;
    const auto size = reservationOverride == 0U
                          ? static_cast<std::size_t>(mSizeEstimate * kHysteresis) + 1
                          : reservationOverride;

    return Draft<Schema>{mChannel.reserve(size), arrivalTimestamp, sequenceNumber};
  }

  /// @brief Delivers an already-frozen @p message to subscribers.
  ///
  /// @param message Message to publish.
  void publish(const Message<Schema>& message)
  {
    updateSizeEstimate(message.crate().span().size());
    mPort.deliver(mChannel.id(), message.crate());
  }

private:
  friend class Port;

  /// Headroom over the last frame size, so small growth does not overflow the next reservation.
  static constexpr double kHysteresis = 1.025;

  /// Reservation size before any message has been published.
  static constexpr std::size_t kInitialReservationSize = 256;

  Port& mPort;
  chronicle::Channel& mChannel;
  std::size_t mSizeEstimate{kInitialReservationSize};
  std::uint64_t mSequenceNumber{0U};

  /// @brief Binds the publisher to its port and channel.
  ///
  /// @param port Port that delivers published frames to subscribers.
  ///
  /// @param channel Channel that records this topic and mints its reservations.
  Publisher(Port& port, chronicle::Channel& channel): mPort{port}, mChannel{channel} {}

  /// @brief Sizes the next reservation from @p framedSize, plus headroom for small growth.
  void updateSizeEstimate(const std::size_t size) noexcept
  {
    mSizeEstimate = std::max(mSizeEstimate, size);
  }
};

} // namespace nioc::terminus

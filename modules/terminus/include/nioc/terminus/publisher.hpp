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

/// @brief The write end of one topic: emits messages of a fixed Cap'n Proto schema onto a single
/// recorded channel.
///
/// Call `draft` to start a message, fill its payload, then seal it into a `Message` and pass it to
/// `publish`. Each draft is stamped with the next sequence number and an arrival timestamp, and the
/// reservation size self-tunes toward the largest payload seen so far.
///
/// Example:
///
///     auto publisher = port.publisher<MySchema>("my/topic");
///     auto d = publisher.draft();
///     d.builder().setValue(42);
///     Message<MySchema> msg = std::move(d); // seal
///     publisher.publish(msg);
///
/// Get one from `Port::publisher`; the constructor is private. Holds its `Port` and channel by
/// reference, so it must not outlive either. Single-threaded: do not share a Publisher across
/// threads.
///
/// @tparam Schema_ The Cap'n Proto schema of the published payload.
///
/// @see Port::publisher, Draft, Message
template<typename Schema_>
class Publisher
{
public:
  using Schema = Schema_;

  /// @brief Begin a new outgoing message, returning a `Draft` to populate then seal.
  ///
  /// The draft comes pre-stamped with the next sequence number and the current steady-clock arrival
  /// timestamp. Writing past the reservation is allowed; the overflow is reconciled when the draft
  /// is sealed.
  ///
  /// @param reservationOverride Bytes to reserve. 0 (the default) sizes the reservation from the
  /// running estimate; any non-zero value requests exactly that many bytes, which the channel
  /// rounds up to a word boundary.
  ///
  /// @see Draft
  [[nodiscard]] Draft<Schema> draft(const std::size_t reservationOverride = 0U)
  {
    const auto arrivalTimestamp = std::chrono::steady_clock::now();
    const auto sequenceNumber = ++mSequenceNumber;
    const auto size = reservationOverride == 0U
                          ? static_cast<std::size_t>(mSizeEstimate * kHysteresis) + 1
                          : reservationOverride;

    return Draft<Schema>{mChannel.reserve(size), arrivalTimestamp, sequenceNumber};
  }

  /// @brief Fan a sealed message out to this channel's subscribers, synchronously on the caller's
  /// thread.
  ///
  /// The message's bytes were already recorded to the channel when its draft was sealed; this only
  /// delivers to subscribers and folds the message size into the estimate that future `draft` calls
  /// use to size reservations.
  ///
  /// @param message Must have been built by this Publisher's `draft`, so its sequence numbering
  /// stays consistent.
  ///
  /// @see draft, Port::deliver
  void publish(const Message<Schema>& message)
  {
    updateSizeEstimate(message.crate().span().size());
    mPort.deliver(mChannel.id(), message.crate());
  }

private:
  friend class Port;

  /// The growth factor applied to the running size estimate when auto-sizing a reservation, so the
  /// reserved buffer sits a little above the largest payload seen and rarely overflows.
  static constexpr double kHysteresis = 1.025;

  /// The starting value of the running size estimate, used until the first message sets a larger
  /// one.
  static constexpr std::size_t kInitialReservationSize = 256;

  /// The port that fans published messages out to subscribers. Outlives this Publisher.
  Port& mPort;

  /// The channel this Publisher records its messages onto. Outlives this Publisher.
  chronicle::Channel& mChannel;

  /// The largest payload size seen so far, in bytes, used to auto-size the next draft's
  /// reservation.
  std::size_t mSizeEstimate{kInitialReservationSize};

  /// The sequence number of the most recently drafted message; pre-incremented per draft.
  std::uint64_t mSequenceNumber{0U};

  /// @brief Construct a Publisher bound to a port and channel; called only by `Port::publisher`.
  ///
  /// @param port The port that fans published messages out to subscribers. Must outlive this
  /// Publisher; held by reference.
  ///
  /// @param channel The recorded channel this Publisher reserves and writes into. Must outlive
  /// this Publisher; held by reference.
  Publisher(Port& port, chronicle::Channel& channel): mPort{port}, mChannel{channel} {}

  /// @brief Raise the running size estimate to the given payload size if it is larger, leaving it
  /// unchanged otherwise. Called by `publish` after each message.
  ///
  /// @param size A published payload size in bytes.
  void updateSizeEstimate(const std::size_t size) noexcept
  {
    mSizeEstimate = std::max(mSizeEstimate, size);
  }
};

} // namespace nioc::terminus

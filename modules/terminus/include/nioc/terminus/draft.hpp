////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "arenaMessageBuilder.hpp"
#include "message.hpp"
#include <chrono>
#include <nioc/chronicle/crate.hpp>
#include <nioc/chronicle/reservation.hpp>
#include <nioc/terminus/idl/envelope.capnp.h>
#include <utility>

namespace nioc::terminus
{

template<typename Schema>
class Publisher;

/// @brief Records the message built in @p builder as a @ref chronicle::Crate on @p reservation's
/// channel.
///
/// A single-segment build still living in @p reservation's arena is recorded in place; a build that
/// overflowed onto the heap is re-rooted into a single segment in a fresh slot. Either way the
/// recorded frame is one segment behind a flat-array header. @p reservation is spent by this call.
///
/// @param builder The arena builder holding the finished message.
///
/// @param reservation The reservation @p builder built into.
[[nodiscard]] chronicle::Crate flattenDraft(
    ArenaMessageBuilder& builder,
    chronicle::Reservation reservation);

/// @brief The read-write form of a message.
///
/// Build the payload through @ref builder, then convert the draft to a @ref Message — which
/// consumes the draft, records its frame, and opens it for reading. A draft converted without @ref
/// builder ever called yields a gap (a null payload). Mint one with @ref Publisher::draft.
///
/// @tparam Schema_ Cap'n Proto schema of the payload.
template<typename Schema_>
class Draft
{
public:
  using Schema = Schema_;

  Draft(const Draft&) = delete;

  Draft(Draft&&) noexcept = default;

  ~Draft() = default;

  Draft& operator=(const Draft&) = delete;

  Draft& operator=(Draft&&) noexcept = delete;

  /// @brief Returns a builder for the payload.
  ///
  /// Calling this makes the message real rather than a gap.
  [[nodiscard]] Schema::Builder builder()
  {
    return mBuilder.getRoot<Envelope<Schema>>().getMessage();
  }

  /// @brief Records this draft's frame and opens it as a @ref Message, consuming the draft.
  ///
  /// Implicit by necessity: @ref Message is immovable, so this conversion is reachable only through
  /// copy-initialization (`Message<Schema> message = std::move(draft)`); an explicit operator would
  /// satisfy neither that nor a `static_cast`, both of which would need Message's deleted move.
  // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
  operator Message<Schema>() &&
  {
    return Message<Schema>{flattenDraft(mBuilder, std::move(mReservation))};
  }

private:
  friend class Publisher<Schema>;

  chronicle::Reservation mReservation;
  ArenaMessageBuilder mBuilder;

  /// @brief Creates an empty draft that builds into a @p reservation and records onto its channel.
  ///
  /// @param reservation Region to build into; carries the channel that records the frozen frame.
  ///
  /// @param arrivalTimestamp Moment the data was perceived.
  ///
  /// @param sequenceNumber Producer-assigned counter.
  Draft(
      chronicle::Reservation reservation,
      const std::chrono::steady_clock::time_point arrivalTimestamp,
      const std::uint64_t sequenceNumber):
    mReservation{std::move(reservation)},
    mBuilder{mReservation.span()}
  {
    const auto nsSinceEpoch = std::chrono::duration_cast<std::chrono::nanoseconds>(
        arrivalTimestamp.time_since_epoch());

    auto envelope = mBuilder.initRoot<Envelope<Schema>>();
    envelope.setArrivalTimestamp(nsSinceEpoch.count());
    envelope.setSequenceNumber(sequenceNumber);
  }
};


} // namespace nioc::terminus

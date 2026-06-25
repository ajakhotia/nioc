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

/// @brief Commit a built message into its reservation and return it as a single-segment crate.
///
/// Reads the message held in @p builder and commits @p reservation around it. When the message fit
/// inside the reservation's arena, its bytes are committed in place with no copy. When it
/// overflowed onto the heap, it is first re-rooted into one segment and that segment is framed back
/// into the reservation, which may resize the reservation. You do not call this directly; seal a
/// `Draft` instead.
///
/// @param builder The message source. Read and then left spent; do not reuse it.
///
/// @param reservation Consumed (committed). Its span must be the bytes @p builder was rooted in.
///
/// @return The committed crate, always a single-segment frame.
///
/// @throws std::logic_error If an overflowed message does not collapse back to a single segment.
///
/// @see Draft
[[nodiscard]] chronicle::Crate flattenDraft(
    ArenaMessageBuilder& builder,
    chronicle::Reservation reservation);

/// @brief The writer's view of a not-yet-published message, built directly in the channel's bytes.
///
/// A Draft owns a reservation in the chronicle channel plus a Cap'n Proto builder rooted in that
/// reservation's bytes, so the payload is written straight into the destination buffer with no
/// staging copy. Get one from `Publisher::draft`, populate the payload through `builder()`, then
/// seal it into a `Message<Schema>` to publish.
///
/// Example:
///
///     auto draft = publisher.draft();
///     draft.builder().setValue(42);
///     Message<Schema> msg = std::move(draft); // seal and publish
///
/// Single-use and single-threaded: build once, then seal exactly once via the rvalue `Message`
/// conversion (which consumes the Draft). The envelope's arrival timestamp and sequence number are
/// stamped at construction by the Publisher. Not copyable and not move-assignable; it is move-
/// constructible. Its reservation must not outlive the owning channel.
///
/// @tparam Schema_ The Cap'n Proto schema of the carried payload.
///
/// @see Publisher, Message, flattenDraft
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

  /// @brief Get the builder for the payload, writing directly into the reservation's bytes.
  ///
  /// Writing past the reservation is allowed: the overflow spills to the heap and is reconciled
  /// when the Draft is sealed. The returned builder is valid only until this Draft is sealed or
  /// destroyed.
  [[nodiscard]] Schema::Builder builder()
  {
    return mBuilder.getRoot<Envelope<Schema>>().getMessage();
  }

  /// @brief Seal this draft into a publishable `Message`, consuming the Draft.
  ///
  /// Implicit and rvalue-only: write `Message<Schema> m = std::move(draft);`. The Draft is spent by
  /// the conversion. May resize the underlying reservation if the payload overflowed the arena.
  ///
  /// @throws std::logic_error Propagated from `flattenDraft` when an overflowed payload does not
  /// collapse to a single segment.
  ///
  /// @see flattenDraft
  // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
  operator Message<Schema>() &&
  {
    return Message<Schema>{flattenDraft(mBuilder, std::move(mReservation))};
  }

private:
  friend class Publisher<Schema>;

  /// @brief The owned channel reservation whose bytes back the builder; consumed when sealed.
  chronicle::Reservation mReservation;

  /// @brief The Cap'n Proto builder rooted in @ref mReservation's span, overflowing to the heap if
  /// the payload outgrows it.
  ArenaMessageBuilder mBuilder;

  /// @brief Construct over @p reservation, stamping the envelope header before any payload is
  /// added.
  ///
  /// Callable only by the owning Publisher.
  ///
  /// @param reservation Taken by move; its span becomes the bytes the builder is rooted in.
  ///
  /// @param arrivalTimestamp Stored as nanoseconds since this steady_clock's process-local epoch.
  ///
  /// @param sequenceNumber The producer-assigned monotonic counter for this message.
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

////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "arenaMessageBuilder.hpp"
#include <capnp/serialize.h>
#include <chrono>
#include <nioc/chronicle/crate.hpp>
#include <nioc/terminus/idl/envelope.capnp.h>
#include <utility>

namespace nioc::terminus
{

/// @brief The read-only view of a message: its payload and envelope metadata.
///
/// Construct it over a @ref chronicle::Crate and read the payload with @ref reader. A message
/// cannot be copied or moved; to pass it on, move its @ref crate and construct a new message from
/// it.
///
/// @tparam Schema_ Cap'n Proto schema of the payload.
template<typename Schema_>
class Message
{
public:
  using Schema = Schema_;

  /// @brief Opens the frame held in @p crate for reading.
  /// @param crate Crate holding one frame.
  explicit Message(chronicle::Crate crate):
    mCrate{std::move(crate)},
    mReader{asWords(mCrate.span())},
    mEnvelope{mReader.getRoot<Envelope<Schema>>()}
  {
  }

  Message(const Message&) = delete;

  /// @brief Moves @p other onto its relocated backing, re-deriving the reader and envelope.
  ///
  /// They cannot be moved member-wise: the reader's arena and the envelope both hold pointers into
  /// the reader, which a member-wise move would leave dangling at the moved-from @p other.
  /// Rebuilding them over the moved (address-stable) backing is sound.
  Message(Message&& other) noexcept: Message(std::move(other.mCrate)) {}

  ~Message() noexcept = default;

  Message& operator=(const Message&) = delete;

  Message& operator=(Message&&) = delete;

  /// @brief Returns a reader for the payload.
  ///
  /// A gap reads as a default payload, so check @ref isGap first.
  [[nodiscard]] Schema::Reader reader() const
  {
    return mEnvelope.getMessage();
  }

  /// @brief Returns the steady-clock arrival time stamped when the message was built.
  ///
  /// Only inter-message deltas are meaningful on replay; the absolute value is process-local.
  [[nodiscard]] std::chrono::steady_clock::time_point arrivalTimestamp() const
  {
    return std::chrono::steady_clock::time_point{
        std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            std::chrono::nanoseconds{mEnvelope.getArrivalTimestamp()})};
  }

  /// @brief Returns the producer-assigned sequence number; 0 when unassigned.
  [[nodiscard]] std::uint64_t sequenceNumber() const
  {
    return mEnvelope.getSequenceNumber();
  }

  /// @brief Whether this envelope stands in for an expected-but-absent message.
  [[nodiscard]] bool isGap() const
  {
    return not mEnvelope.hasMessage();
  }

  /// @brief Returns the crate holding the recorded frame.
  [[nodiscard]] const chronicle::Crate& crate() const noexcept
  {
    return mCrate;
  }

private:
  chronicle::Crate mCrate;
  capnp::FlatArrayMessageReader mReader;
  Envelope<Schema>::Reader mEnvelope;
};

} // namespace nioc::terminus

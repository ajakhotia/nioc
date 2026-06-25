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

/// @brief The reader's view of one sealed envelope, decoded in place from a crate's bytes with no
/// parsing or copying.
///
/// A Message owns the `chronicle::Crate` that keeps the encoded bytes alive and roots a zero-copy
/// Cap'n Proto reader at the envelope inside them. Read the payload, arrival timestamp, and
/// sequence number directly through the accessors. A Message may carry no payload, which marks a
/// gap in the sequence; check @ref isGap before calling @ref reader.
///
/// Example:
///
///     // Seal a Draft to publish, or wrap a crate received off a channel.
///     Message<MySchema> msg = std::move(draft);
///     if(not msg.isGap())
///     {
///       MySchema::Reader payload = msg.reader();
///       // ... read payload ...
///     }
///
/// Not copyable; move-construction transfers the crate and re-roots the reader, leaving the source
/// empty. Every reader handed out by @ref reader stays valid only while this Message (or a
/// surviving copy of its @ref crate) lives. The crate must hold a single-segment frame, as
/// produced by `flattenDraft`; a multi-segment or unframed crate is undefined.
///
/// @tparam Schema_ The Cap'n Proto struct schema of the carried payload.
///
/// @see Draft, flattenDraft, chronicle::Crate
template<typename Schema_>
class Message
{
public:
  using Schema = Schema_;

  /// @brief Take ownership of a sealed crate and root a reader at its envelope.
  ///
  /// @param crate Single-segment frame holding the encoded envelope. Read in place and retained to
  /// keep the bytes alive.
  explicit Message(chronicle::Crate crate):
    mCrate{std::move(crate)},
    mReader{asWords(mCrate.span())},
    mEnvelope{mReader.getRoot<Envelope<Schema>>()}
  {
  }

  Message(const Message&) = delete;

  /// @brief Take over @p other's crate and root a fresh reader in it, leaving @p other empty.
  Message(Message&& other) noexcept: Message(std::move(other.mCrate)) {}

  ~Message() noexcept = default;

  Message& operator=(const Message&) = delete;

  Message& operator=(Message&&) = delete;

  /// @brief Return a reader over the carried payload.
  ///
  /// The reader is valid only while this Message lives. Reading a gap (see @ref isGap) yields a
  /// default-valued reader rather than throwing.
  [[nodiscard]] Schema::Reader reader() const
  {
    return mEnvelope.getMessage();
  }

  /// @brief Return the producer's `steady_clock` arrival instant, rebuilt from the envelope's
  /// stored nanoseconds.
  ///
  /// Meaningful only against clocks that share the producing process's epoch; do not compare it
  /// with instants from another process.
  [[nodiscard]] std::chrono::steady_clock::time_point arrivalTimestamp() const
  {
    return std::chrono::steady_clock::time_point{
        std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            std::chrono::nanoseconds{mEnvelope.getArrivalTimestamp()})};
  }

  /// @brief Return the producer-assigned monotonic counter, or 0 when unassigned.
  ///
  /// Compare values across consecutive messages to detect drops or reordering after the fact.
  [[nodiscard]] std::uint64_t sequenceNumber() const
  {
    return mEnvelope.getSequenceNumber();
  }

  /// @brief Report whether this Message carries no payload, marking a gap in the sequence.
  ///
  /// Call before @ref reader; a true result means there is nothing meaningful to read.
  [[nodiscard]] bool isGap() const
  {
    return not mEnvelope.hasMessage();
  }

  /// @brief Return the underlying frame; copy it to keep the bytes alive independently of this
  /// Message.
  [[nodiscard]] const chronicle::Crate& crate() const noexcept
  {
    return mCrate;
  }

private:
  /// Owns the single-segment frame whose bytes back the reader; copied out by @ref crate so callers
  /// can outlive this Message.
  chronicle::Crate mCrate;

  /// Zero-copy Cap'n Proto reader rooted in @ref mCrate's bytes. Stays valid only while those bytes
  /// live, so it must be re-rooted whenever the crate moves.
  capnp::FlatArrayMessageReader mReader;

  /// The decoded envelope read out of @ref mReader; backs every payload, timestamp, and sequence
  /// accessor.
  Envelope<Schema>::Reader mEnvelope;
};

} // namespace nioc::terminus

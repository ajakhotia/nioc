////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <capnp/serialize.h>
#include <chrono>
#include <cstdint>
#include <functional>
#include <nioc/chronicle/defines.hpp>
#include <nioc/chronicle/memoryCrate.hpp>
#include <nioc/chronicle/writer.hpp>
#include <nioc/terminus/idl/envelope.capnp.h>
#include <variant>

namespace nioc::terminus
{

/// @brief Reads a Cap'n Proto message from a MemoryCrate's bytes, without copying.
///
/// Obtain one through @ref Msg, not by constructing it directly.
class MMappedMessageReader final:
  public chronicle::MemoryCrate,
  public capnp::FlatArrayMessageReader
{
public:
  /// @brief Wraps a crate's bytes for reading.
  /// @param memoryCrate Crate holding one serialized Cap'n Proto message.
  explicit MMappedMessageReader(MemoryCrate memoryCrate);

  MMappedMessageReader(const MMappedMessageReader&) = delete;

  MMappedMessageReader(MMappedMessageReader&&) = delete;

  ~MMappedMessageReader() final = default;

  MMappedMessageReader& operator=(const MMappedMessageReader&) = delete;

  MMappedMessageReader& operator=(MMappedMessageReader&&) = delete;
};

/// @brief Identifier for a message type, taken from its Cap'n Proto schema.
struct MsgId
{
  std::uint64_t mValue;

  constexpr bool operator==(const MsgId&) const = default;
};

/// @brief Common base for typed messages; use the derived @ref Msg instead.
///
/// A message is either built (created empty, then filled in and written) or read (loaded from a
/// MemoryCrate). MsgBase holds the current state and gives its type identifier.
class MsgBase
{
public:
  /// @brief Holds the message in its built or read state.
  using Variant = std::variant<capnp::MallocMessageBuilder, MMappedMessageReader>;

protected:
  /// @brief Sets up an empty enveloped message and records its framing.
  ///
  /// The payload is left null - a gap - until @ref Msg::builder allocates it.
  ///
  /// @param arrivalTimestamp Steady-clock time the message was built.
  ///
  /// @param sequenceNumber Producer-assigned counter; 0 means unassigned.
  MsgBase(std::chrono::steady_clock::time_point arrivalTimestamp, std::uint64_t sequenceNumber);

  /// @brief Loads a message for reading from a MemoryCrate.
  /// @param memoryCrate Crate holding one serialized message.
  explicit MsgBase(chronicle::MemoryCrate memoryCrate);

public:
  MsgBase(const MsgBase&) = delete;

  MsgBase(MsgBase&&) noexcept = delete;

  // capnp's MallocMessageBuilder destructor is noexcept(false), so the defaulted destructor here
  // inherits that exception specification through the held variant.
  // NOLINTNEXTLINE(cppcoreguidelines-noexcept-destructor,performance-noexcept-destructor)
  virtual ~MsgBase() = default;

  MsgBase& operator=(const MsgBase&) = delete;

  MsgBase& operator=(MsgBase&&) noexcept = delete;

  /// @brief Returns this message type's identifier.
  [[nodiscard]] constexpr virtual MsgId msgId() const = 0;

  /// @brief Returns the steady-clock arrival time stamped when the message was built.
  ///
  /// Only inter-message deltas are meaningful on replay; the absolute value is process-local.
  [[nodiscard]] std::chrono::steady_clock::time_point arrivalTimestamp() const;

  /// @brief Returns the producer-assigned sequence number; 0 when unassigned.
  [[nodiscard]] std::uint64_t sequenceNumber() const;

  /// @brief Whether this envelope stands in for an expected-but-absent message.
  ///
  /// True when the payload was never built. A gap reads as a default payload through
  /// @ref Msg::reader, so check this first.
  [[nodiscard]] bool isGap() const;

protected:
  friend void write(
      const MsgBase& msgBase,
      chronicle::ChannelId channelId,
      chronicle::Writer& writer);

  /// @brief Returns the built/read state.
  [[nodiscard]] Variant& variant() noexcept;

  /// @brief Returns the built/read state.
  [[nodiscard]] const Variant& variant() const noexcept;

private:
  Variant mVariant;
};

using MsgBasePtr = std::shared_ptr<MsgBase>;
using ConstMsgBasePtr = std::shared_ptr<const MsgBase>;

using MsgBaseUPtr = std::unique_ptr<MsgBase>;
using ConstMsgBaseUPtr = std::unique_ptr<const MsgBase>;


/// @brief Serializes a built message and appends it to a chronicle on a given channel.
///
/// Use this when you already hold the channel. To resolve the channel from a topic, use the
/// @p topic overload instead.
///
/// @param msgBase Message to write; must be built, not opened for reading.
///
/// @param channelId Channel to append to.
///
/// @param writer Open chronicle writer that receives the message.
///
/// @throws std::bad_variant_access if @p msgBase was opened for reading.
void write(const MsgBase& msgBase, chronicle::ChannelId channelId, chronicle::Writer& writer);

/// @brief Serializes a built message and appends it to a chronicle on a topic's channel.
///
/// The channel comes from the message type and @p topic, so the same type on two topics goes to
/// two channels.
///
/// @param msgBase Message to write; must be built, not opened for reading.
///
/// @param topic Topic name.
///
/// @param writer Open chronicle writer that receives the message.
///
/// @throws std::bad_variant_access if @p msgBase was opened for reading.
void write(const MsgBase& msgBase, const std::string_view& topic, chronicle::Writer& writer);

/// @brief Computes the channel for a message type carried on a topic.
///
/// Combines the type and topic, so the same type on two topics maps to two channels. Equal inputs
/// always give the same channel.
///
/// @param msgId Message type identifier (see @ref Msg::kMsgId).
///
/// @param topic Topic name.
///
/// @return The channel for this type-and-topic pair.
chronicle::ChannelId makeChannelId(const MsgId& msgId, const std::string_view& topic);


} // namespace nioc::terminus

template<>
struct std::hash<nioc::terminus::MsgId>
{
  decltype(auto) operator()(const nioc::terminus::MsgId& msgId) const noexcept
  {
    return std::hash<std::uint64_t>{}(msgId.mValue);
  }
};

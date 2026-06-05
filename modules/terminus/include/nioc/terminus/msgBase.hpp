////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <capnp/serialize.h>
#include <cstdint>
#include <functional>
#include <nioc/chronicle/defines.hpp>
#include <nioc/chronicle/memoryCrate.hpp>
#include <nioc/chronicle/writer.hpp>
#include <variant>

namespace nioc::terminus
{

/// @brief Reads a Cap'n Proto message in place over a chronicle::MemoryCrate.
///
/// Exposes the bytes held by a MemoryCrate as a Cap'n Proto message reader, without copying them.
/// Obtain one through @ref Msg rather than constructing it directly.
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
/// A message is in one of two states: built (created empty, then filled in and written) or read
/// (loaded from an existing MemoryCrate). MsgBase holds whichever state applies and exposes the
/// type identifier shared by both.
class MsgBase
{
public:
  /// @brief Holds the message in either its built or read state.
  using Variant = std::variant<capnp::MallocMessageBuilder, MMappedMessageReader>;

protected:
  /// @brief Creates an empty message ready to be built.
  MsgBase();

  /// @brief Loads a message for reading from an existing MemoryCrate.
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

protected:
  friend void write(
      const MsgBase& msgBase,
      chronicle::ChannelId channelId,
      chronicle::Writer& writer);

  /// @brief Returns the underlying built/read state, used by @ref write.
  [[nodiscard]] Variant& variant() noexcept;

  /// @brief Returns the underlying built/read state of a const message, used by @ref write.
  [[nodiscard]] const Variant& variant() const noexcept;

private:
  Variant mVariant;
};

using MsgBasePtr = std::shared_ptr<MsgBase>;
using ConstMsgBasePtr = std::shared_ptr<const MsgBase>;

using MsgBaseUPtr = std::unique_ptr<MsgBase>;
using ConstMsgBaseUPtr = std::unique_ptr<const MsgBase>;


/// @brief Serializes a built message and appends it to a chronicle on a precomputed channel.
///
/// Use this when the caller already holds the channel the message belongs to (for example a hub
/// that hashed the channel once at publish time and routes by it thereafter).
///
/// @param msgBase Message to write; must be one you built, not one opened for reading.
///
/// @param channelId Channel the message is appended to.
///
/// @param writer Open chronicle writer that receives the serialized message.
///
/// @throws std::bad_variant_access if @p msgBase was opened for reading rather than built.
void write(const MsgBase& msgBase, chronicle::ChannelId channelId, chronicle::Writer& writer);

/// @brief Serializes a built message and appends it to a chronicle on the channel for a topic.
///
/// Resolves the channel from the message type and @p topic, so that two topics carrying the same
/// message type land on distinct channels, then appends as @ref write(const MsgBase&,
/// chronicle::ChannelId, chronicle::Writer&) does.
///
/// @param msgBase Message to write; must be one you built, not one opened for reading.
///
/// @param topic Topic name.
///
/// @param writer Open chronicle writer that receives the serialized message.
///
/// @throws std::bad_variant_access if @p msgBase was opened for reading rather than built.
void write(const MsgBase& msgBase, const std::string_view& topic, chronicle::Writer& writer);

/// @brief Computes the channel a message type carried on a topic belongs to.
///
/// Combines the message type identifier with the topic name, so the same schema on two topics maps
/// to two distinct channels. Deterministic: equal inputs always yield the same channel.
///
/// @param msgId Identifier of the message type (see @ref Msg::kMsgId).
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

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
      const std::string_view& topic,
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


/// @brief Serializes a built message and appends it to a chronicle on an explicit channel.
///
/// Use this when the channel is keyed by more than the message type (for example by a topic name),
/// so that two topics carrying the same message type land on distinct channels.
///
/// @param msgBase Message to write; must be one you built, not one opened for reading.
///
/// @param topic Topic name.
///
/// @param writer Open chronicle writer that receives the serialized message.
void write(const MsgBase& msgBase, const std::string_view& topic, chronicle::Writer& writer);

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

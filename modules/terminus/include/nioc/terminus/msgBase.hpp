////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <capnp/serialize.h>
#include <cstdint>
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

/// @brief Common base for typed messages; use the derived @ref Msg instead.
///
/// A message is in one of two states: built (created empty, then filled in and written) or read
/// (loaded from an existing MemoryCrate). MsgBase holds whichever state applies and exposes the
/// type identifier shared by both.
class MsgBase
{
public:
  /// @brief Numeric identifier for a message type, taken from its Cap'n Proto schema.
  using MsgHandle = std::uint64_t;

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

  virtual ~MsgBase() = default;

  MsgBase& operator=(const MsgBase&) = delete;

  MsgBase& operator=(MsgBase&&) noexcept = delete;

  /// @brief Returns this message type's numeric identifier.
  [[nodiscard]] virtual MsgHandle msgHandle() const = 0;

protected:
  friend void write(MsgBase& msgBase, chronicle::Writer& writer);

  /// @brief Returns the underlying built/read state, used by @ref write.
  [[nodiscard]] Variant& variant() noexcept;

private:
  Variant mVariant;
};

/// @brief Serializes a built message and appends it to a chronicle.
///
/// @param msgBase Message to write; must be one you built, not one opened for reading.
///
/// @param writer Open chronicle writer that receives the serialized message.
void write(MsgBase& msgBase, chronicle::Writer& writer);


} // namespace nioc::terminus

////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <capnp/serialize.h>
#include <cstdint>
#include <nioc/chronicle/chronicle.hpp>
#include <nioc/chronicle/memoryCrate.hpp>
#include <variant>

namespace nioc::messages
{

/// @brief Message reader for chronicle data.
///
/// Reads Cap'n Proto messages from chronicle storage.
class MMappedMessageReader final:
    public chronicle::MemoryCrate,
    public capnp::FlatArrayMessageReader
{
public:
  /// @brief Constructs reader from chronicle data.
  /// @param memoryCrate Chronicle data container.
  explicit MMappedMessageReader(chronicle::MemoryCrate memoryCrate);

  MMappedMessageReader(const MMappedMessageReader&) = delete;

  MMappedMessageReader(MMappedMessageReader&&) = delete;

  ~MMappedMessageReader() final = default;

  MMappedMessageReader& operator=(const MMappedMessageReader&) = delete;

  MMappedMessageReader& operator=(MMappedMessageReader&&) = delete;
};

/// @brief Base class for messages.
///
/// Provides common interface for all message types. Supports both creating new messages and reading from chronicle.
class MsgBase
{
public:
  /// @brief Unique identifier for message type.
  using MsgHandle = std::uint64_t;

  using Variant = std::variant<capnp::MallocMessageBuilder, MMappedMessageReader>;

protected:
  /// @brief Creates new message.
  MsgBase();

  /// @brief Loads message from chronicle.
  /// @param memoryCrate Chronicle data.
  explicit MsgBase(chronicle::MemoryCrate memoryCrate);

public:
  MsgBase(const MsgBase&) = delete;

  MsgBase(MsgBase&&) noexcept = delete;

  virtual ~MsgBase() = default;

  MsgBase& operator=(const MsgBase&) = delete;

  MsgBase& operator=(MsgBase&&) noexcept = delete;

  /// @brief Gets message type identifier.
  /// @return Message type ID.
  [[nodiscard]] virtual MsgHandle msgHandle() const = 0;

protected:
  friend void write(MsgBase&, chronicle::Writer&);

  /// @brief Accesses internal message variant.
  /// @return Message variant.
  [[nodiscard]] Variant& variant() noexcept;

private:
  Variant mVariant;
};

/// @brief Writes message to chronicle.
/// @param msgBase Message to write.
/// @param writer Chronicle writer.
void write(MsgBase& msgBase, chronicle::Writer& writer);


} // namespace nioc::messages

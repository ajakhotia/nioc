////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "msgBase.hpp"

namespace nioc::messages
{

/// @brief Typed message container.
///
/// Wraps Cap'n Proto schema for serialization. Supports both creating and reading messages.
///
/// Usage:
/// ```
/// Msg<MySchema> msg;
/// auto builder = msg.builder();
/// builder.setField(value);
/// write(msg, writer);
/// ```
///
/// @tparam Schema_ Cap'n Proto schema type.
template<typename Schema_>
class Msg final: public MsgBase
{
public:
  using Schema = Schema_;
  using Reader = Schema::Reader;
  using Builder = Schema::Builder;

  static constexpr auto kMsgHandle = static_cast<MsgHandle>(Schema::_capnpPrivate::typeId);

  /// @brief Creates new empty message.
  Msg()
  {
    std::get<capnp::MallocMessageBuilder>(variant()).template initRoot<Schema>();
  }

  /// @brief Loads message from chronicle.
  /// @param memoryCrate Chronicle data.
  explicit Msg(chronicle::MemoryCrate memoryCrate): MsgBase(std::move(memoryCrate)) {}

  /// @brief Gets message type ID.
  /// @return Type identifier.
  [[nodiscard]] MsgHandle msgHandle() const override
  {
    return kMsgHandle;
  }

  /// @brief Gets reader to access message data.
  /// @return Message reader.
  Reader reader()
  {
    return std::visit(
        [](auto& var)
        {
          return Reader(var.template getRoot<Schema>());
        },
        variant());
  }

  /// @brief Gets builder to modify message data.
  /// @return Message builder.
  Builder builder()
  {
    return std::get<capnp::MallocMessageBuilder>(variant()).template getRoot<Schema>();
  }
};


} // namespace nioc::messages

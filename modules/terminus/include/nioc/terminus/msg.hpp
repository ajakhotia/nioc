////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "msgBase.hpp"

namespace nioc::terminus
{

/// @brief Typed message for a single Cap'n Proto schema.
///
/// Create one empty to build a message, or from a MemoryCrate to read an existing one. Reach the
/// payload through @ref builder (write side) or @ref reader (read side).
///
/// @code
/// // Build a message and hand it to a chronicle writer.
/// auto msg = nioc::terminus::Msg<MySchema>{};
/// auto builder = msg.builder();
/// builder.setField(value);
/// nioc::terminus::write(msg, writer);
///
/// // Open an existing message held in a MemoryCrate.
/// auto loaded = nioc::terminus::Msg<MySchema>{ memoryCrate };
/// const auto field = loaded.reader().getField();
/// @endcode
///
/// @tparam Schema_ Cap'n Proto schema type.
template<typename Schema_>
class Msg final: public MsgBase
{
public:
  using Schema = Schema_;
  using Reader = Schema::Reader;
  using Builder = Schema::Builder;

  /// @brief Compile-time message type identifier for this schema.
  static constexpr auto kMsgHandle = static_cast<MsgHandle>(Schema::_capnpPrivate::typeId);

  /// @brief Creates an empty message ready to be built.
  Msg()
  {
    std::get<capnp::MallocMessageBuilder>(variant()).template initRoot<Schema>();
  }

  /// @brief Loads a message for reading from an existing MemoryCrate.
  /// @param memoryCrate Crate holding one serialized message.
  explicit Msg(chronicle::MemoryCrate memoryCrate): MsgBase(std::move(memoryCrate)) {}

  /// @brief Returns this message type's numeric identifier.
  [[nodiscard]] MsgHandle msgHandle() const override
  {
    return kMsgHandle;
  }

  /// @brief Returns a reader over the message payload.
  Reader reader()
  {
    return std::visit(
        [](auto& var)
        {
          return Reader(var.template getRoot<Schema>());
        },
        variant());
  }

  /// @brief Returns a builder for the message payload.
  ///
  /// Call only on a message you created empty, not one opened for reading.
  Builder builder()
  {
    return std::get<capnp::MallocMessageBuilder>(variant()).template getRoot<Schema>();
  }
};


} // namespace nioc::terminus

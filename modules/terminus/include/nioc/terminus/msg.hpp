////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "msgBase.hpp"

namespace nioc::terminus
{

/// @brief Typed message for one Cap'n Proto schema.
///
/// Create it empty to build a message, or from a MemoryCrate to read one. Get the payload through
/// @ref builder to write or @ref reader to read.
///
/// @code
/// // Build a message and hand it to a chronicle writer.
/// auto msg = nioc::terminus::Msg<MySchema>{};
/// auto builder = msg.builder();
/// builder.setField(value);
/// nioc::terminus::write(msg, "myTopic", writer);
///
/// // Open an existing message held in a MemoryCrate.
/// auto loaded = nioc::terminus::Msg<MySchema>{ memoryCrate };
/// const auto field = loaded.reader().getField();
/// @endcode
///
/// @tparam SchemaType Cap'n Proto schema type.
template<typename SchemaType>
class Msg final: public MsgBase
{
public:
  using Schema = SchemaType;
  using Reader = Schema::Reader;
  using Builder = Schema::Builder;

  /// @brief This schema's message type ID, known at compile time.
  static constexpr auto kMsgId = MsgId{static_cast<std::uint64_t>(Schema::_capnpPrivate::typeId)};

  /// @brief Creates an empty message, ready to build.
  Msg()
  {
    std::get<capnp::MallocMessageBuilder>(variant()).template initRoot<Schema>();
  }

  /// @brief Opens a message for reading from a MemoryCrate.
  /// @param memoryCrate Crate holding one serialized message.
  explicit Msg(chronicle::MemoryCrate memoryCrate): MsgBase(std::move(memoryCrate)) {}

  /// @brief Returns this message's type ID.
  [[nodiscard]] constexpr MsgId msgId() const override
  {
    return kMsgId;
  }

  /// @brief Returns a reader for the payload.
  Reader reader()
  {
    return std::visit([](auto& var) { return Reader(var.template getRoot<Schema>()); }, variant());
  }

  /// @brief Returns a reader for the payload of a const message.
  [[nodiscard]] Reader reader() const
  {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    return const_cast<Msg*>(this)->reader();
  }

  /// @brief Returns a builder for the payload.
  ///
  /// Call only on a message you created empty, not one opened for reading.
  Builder builder()
  {
    return std::get<capnp::MallocMessageBuilder>(variant()).template getRoot<Schema>();
  }
};

/// @brief Shared-ownership pointer to a mutable message.
template<typename Schema>
using MsgPtr = std::shared_ptr<Msg<Schema>>;

/// @brief Shared-ownership pointer to a read-only message.
template<typename Schema>
using ConstMsgPtr = std::shared_ptr<const Msg<Schema>>;

/// @brief Sole-ownership pointer to a mutable message.
template<typename Schema>
using MsgUPtr = std::unique_ptr<Msg<Schema>>;

/// @brief Sole-ownership pointer to a read-only message.
template<typename Schema>
using ConstMsgUPtr = std::unique_ptr<const Msg<Schema>>;


} // namespace nioc::terminus

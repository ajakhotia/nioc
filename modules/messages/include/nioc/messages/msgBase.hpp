////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <capnp/serialize.h>
#include <cstdint>
#include <nioc/logger/logger.hpp>
#include <nioc/logger/memoryCrate.hpp>
#include <variant>

namespace nioc::messages
{

class MMappedMessageReader final: public logger::MemoryCrate, public capnp::FlatArrayMessageReader
{
public:
  explicit MMappedMessageReader(logger::MemoryCrate memoryCrate);

  MMappedMessageReader(const MMappedMessageReader&) = delete;

  MMappedMessageReader(MMappedMessageReader&&) = delete;

  ~MMappedMessageReader() final = default;

  MMappedMessageReader& operator=(const MMappedMessageReader&) = delete;

  MMappedMessageReader& operator=(MMappedMessageReader&&) = delete;
};

class MsgBase
{
public:
  using MsgHandle = std::uint64_t;

  using Variant = std::variant<capnp::MallocMessageBuilder, MMappedMessageReader>;

protected:
  MsgBase();

  explicit MsgBase(logger::MemoryCrate memoryCrate);

public:
  MsgBase(const MsgBase&) = delete;

  MsgBase(MsgBase&&) noexcept = delete;

  virtual ~MsgBase() = default;

  MsgBase& operator=(const MsgBase&) = delete;

  MsgBase& operator=(MsgBase&&) noexcept = delete;

  [[nodiscard]] virtual MsgHandle msgHandle() const = 0;

protected:
  friend void write(MsgBase&, logger::Logger&);

  [[nodiscard]] Variant& variant() noexcept;

private:
  Variant mVariant;
};

void write(MsgBase& msgBase, logger::Logger& logger);


} // namespace nioc::messages

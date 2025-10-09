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

class MMappedMessageReader final:
    public chronicle::MemoryCrate,
    public capnp::FlatArrayMessageReader
{
public:
  explicit MMappedMessageReader(chronicle::MemoryCrate memoryCrate);

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

  explicit MsgBase(chronicle::MemoryCrate memoryCrate);

public:
  MsgBase(const MsgBase&) = delete;

  MsgBase(MsgBase&&) noexcept = delete;

  virtual ~MsgBase() = default;

  MsgBase& operator=(const MsgBase&) = delete;

  MsgBase& operator=(MsgBase&&) noexcept = delete;

  [[nodiscard]] virtual MsgHandle msgHandle() const = 0;

protected:
  friend void write(MsgBase&, chronicle::Writer&);

  [[nodiscard]] Variant& variant() noexcept;

private:
  Variant mVariant;
};

void write(MsgBase& msgBase, chronicle::Writer& writer);


} // namespace nioc::messages

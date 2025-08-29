////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "msgBase.hpp"

namespace nioc::messages
{


template<typename Schema_>
class Msg final: public MsgBase
{
public:
    using Schema = Schema_;
    using Reader = typename Schema::Reader;
    using Builder = typename Schema::Builder;
    using MsgHandle = MsgBase::MsgHandle;

    static constexpr MsgHandle kMsgHandle = Schema::_capnpPrivate::typeId;

    Msg(): MsgBase()
    {
        std::get<capnp::MallocMessageBuilder>(variant()).template initRoot<Schema>();
    }

    explicit Msg(logger::MemoryCrate memoryCrate): MsgBase(std::move(memoryCrate)) {}


    [[nodiscard]] MsgHandle msgHandle() const override
    {
        return kMsgHandle;
    }


    Reader reader()
    {
        return std::visit([](auto& var) { return Reader(var.template getRoot<Schema>()); },
                          variant());
    }

    Builder builder()
    {
        return std::get<capnp::MallocMessageBuilder>(variant()).template getRoot<Schema>();
    }
};


} // namespace nioc::messages

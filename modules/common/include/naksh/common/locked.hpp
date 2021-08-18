////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <shared_mutex>

namespace naksh::common
{

template<typename ValueType>
class Locked
{
public:
    Locked():
            mMutex{},
            mLockedValue{}
    {
    }

    explicit Locked(ValueType&& value):
            mMutex{},
            mLockedValue{std::move(value)}
    {
    };

    Locked(const Locked&) = delete;

    Locked(Locked&&) = delete;

    ~Locked() = default;

    Locked& operator=(const Locked&) = delete;

    Locked& operator=(Locked&&) = delete;


    template<typename Operation>
    decltype(auto) operator()(Operation&& operation) const
    {
        std::shared_lock sharedLock(mMutex);
        return operation(mLockedValue);
    }


    template<typename Operation>
    decltype(auto) operator()(Operation&& operation)
    {
        std::lock_guard exclusiveLock(mMutex);
        return operation(mLockedValue);
    }


    template<typename RhsType>
    Locked& operator=(const RhsType& rhs)
    {
        this->template operator()(
                [&rhs](ValueType& valueRef)
                {
                    valueRef = rhs;
                });

        return *this;
    }


    template<typename RhsType>
    Locked& operator=(RhsType&& rhs)
    {
        this->template operator()(
                [rhs = std::forward<RhsType>(rhs)](ValueType& valueRef) mutable
                {
                    valueRef = std::move(rhs);
                });

        return *this;
    }


    ValueType copy() const
    {
        return this->template operator()([](const ValueType valueCopy){ return valueCopy; });
    }


    ValueType move()
    {
        return this->template operator()(
                [](ValueType& valueRef)
                {
                    ValueType value = std::move(valueRef);
                    return value;
                });
    }

private:
    mutable std::shared_mutex mMutex;

    ValueType mLockedValue;
};


} // End of namespace naksh::common.

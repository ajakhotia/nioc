////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <shared_mutex>

namespace naksh::common
{

/// @Class      Locked
/// @brief      A utility to protect a non-atomic type from access contention.
///
/// @details    All operations on the underlying variable Locked::mLockedValue
///             are issued using lambdas with exactly one parameter as in the
///             following signatures:
///
///                 1. [](const ValueType& value){ DoSomethingWithValue; return result; };
///                 2. [](Value value){ DoSomethingWithValue; return result; };
///                 3. [](Value& value){ DoSomethingWithValue; return result; };
///                 4. [](Value&&){ DoSomethingWithValue; return result; };
///
///             The underlying variable Locked::mLockedValue is passed as an
///             argument to the provided lambda after suitably locking the
///             protecting mutex (Locked::mMutex). See Locked::operator()(...).
///
///             NOTE A:
///                 Lambdas with signature 1 and 2 do not modify the underlying
///                 value and hence can be used with const-qualified instances
///                 of this class. Such an operation requires a shared lock only
///                 as seen on the const-qualified overload of operator()(...).
///
///             NOTE B:
///                 Lambdas with signature 3 and 4 may modify the underlying
///                 value and hence can be only used with non-const-qualified
///                 variables. Such an operation mandates an exclusive lock
///                 as seen in the non-const qualified overload of operator()(...).
///
///
/// @tparam     ValueType_  Type of the underlying variable. Any const qualifiers
///                         specifiers are discarded.
template<typename ValueType_>
class Locked
{
public:
    using ValueType = typename std::remove_const<ValueType_>::type;


    /// @brief  Variadic constructor that accepts and forwards any arguments to the
    ///         constructor of the underlying types.
    ///
    /// @tparam Args    Variadic parameter pack accepted by the constructors of
    ///                 the underlying type a.k.a @tparam ValueType
    ///
    /// @param  args    Parameter pack to be forwarded as arguments to the
    ///                 constructor of the underlying value a.k.a Locked::mLockedValue
    template<typename... Args>
    explicit Locked(Args... args):
            mMutex{},
            mLockedValue{std::forward<Args>(args)...}
    {
    }


    /// @brief  Deleted copy constructor. Mutexes are not copyable.
    Locked(const Locked&) = delete;

    /// @brief  Deleted move constructor. Mutexes are not movable.
    Locked(Locked&&) noexcept = delete;

    /// @brief  Default destructor.
    ~Locked() = default;

    /// @brief  Deleted copy-assignment. Mutexes do not allow copy-assignment.
    Locked& operator=(const Locked&) = delete;

    /// @brief  Deleted move-assignment. Mutexes do not allow move-assignment.
    Locked& operator=(Locked&&) = delete;


    /// @brief  Runs the @param operation with Locked::mLockedValue as the argument
    ///         after acquiring a shared lock. Because this overload is
    ///         const-qualified, it's invoked only for read-only operations.
    ///
    /// @tparam Operation   Type of the operating lambda.
    ///
    /// @param  operation   The operating lambda.
    ///
    /// @return Any result returned by the @param operation is returned to the
    ///         calling context.
    template<typename Operation>
    decltype(auto) operator()(Operation&& operation) const
    {
        std::shared_lock sharedLock(mMutex);
        return operation(mLockedValue);
    }


    /// @brief  Runs the @param operation with Locked::mLockedValue as the argument
    ///         after acquiring an exclusive lock. Because this overload is
    ///         not const-qualified, it's typical invoked for write operations.
    ///
    /// @tparam Operation   Type of the operating lambda.
    ///
    /// @param  operation   The operating lambda.
    ///
    /// @return Any result returned by the @param operation is returned to the
    ///         calling context.
    template<typename Operation>
    decltype(auto) operator()(Operation&& operation)
    {
        std::lock_guard exclusiveLock(mMutex);
        return operation(mLockedValue);
    }


    /// @brief  A convenience operator to copy-assign a value to the underlying type
    ///         in a thread-safe manner. Of course invocation of this method is
    ///         only valid when @tparam OtherType is copy-assignable to @tparam ValueType.
    ///
    /// @tparam OtherType   The type of the assigned variable.
    ///                     Any implicit conversion are performed automatically.
    ///
    /// @param  other       The assigned variable.
    ///
    /// @return A reference to self post assignment.
    template<typename OtherType>
    Locked& operator=(const OtherType& other)
    {
        this->template operator()([&other](ValueType& value){ value = other; });
        return *this;
    }


    /// @brief  A convenience operator to move-assign a value to the underlying type
    ///         in a thread-safe manner. Of course invocation of this method is
    ///         only valid when @tparam OtherType is move-assignable to @tparam ValueType .
    ///
    /// @tparam OtherType   The type of the assigned variable.
    ///                     Any implicit conversion are performed automatically.
    ///
    /// @param  other       The assigned variable that is inadvertently consumed by the move.
    ///
    /// @return A reference to self.
    template<typename OtherType>
    Locked& operator=(OtherType&& other)
    {
        this->template operator()(
                [other = std::forward<OtherType>(other)](ValueType& value) mutable
                {
                    value = std::move(other);
                });

        return *this;
    }


    /// @brief  Convenience function to return a copy of the underlying member.
    /// @return A copy of the underlying member.
    ValueType copy() const
    {
        // Note the copy in the invocation of the lambda.
        return this->template operator()([](const ValueType value){ return value; });
    }


    /// @brief      Convenience function to extract the underlying member.
    ///
    /// @details    After the extraction, the underlying member is set to state
    ///             as dictated by the move assignment of the underlying type,
    ///             a.k.a @tparam ValueType.
    ///
    /// @return     Extracted value of the underlying member.
    ValueType move()
    {
        // Without std::move() here, the compiler will attempt to make a copy.
        return this->template operator()([](ValueType& value){ return std::move(value); });
    }

private:
    /// Mutex to protect the mLockedValue.
    mutable std::shared_mutex mMutex;

    /// Guarded l-value.
    ValueType mLockedValue;
};


/// Equality Check.
template<typename ValueType, typename Other>
constexpr bool operator==(const Locked<ValueType>& lockedValue, const Other& otherValue)
{
    return lockedValue([&otherValue](const ValueType& value) { return value == otherValue; });
}

template<typename Other, typename ValueType>
constexpr bool operator==(const Other& otherValue, const Locked<ValueType>& lockedValue)
{
    return lockedValue([&otherValue](const ValueType& value) { return otherValue == value; });
}


/// In-equality check.
template<typename ValueType, typename Other>
constexpr bool operator!=(const Locked<ValueType>& lockedValue, const Other& otherValue)
{
    return lockedValue([&otherValue](const ValueType& value) { return value != otherValue; });
}

template<typename Other, typename ValueType>
constexpr bool operator!=(const Other& otherValue, const Locked<ValueType>& lockedValue)
{
    return lockedValue([&otherValue](const ValueType& value) { return otherValue != value; });
}


/// Lesser-than check.
template<typename ValueType, typename Other>
constexpr bool operator<(const Locked<ValueType>& lockedValue, const Other& otherValue)
{
    return lockedValue([&otherValue](const ValueType& value) { return value < otherValue; });
}

template<typename Other, typename ValueType>
constexpr bool operator<(const Other& otherValue, const Locked<ValueType>& lockedValue)
{
    return lockedValue([&otherValue](const ValueType& value) { return  otherValue < value; });
}


/// Lesser-than-or-equal check.
template<typename ValueType, typename Other>
constexpr bool operator<=(const Locked<ValueType>& lockedValue, const Other& otherValue)
{
    return lockedValue([&otherValue](const ValueType& value) { return value <= otherValue; });
}

template<typename Other, typename ValueType>
constexpr bool operator<=(const Other& otherValue, const Locked<ValueType>& lockedValue)
{
    return lockedValue([&otherValue](const ValueType& value) { return otherValue <= value; });
}


/// Greater-than check.
template<typename ValueType, typename Other>
constexpr bool operator>(const Locked<ValueType>& lockedValue, const Other& otherValue)
{
    return lockedValue([&otherValue](const ValueType& value) { return value > otherValue; });
}

template<typename Other, typename ValueType>
constexpr bool operator>(const Other& otherValue, const Locked<ValueType>& lockedValue)
{
    return lockedValue([&otherValue](const ValueType& value) { return otherValue > value; });
}


/// Greater-than-or-equal.
template<typename ValueType, typename Other>
constexpr bool operator>=(const Locked<ValueType>& lockedValue, const Other& otherValue)
{
    return lockedValue([&otherValue](const ValueType& value) { return value >= otherValue; });
}

template<typename Other, typename ValueType>
constexpr bool operator>=(const Other& otherValue, const Locked<ValueType>& lockedValue)
{
    return lockedValue([&otherValue](const ValueType& value) { return otherValue >= value; });
}



} // End of namespace naksh::common.

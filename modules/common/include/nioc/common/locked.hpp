////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <mutex>
#include <shared_mutex>

namespace nioc::common
{
/// @brief A thread-safe wrapper that owns one value and exposes it only through caller-supplied
/// callables run while a lock is held.
///
/// You never get a raw reference to the value. Instead you hand in a callable; it receives a
/// reference to the value for the duration of the call, under a lock. Reads take a shared lock and
/// can run at the same time as other reads; writes take an exclusive lock and run alone. This makes
/// every access to the value safe across threads, even if the value's own type is not thread-safe.
///
/// Example:
///
///     Locked<std::vector<int>> shared{};
///     shared.execute([](auto& v) { v.push_back(42); }); // exclusive lock
///     const auto size = shared.cExecute([](const auto& v) { return v.size(); }); // shared lock
///
/// Not copyable or movable, because it owns a `std::shared_mutex`.
///
/// @tparam ValueType_ The guarded value's type. Any top-level `const` is stripped (see
/// `ValueType`), so a value declared `const` can still be mutated under the exclusive lock.
///
/// @see cExecute, execute, operator()
template<typename ValueType_>
class Locked
{
public:
  /// The stored value type: `ValueType_` with any top-level `const` removed.
  using ValueType = std::remove_const_t<ValueType_>;

  /// @brief Constructs the guarded value in place by forwarding the arguments to its constructor.
  ///
  /// @tparam Args The guarded value's constructor argument types.
  ///
  /// @param args Forwarded to `ValueType`'s constructor. With no arguments the value is
  /// value-initialized.
  template<typename... Args>
  explicit Locked(Args&&... args): mLockedValue{std::forward<Args>(args)...}
  {
  }

  Locked(const Locked&) = delete;

  Locked(Locked&&) noexcept = delete;

  ~Locked() = default;

  Locked& operator=(const Locked&) = delete;

  Locked& operator=(Locked&&) = delete;

  /// @brief Runs a read-only operation under a shared lock and returns whatever it returns.
  ///
  /// @tparam Operation The callable's type.
  ///
  /// @param operation Called with a `const` reference to the value. Must not let that reference
  /// escape, and must not call back into the same `Locked` (that deadlocks). Other `cExecute` calls
  /// may run concurrently. Anything it throws propagates out unchanged.
  ///
  /// @return The result of `operation`, with its exact type and value category preserved.
  template<typename Operation>
  decltype(auto) cExecute(Operation&& operation) const
  {
    const auto sharedLock = std::shared_lock(mMutex);
    return std::forward<Operation>(operation)(mLockedValue);
  }

  /// @brief Runs a read-only operation under a shared lock; identical to `cExecute`.
  ///
  /// @see cExecute
  template<typename Operation>
  decltype(auto) execute(Operation&& operation) const
  {
    return cExecute(std::forward<Operation>(operation));
  }

  /// @brief Call syntax for the read-only `cExecute`: `locked(op)` runs `op` under a shared lock.
  ///
  /// @see cExecute
  template<typename Operation>
  decltype(auto) operator()(Operation&& operation) const
  {
    return cExecute(std::forward<Operation>(operation));
  }

  /// @brief Runs a mutating operation under an exclusive lock and returns whatever it returns.
  ///
  /// Blocks until no shared or exclusive lock is held, then runs alone.
  ///
  /// @tparam Operation The callable's type.
  ///
  /// @param operation Called with a mutable reference to the value. Must not let that reference
  /// escape, and must not call back into the same `Locked` (that deadlocks). Anything it throws
  /// propagates out unchanged.
  ///
  /// @return The result of `operation`, with its exact type and value category preserved.
  template<typename Operation>
  decltype(auto) execute(Operation&& operation)
  {
    const auto exclusiveLock = std::scoped_lock(mMutex);
    return std::forward<Operation>(operation)(mLockedValue);
  }

  /// @brief Call syntax for the mutating `execute`: `locked(op)` runs `op` under an exclusive lock.
  ///
  /// @see execute
  template<typename Operation>
  decltype(auto) operator()(Operation&& operation)
  {
    return execute(std::forward<Operation>(operation));
  }

  /// @brief Copy-assigns `other` into the guarded value under an exclusive lock.
  ///
  /// @tparam OtherType The assigned value's type.
  ///
  /// @param other Copied into the guarded value via its copy-assignment.
  template<typename OtherType>
  Locked& operator=(const OtherType& other)
  {
    execute([&other](auto& value) { value = other; });
    return *this;
  }

  /// @brief Move-assigns `other` into the guarded value under an exclusive lock.
  ///
  /// @tparam OtherType The assigned value's type.
  ///
  /// @param other An rvalue moved into the guarded value, leaving it in its moved-from state.
  template<typename OtherType>
  Locked& operator=(OtherType&& other)
  {
    execute([other = std::forward<OtherType>(other)](auto& value) mutable
            { value = std::move(other); });

    return *this;
  }

  /// @brief Returns a copy of the guarded value, taken under a shared lock.
  ValueType copy() const
  {
    return cExecute([](const auto value) { return value; });
  }

  /// @brief Moves the guarded value out under an exclusive lock, leaving it in its moved-from
  /// state.
  ValueType move()
  {
    return execute([](auto& value) { return std::move(value); });
  }

private:
  /// Guards every access to `mLockedValue`. A shared lock serves reads; an exclusive lock serves
  /// writes. Mutable so that `const` member functions can take a shared lock.
  mutable std::shared_mutex mMutex;

  /// The guarded value. Touched only while `mMutex` is held.
  ValueType mLockedValue;
};

/// @brief Compares the guarded value with `otherValue`, reading the guarded value under a shared
/// lock.
///
/// The full relational family (`==`, `!=`, `<`, `<=`, `>`, `>=`, with the `Locked` on either side)
/// follows this same pattern. Each call locks independently and is atomic with respect to writes,
/// but a sequence of comparisons is not collectively atomic.
///
/// @return The result of `value == otherValue`.
template<typename ValueType, typename Other>
bool operator==(const Locked<ValueType>& lockedValue, const Other& otherValue)
{
  return lockedValue([&otherValue](const auto& value) { return value == otherValue; });
}

/// @brief Reversed-argument `==`: compares `otherValue` with the guarded value under a shared lock.
template<typename Other, typename ValueType>
bool operator==(const Other& otherValue, const Locked<ValueType>& lockedValue)
{
  return lockedValue([&otherValue](const auto& value) { return otherValue == value; });
}

/// @brief Compares the guarded value with `otherValue` for inequality under a shared lock.
template<typename ValueType, typename Other>
bool operator!=(const Locked<ValueType>& lockedValue, const Other& otherValue)
{
  return lockedValue([&otherValue](const auto& value) { return value != otherValue; });
}

/// @brief Reversed-argument `!=`: compares `otherValue` with the guarded value under a shared lock.
template<typename Other, typename ValueType>
bool operator!=(const Other& otherValue, const Locked<ValueType>& lockedValue)
{
  return lockedValue([&otherValue](const auto& value) { return otherValue != value; });
}

/// @brief Tests whether the guarded value is less than `otherValue` under a shared lock.
template<typename ValueType, typename Other>
bool operator<(const Locked<ValueType>& lockedValue, const Other& otherValue)
{
  return lockedValue([&otherValue](const auto& value) { return value < otherValue; });
}

/// @brief Tests whether `otherValue` is less than the guarded value under a shared lock.
template<typename Other, typename ValueType>
bool operator<(const Other& otherValue, const Locked<ValueType>& lockedValue)
{
  return lockedValue([&otherValue](const auto& value) { return otherValue < value; });
}

/// @brief Tests whether the guarded value is less than or equal to `otherValue` under a shared
/// lock.
template<typename ValueType, typename Other>
bool operator<=(const Locked<ValueType>& lockedValue, const Other& otherValue)
{
  return lockedValue([&otherValue](const auto& value) { return value <= otherValue; });
}

/// @brief Tests whether `otherValue` is less than or equal to the guarded value under a shared
/// lock.
template<typename Other, typename ValueType>
bool operator<=(const Other& otherValue, const Locked<ValueType>& lockedValue)
{
  return lockedValue([&otherValue](const auto& value) { return otherValue <= value; });
}

/// @brief Tests whether the guarded value is greater than `otherValue` under a shared lock.
template<typename ValueType, typename Other>
bool operator>(const Locked<ValueType>& lockedValue, const Other& otherValue)
{
  return lockedValue([&otherValue](const auto& value) { return value > otherValue; });
}

/// @brief Tests whether `otherValue` is greater than the guarded value under a shared lock.
template<typename Other, typename ValueType>
bool operator>(const Other& otherValue, const Locked<ValueType>& lockedValue)
{
  return lockedValue([&otherValue](const auto& value) { return otherValue > value; });
}

/// @brief Tests whether the guarded value is greater than or equal to `otherValue` under a shared
/// lock.
template<typename ValueType, typename Other>
bool operator>=(const Locked<ValueType>& lockedValue, const Other& otherValue)
{
  return lockedValue([&otherValue](const auto& value) { return value >= otherValue; });
}

/// @brief Tests whether `otherValue` is greater than or equal to the guarded value under a shared
/// lock.
template<typename Other, typename ValueType>
bool operator>=(const Other& otherValue, const Locked<ValueType>& lockedValue)
{
  return lockedValue([&otherValue](const auto& value) { return otherValue >= value; });
}


} // namespace nioc::common

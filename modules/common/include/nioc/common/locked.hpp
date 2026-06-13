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
/// @brief Thread-safe wrapper for a value.
///
/// Access the value only through a callable you pass in. Reading (const access, or @ref cExecute)
/// gives the callable a const reference under a shared lock, so readers run at the same time.
/// Writing (mutable access) gives a mutable reference under an exclusive lock, so a writer runs
/// alone. Cannot be copied or moved; use @ref copy or @ref move to get the value out.
///
/// @code
/// nioc::common::Locked<int> counter(0);
///
/// // Read: the lambda gets a const reference.
/// const int value = counter([](const auto& val) { return val; });
///
/// // Write: the lambda gets a mutable reference.
/// counter([](auto& val) { val++; });
///
/// // The call returns whatever the lambda returns.
/// const bool wasZero = counter([](auto& val) {
///   const bool result = (val == 0);
///   val = 42;
///   return result;
/// });
/// @endcode
///
/// @tparam ValueType_ Type of the guarded value.
template<typename ValueType_>
class Locked
{
public:
  /// @brief The guarded value type, with any top-level const removed.
  using ValueType = std::remove_const_t<ValueType_>;

  /// @brief Builds the guarded value in place from @p args.
  template<typename... Args>
  explicit Locked(Args&&... args): mLockedValue{std::forward<Args>(args)...}
  {
  }

  Locked(const Locked&) = delete;

  Locked(Locked&&) noexcept = delete;

  ~Locked() = default;

  Locked& operator=(const Locked&) = delete;

  Locked& operator=(Locked&&) = delete;

  /// @brief Runs @p operation with a const reference to the value, under a shared lock.
  /// @return Whatever @p operation returns.
  template<typename Operation>
  decltype(auto) cExecute(Operation&& operation) const
  {
    const auto sharedLock = std::shared_lock(mMutex);
    return std::forward<Operation>(operation)(mLockedValue);
  }

  /// @brief Same as @ref cExecute.
  template<typename Operation>
  decltype(auto) execute(Operation&& operation) const
  {
    return cExecute(std::forward<Operation>(operation));
  }

  /// @brief Same as @ref cExecute.
  template<typename Operation>
  decltype(auto) operator()(Operation&& operation) const
  {
    return cExecute(std::forward<Operation>(operation));
  }

  /// @brief Runs @p operation with a mutable reference to the value, under an exclusive lock.
  /// @return Whatever @p operation returns.
  template<typename Operation>
  decltype(auto) execute(Operation&& operation)
  {
    const auto exclusiveLock = std::scoped_lock(mMutex);
    return std::forward<Operation>(operation)(mLockedValue);
  }

  /// @brief Same as the mutable @ref execute.
  template<typename Operation>
  decltype(auto) operator()(Operation&& operation)
  {
    return execute(std::forward<Operation>(operation));
  }

  /// @brief Copy-assigns @p other to the value, under an exclusive lock.
  template<typename OtherType>
  Locked& operator=(const OtherType& other)
  {
    execute([&other](auto& value) { value = other; });
    return *this;
  }

  /// @brief Move-assigns @p other to the value, under an exclusive lock.
  template<typename OtherType>
  Locked& operator=(OtherType&& other)
  {
    execute([other = std::forward<OtherType>(other)](auto& value) mutable
            { value = std::move(other); });

    return *this;
  }

  /// @brief Returns a copy of the value.
  ValueType copy() const
  {
    return cExecute([](const auto value) { return value; });
  }

  /// @brief Moves the value out. The wrapper is then moved-from but still assignable.
  ValueType move()
  {
    return execute([](auto& value) { return std::move(value); });
  }

private:
  /// Guards mLockedValue against concurrent access.
  mutable std::shared_mutex mMutex;

  /// The protected value.
  ValueType mLockedValue;
};

/// @brief Compares the protected value with @p otherValue, under a shared lock.
template<typename ValueType, typename Other>
bool operator==(const Locked<ValueType>& lockedValue, const Other& otherValue)
{
  return lockedValue([&otherValue](const auto& value) { return value == otherValue; });
}

/// @brief Compares @p otherValue with the protected value, under a shared lock.
template<typename Other, typename ValueType>
bool operator==(const Other& otherValue, const Locked<ValueType>& lockedValue)
{
  return lockedValue([&otherValue](const auto& value) { return otherValue == value; });
}

/// @brief Compares the protected value with @p otherValue, under a shared lock.
template<typename ValueType, typename Other>
bool operator!=(const Locked<ValueType>& lockedValue, const Other& otherValue)
{
  return lockedValue([&otherValue](const auto& value) { return value != otherValue; });
}

/// @brief Compares @p otherValue with the protected value, under a shared lock.
template<typename Other, typename ValueType>
bool operator!=(const Other& otherValue, const Locked<ValueType>& lockedValue)
{
  return lockedValue([&otherValue](const auto& value) { return otherValue != value; });
}

/// @brief Compares the protected value with @p otherValue, under a shared lock.
template<typename ValueType, typename Other>
bool operator<(const Locked<ValueType>& lockedValue, const Other& otherValue)
{
  return lockedValue([&otherValue](const auto& value) { return value < otherValue; });
}

/// @brief Compares @p otherValue with the protected value, under a shared lock.
template<typename Other, typename ValueType>
bool operator<(const Other& otherValue, const Locked<ValueType>& lockedValue)
{
  return lockedValue([&otherValue](const auto& value) { return otherValue < value; });
}

/// @brief Compares the protected value with @p otherValue, under a shared lock.
template<typename ValueType, typename Other>
bool operator<=(const Locked<ValueType>& lockedValue, const Other& otherValue)
{
  return lockedValue([&otherValue](const auto& value) { return value <= otherValue; });
}

/// @brief Compares @p otherValue with the protected value, under a shared lock.
template<typename Other, typename ValueType>
bool operator<=(const Other& otherValue, const Locked<ValueType>& lockedValue)
{
  return lockedValue([&otherValue](const auto& value) { return otherValue <= value; });
}

/// @brief Compares the protected value with @p otherValue, under a shared lock.
template<typename ValueType, typename Other>
bool operator>(const Locked<ValueType>& lockedValue, const Other& otherValue)
{
  return lockedValue([&otherValue](const auto& value) { return value > otherValue; });
}

/// @brief Compares @p otherValue with the protected value, under a shared lock.
template<typename Other, typename ValueType>
bool operator>(const Other& otherValue, const Locked<ValueType>& lockedValue)
{
  return lockedValue([&otherValue](const auto& value) { return otherValue > value; });
}

/// @brief Compares the protected value with @p otherValue, under a shared lock.
template<typename ValueType, typename Other>
bool operator>=(const Locked<ValueType>& lockedValue, const Other& otherValue)
{
  return lockedValue([&otherValue](const auto& value) { return value >= otherValue; });
}

/// @brief Compares @p otherValue with the protected value, under a shared lock.
template<typename Other, typename ValueType>
bool operator>=(const Other& otherValue, const Locked<ValueType>& lockedValue)
{
  return lockedValue([&otherValue](const auto& value) { return otherValue >= value; });
}


} // namespace nioc::common

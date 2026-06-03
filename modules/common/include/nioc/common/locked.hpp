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
/// Guards a value behind a shared mutex and grants access only through a lambda you pass in. Many
/// threads may be read at once; writes are exclusive. The wrapper itself cannot be copied or moved
/// — extract the value with @ref copy or @ref move instead.
///
/// @code
/// nioc::common::Locked<int> counter(0);
///
/// // Read concurrently: the lambda receives a const reference.
/// const int value = counter([](const auto& val) { return val; });
///
/// // Modify exclusively: the lambda receives a mutable reference.
/// counter([](auto& val) { val++; });
///
/// // A modifying lambda may also return a value.
/// const bool wasZero = counter([](auto& val) {
///   const bool result = (val == 0);
///   val = 42;
///   return result;
/// });
/// @endcode
///
/// @tparam ValueType_ Type of the protected value.
template<typename ValueType_>
class Locked
{
public:
  using ValueType = std::remove_const_t<ValueType_>;

  /// @brief Constructs the protected value.
  /// @param args Arguments forwarded to value constructor.
  template<typename... Args>
  explicit Locked(Args&&... args): mLockedValue{std::forward<Args>(args)...}
  {
  }

  Locked(const Locked&) = delete;

  Locked(Locked&&) noexcept = delete;

  ~Locked() = default;

  Locked& operator=(const Locked&) = delete;

  Locked& operator=(Locked&&) = delete;

  /// @brief Reads the value (allows concurrent reads).
  ///
  /// Your lambda receives `const auto&` - you can read but not modify.
  ///
  /// @param operation Lambda that receives const reference to value.
  /// @return Whatever your lambda returns.
  template<typename Operation>
  decltype(auto) cExecute(Operation&& operation) const
  {
    const auto sharedLock = std::shared_lock(mMutex);
    return std::forward<Operation>(operation)(mLockedValue);
  }

  /// @brief Reads the value (allows concurrent reads).
  ///
  /// Alias for cExecute(). Your lambda receives `const auto&`.
  ///
  /// @param operation Lambda that receives const reference to value.
  /// @return Whatever your lambda returns.
  template<typename Operation>
  decltype(auto) execute(Operation&& operation) const
  {
    return cExecute(std::forward<Operation>(operation));
  }

  /// @brief Reads the value (allows concurrent reads).
  ///
  /// Shorthand: `locked([](const auto& val) { ... })`. Lambda receives `const auto&`.
  ///
  /// @param operation Lambda that receives const reference to value.
  /// @return Whatever your lambda returns.
  template<typename Operation>
  decltype(auto) operator()(Operation&& operation) const
  {
    return cExecute(std::forward<Operation>(operation));
  }

  /// @brief Modifies the value (exclusive access).
  ///
  /// Your lambda receives `auto&` - you can read and modify. Only one thread can do this at a time.
  ///
  /// @param operation Lambda that receives mutable reference to value.
  /// @return Whatever your lambda returns.
  template<typename Operation>
  decltype(auto) execute(Operation&& operation)
  {
    const auto exclusiveLock = std::scoped_lock(mMutex);
    return std::forward<Operation>(operation)(mLockedValue);
  }

  /// @brief Modifies the value (exclusive access).
  ///
  /// Shorthand: `locked([](auto& val) { ... })`. Lambda receives `auto&` for modification.
  ///
  /// @param operation Lambda that receives mutable reference to value.
  /// @return Whatever your lambda returns.
  template<typename Operation>
  decltype(auto) operator()(Operation&& operation)
  {
    return execute(std::forward<Operation>(operation));
  }

  /// @brief Replaces the value with a copy.
  ///
  /// Thread-safe assignment. Example: `locked = 42`;
  ///
  /// @param other Value to copy in.
  /// @return Reference to this.
  template<typename OtherType>
  Locked& operator=(const OtherType& other)
  {
    execute(
        [&other](auto& value)
        {
          value = other;
        });
    return *this;
  }

  /// @brief Replaces the value by moving.
  ///
  /// Thread-safe move assignment. Example: `locked = std::move(value);`
  ///
  /// @param other Value to move in.
  /// @return Reference to this.
  template<typename OtherType>
  Locked& operator=(OtherType&& other)
  {
    execute(
        [other = std::forward<OtherType>(other)](auto& value) mutable
        {
          value = std::move(other);
        });

    return *this;
  }

  /// @brief Gets a copy of the protected value.
  ///
  /// Use this to extract the value. The Locked itself cannot be copied.
  ///
  /// @return Copy of the value.
  ValueType copy() const
  {
    return cExecute(
        [](const auto value)
        {
          return value;
        });
  }

  /// @brief Moves the value out, leaving it in the moved-from state.
  ///
  /// The protected value remains valid but unspecified after this.
  ///
  /// @return Moved value.
  ValueType move()
  {
    return execute(
        [](auto& value)
        {
          return std::move(value);
        });
  }

private:
  /// Guards mLockedValue against concurrent access.
  mutable std::shared_mutex mMutex;

  /// The protected value.
  ValueType mLockedValue;
};

/// @brief Compares a locked value with another value.
///
/// Thread-safe comparison. Example: `if (locked == 42) { ... }`
///
/// @return True if equal.
template<typename ValueType, typename Other>
constexpr bool operator==(const Locked<ValueType>& lockedValue, const Other& otherValue)
{
  return lockedValue(
      [&otherValue](const auto& value)
      {
        return value == otherValue;
      });
}

/// @brief Compares value with locked value.
///
/// Thread-safe comparison. Example: `if (42 == locked) { ... }`
///
/// @return True if equal.
template<typename Other, typename ValueType>
constexpr bool operator==(const Other& otherValue, const Locked<ValueType>& lockedValue)
{
  return lockedValue(
      [&otherValue](const auto& value)
      {
        return otherValue == value;
      });
}

/// @brief Compares a locked value with another value.
///
/// Thread-safe inequality check.
///
/// @return True if not equal.
template<typename ValueType, typename Other>
constexpr bool operator!=(const Locked<ValueType>& lockedValue, const Other& otherValue)
{
  return lockedValue(
      [&otherValue](const auto& value)
      {
        return value != otherValue;
      });
}

/// @brief Compares value with locked value.
///
/// Thread-safe inequality check.
///
/// @return True if not equal.
template<typename Other, typename ValueType>
constexpr bool operator!=(const Other& otherValue, const Locked<ValueType>& lockedValue)
{
  return lockedValue(
      [&otherValue](const auto& value)
      {
        return otherValue != value;
      });
}

/// @brief Compares a locked value with another value.
///
/// Thread-safe less-than comparison.
///
/// @return True if less than.
template<typename ValueType, typename Other>
constexpr bool operator<(const Locked<ValueType>& lockedValue, const Other& otherValue)
{
  return lockedValue(
      [&otherValue](const auto& value)
      {
        return value < otherValue;
      });
}

/// @brief Compares value with locked value.
///
/// Thread-safe less-than comparison.
///
/// @return True if less than.
template<typename Other, typename ValueType>
constexpr bool operator<(const Other& otherValue, const Locked<ValueType>& lockedValue)
{
  return lockedValue(
      [&otherValue](const auto& value)
      {
        return otherValue < value;
      });
}

/// @brief Compares a locked value with another value.
///
/// Thread-safe less-than-or-equal comparison.
///
/// @return True if less than or equal.
template<typename ValueType, typename Other>
constexpr bool operator<=(const Locked<ValueType>& lockedValue, const Other& otherValue)
{
  return lockedValue(
      [&otherValue](const auto& value)
      {
        return value <= otherValue;
      });
}

/// @brief Compares value with locked value.
///
/// Thread-safe less-than-or-equal comparison.
///
/// @return True if less than or equal.
template<typename Other, typename ValueType>
constexpr bool operator<=(const Other& otherValue, const Locked<ValueType>& lockedValue)
{
  return lockedValue(
      [&otherValue](const auto& value)
      {
        return otherValue <= value;
      });
}

/// @brief Compares a locked value with another value.
///
/// Thread-safe greater-than comparison.
///
/// @return True if greater than.
template<typename ValueType, typename Other>
constexpr bool operator>(const Locked<ValueType>& lockedValue, const Other& otherValue)
{
  return lockedValue(
      [&otherValue](const auto& value)
      {
        return value > otherValue;
      });
}

/// @brief Compares value with locked value.
///
/// Thread-safe greater-than comparison.
///
/// @return True if greater than.
template<typename Other, typename ValueType>
constexpr bool operator>(const Other& otherValue, const Locked<ValueType>& lockedValue)
{
  return lockedValue(
      [&otherValue](const auto& value)
      {
        return otherValue > value;
      });
}

/// @brief Compares a locked value with another value.
///
/// Thread-safe greater-than-or-equal comparison.
///
/// @return True if greater than or equal.
template<typename ValueType, typename Other>
constexpr bool operator>=(const Locked<ValueType>& lockedValue, const Other& otherValue)
{
  return lockedValue(
      [&otherValue](const auto& value)
      {
        return value >= otherValue;
      });
}

/// @brief Compares value with locked value.
///
/// Thread-safe greater-than-or-equal comparison.
///
/// @return True if greater than or equal.
template<typename Other, typename ValueType>
constexpr bool operator>=(const Other& otherValue, const Locked<ValueType>& lockedValue)
{
  return lockedValue(
      [&otherValue](const auto& value)
      {
        return otherValue >= value;
      });
}


} // namespace nioc::common

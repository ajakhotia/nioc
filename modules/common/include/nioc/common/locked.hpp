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
/// Protects a value from concurrent access across threads. Use lambdas to read or modify safely.
///
/// **Key features:**
/// - Multiple threads can read simultaneously
/// - Only one thread can write at a time
/// - Cannot be copied or moved (use copy() or move() instead)
///
/// **Usage:**
/// ```
/// Locked<int> counter(0);
///
/// // Read the value (multiple threads can do this concurrently)
/// int value = counter([](const auto& val) { return val; });
///
/// // Modify the value (exclusive access)
/// counter([](auto& val) { val++; });
///
/// // Can also return values when modifying
/// bool wasZero = counter([](auto& val) {
///   bool result = (val == 0);
///   val = 42;
///   return result;
/// });
/// ```
///
/// @tparam ValueTypeT Type of value to protect.
template<typename ValueTypeT>
class Locked
{
public:
  using ValueType = typename std::remove_const_t<ValueTypeT>;

  /// @brief Constructs the protected value.
  /// @param args Arguments forwarded to value constructor.
  template<typename... Args>
  explicit Locked(Args&&... args): mLockedValue{ std::forward<Args>(args)... }
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
  /// Thread-safe assignment. Example: `locked = 42;`
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

  /// @brief Moves the value out, leaving it in moved-from state.
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
  /// Mutex to protect the mLockedValue.
  mutable std::shared_mutex mMutex;

  /// Guarded l-value.
  ValueType mLockedValue;
};

/// @brief Compares locked value with another value.
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

/// @brief Compares locked value with another value.
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

/// @brief Compares locked value with another value.
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

/// @brief Compares locked value with another value.
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

/// @brief Compares locked value with another value.
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

/// @brief Compares locked value with another value.
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

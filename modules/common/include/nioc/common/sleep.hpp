////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <chrono>
#include <semaphore>
#include <stop_token>

namespace nioc::common
{

/// @brief Sleep until @p deadline, returning early if @p stopToken is signalled.
///
/// The standard sleep functions cannot be interrupted, so a routine that uses them ignores shutdown
/// until they return. This waits the same way but stays responsive: a stop callback releases a
/// semaphore the instant a stop is requested, so the timed wait returns immediately instead of
/// running out the clock. Everything it needs (the semaphore and the callback) is allocated
/// locally, so the call is self-contained and safe to use from any thread.
///
/// @param stopToken The cooperative-cancellation token to honour.
///
/// @param deadline The instant to sleep until.
///
/// @return true if a stop was requested (the sleep ended early); false if @p deadline was reached.
template<typename Clock, typename Duration>
bool sleepUntil(
    const std::stop_token& stopToken,
    const std::chrono::time_point<Clock, Duration>& deadline)
{
  auto wakeUp = std::binary_semaphore{0};
  const auto onStop = std::stop_callback(stopToken, [&wakeUp] { wakeUp.release(); });
  return wakeUp.try_acquire_until(deadline) or stopToken.stop_requested();
}

/// @brief Sleep for @p duration, returning early if @p stopToken is signalled.
///
/// See @ref sleepUntil; this is the same, expressed as a relative duration.
///
/// @param stopToken The cooperative-cancellation token to honour.
///
/// @param duration How long to sleep.
///
/// @return true if a stop was requested (the sleep ended early); false if @p duration elapsed.
template<typename Rep, typename Period>
bool sleepFor(const std::stop_token& stopToken, const std::chrono::duration<Rep, Period>& duration)
{
  auto wakeUp = std::binary_semaphore{0};
  const auto onStop = std::stop_callback(stopToken, [&wakeUp] { wakeUp.release(); });
  return wakeUp.try_acquire_for(duration) or stopToken.stop_requested();
}

} // namespace nioc::common

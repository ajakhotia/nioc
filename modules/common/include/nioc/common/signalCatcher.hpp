////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <functional>
#include <ranges>
#include <thread>

namespace nioc::common
{

class SignalCatcher
{
public:
  using SignalAction = std::function<void(std::int32_t)>;

  template<typename... Args>
  explicit SignalCatcher(Args&&... args):
    mSignalActions{std::forward<Args>(args)...},
    mWatchThead([this]() { watch(); })
  {
    for(const auto signal: mSignalActions | std::views::keys)
    {
      setupHandler(signal);
    }
  }

  SignalCatcher(const SignalCatcher&) = delete;

  SignalCatcher(SignalCatcher&&) noexcept = delete;

  ~SignalCatcher();

  SignalCatcher& operator=(const SignalCatcher&) = delete;

  SignalCatcher& operator=(SignalCatcher&&) noexcept = delete;

private:
  const std::unordered_map<std::int32_t, SignalAction> mSignalActions;
  std::jthread mWatchThead;
  std::unordered_map<std::int32_t, std::int32_t> mSignalCounter;

  void watch();

  static void setupHandler(int signal);
};

} // namespace nioc::common

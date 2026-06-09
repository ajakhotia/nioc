////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

// NOLINTBEGIN -- temporary: whole-file clang-tidy suppression while debugging the watch loop.

#include <nioc/common/signalCatcher.hpp>

#include <atomic>
#include <csignal>
#include <iostream>
#include <ostream>
#include <ranges>

namespace nioc::common
{
namespace
{

constexpr auto kDefaultSignal = 0;
constexpr auto kStopSignal = -1;

std::atomic_int& signalCache()
{
  static auto signalCache = std::atomic_int{kDefaultSignal};
  return signalCache;
}

void signalHandler(const int signal)
{
  signalCache() = signal;
  signalCache().notify_one();
}

} // namespace

SignalCatcher::~SignalCatcher()
{
  for(const auto signal: mSignalActions | std::views::keys)
  {
    static_cast<void>(std::signal(signal, SIG_DFL));
  }

  static_cast<void>(mStop.request_stop());
  signalCache().store(kStopSignal, std::memory_order_relaxed);
  signalCache().notify_all();
  mWatchThead.join();
}

void SignalCatcher::watch()
{
  while(true)
  {
    signalCache().wait(kDefaultSignal, std::memory_order_relaxed);
    const auto cachedSignal = signalCache().exchange(kDefaultSignal, std::memory_order_relaxed);

    if (mStop.stop_requested())
    {
      break;
    }

    ++mSignalCounter[cachedSignal];

    auto signalWithAction = mSignalActions.find(cachedSignal);
    if(signalWithAction != mSignalActions.cend())
    {
      signalWithAction->second(mSignalCounter.at(cachedSignal));
    }
  }
}

void SignalCatcher::setupHandler(const int signal)
{
  static_cast<void>(std::signal(signal, signalHandler));
}

} // namespace nioc::common

// NOLINTEND

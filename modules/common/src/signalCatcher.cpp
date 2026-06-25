////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <atomic>
#include <csignal>
#include <nioc/common/signalCatcher.hpp>

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

void signalHandler(const std::int32_t signal)
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

  signalCache().store(kStopSignal, std::memory_order_relaxed);
  signalCache().notify_all();
  mWatchThread.join();
}

void SignalCatcher::watch()
{
  while(true)
  {
    signalCache().wait(kDefaultSignal, std::memory_order_relaxed);
    const auto cachedSignal = signalCache().exchange(kDefaultSignal, std::memory_order_relaxed);

    if(cachedSignal == kStopSignal)
    {
      break;
    }

    const auto count = ++mSignalCounter[cachedSignal];

    if(const auto signalWithAction = mSignalActions.find(cachedSignal);
       signalWithAction != mSignalActions.cend())
    {
      signalWithAction->second(count);
    }
  }
}

void SignalCatcher::setupHandler(const std::int32_t signal)
{
  static_cast<void>(std::signal(signal, signalHandler));
}

} // namespace nioc::common

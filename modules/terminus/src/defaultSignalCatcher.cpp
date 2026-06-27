////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <csignal>
#include <cstdint>
#include <nioc/terminus/defaultSignalCatcher.hpp>
#include <nioc/terminus/port.hpp>
#include <utility>

namespace nioc::terminus
{

common::SignalCatcher defaultSignalCatcher(Port& port)
{
  // The catcher counts deliveries per signal, so the escalation is per signal too: a SIGINT
  // following a SIGTERM requests another shutdown, not the abort.
  const auto shutdownThenAbort = [&port](const std::int32_t count)
  {
    if(count == 1)
    {
      port.shutdown();
    }
    if(count >= 2)
    {
      port.abort();
    }
  };

  const auto abortNow = [&port](const std::int32_t) { port.abort(); };

  return common::SignalCatcher{
      std::pair{SIGINT, shutdownThenAbort},
      std::pair{SIGTERM, shutdownThenAbort},
      std::pair{SIGABRT, abortNow}};
}

} // namespace nioc::terminus

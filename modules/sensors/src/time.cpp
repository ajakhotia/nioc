////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <chrono>
#include <nioc/sensors/time.hpp>

namespace nioc::sensors
{

NamedClock::time_point timePoint(const Timestamp::Reader timestamp)
{
  return NamedClock::time_point{NamedClock::duration(timestamp.getNanosecondSinceEpoch())};
}

} // namespace nioc::sensors

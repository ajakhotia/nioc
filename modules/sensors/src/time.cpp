////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <capnp/blob.h>
#include <chrono>
#include <nioc/sensors/time.hpp>
#include <string_view>

namespace nioc::sensors
{

NamedClock::time_point timePoint(const Timestamp::Reader reader)
{
  return NamedClock::time_point{NamedClock::duration(reader.getNanosecondSinceEpoch())};
}

void setTimePoint(
    Timestamp::Builder builder,
    const NamedClock::time_point timePoint,
    const std::string_view reference)
{
  builder.setNanosecondSinceEpoch(timePoint.time_since_epoch().count());
  builder.setReference(capnp::Text::Reader{reference.data(), reference.size()});
}

} // namespace nioc::sensors

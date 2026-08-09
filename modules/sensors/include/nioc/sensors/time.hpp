////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <chrono>
#include <nioc/common/time.hpp>
#include <nioc/sensors/idl/timestamp.capnp.h>
#include <string_view>

namespace nioc::sensors
{

/// @brief The instant @p reader records, as a nanosecond-resolution time point on the named
/// clock it was recorded against.
[[nodiscard]] common::NamedClock::time_point timePoint(Timestamp::Reader reader);

/// @brief Record @p timePoint into @p builder as its nanosecond count since the epoch, naming the
/// clock it is read against.
///
/// @param builder The Timestamp record to fill.
///
/// @param timePoint The instant to record.
///
/// @param reference The clock the instant is expressed in (for example "UTC", "GPS", "system");
/// empty marks it unspecified.
void setTimePoint(
    Timestamp::Builder builder,
    common::NamedClock::time_point timePoint,
    std::string_view reference);

} // namespace nioc::sensors

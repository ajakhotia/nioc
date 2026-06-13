////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <nioc/common/signalCatcher.hpp>

namespace nioc::terminus
{

class Port;

/// @brief Returns a SignalCatcher that drives a Port's two-stage shutdown.
///
/// The first SIGINT or SIGTERM shuts the port down gracefully (@ref Port::shutdown). A second one
/// halts it now (@ref Port::abort). SIGABRT halts it now. Keep the returned catcher alive while the
/// signals should drive @p port. Signal handlers are process-global, so keep at most one catcher at
/// a time.
///
/// @param port Port the signals drive. Must outlive the returned catcher.
[[nodiscard]] common::SignalCatcher defaultSignalCatcher(Port& port);

} // namespace nioc::terminus

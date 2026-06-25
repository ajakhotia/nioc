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

/// @brief Install the standard shutdown signal handlers for a port and return the running catcher
/// that keeps them active.
///
/// Wires SIGINT and SIGTERM to escalating shutdown and SIGABRT to immediate abort:
///   - the first SIGINT or SIGTERM calls `port.shutdown()`;
///   - a second delivery of the same signal calls `port.abort()`;
///   - SIGABRT calls `port.abort()` on every delivery.
/// The escalation count is per signal, so a SIGINT after a SIGTERM is a first delivery and shuts
/// down rather than aborts.
///
/// Example:
///
///     // Keep the catcher alive for the lifetime of the port's run loop.
///     auto catcher = defaultSignalCatcher(port);
///     port.run();
///
/// @param port Must outlive the returned catcher. Its `shutdown()` and `abort()` run on the
/// catcher's watcher thread, not in the OS signal handler.
///
/// @return A live @ref common::SignalCatcher that owns SIGINT, SIGTERM, and SIGABRT until it is
/// destroyed. Store it; discarding it uninstalls the handlers at once. Its process-global
/// and lossy-delivery constraints apply here too.
///
/// @see common::SignalCatcher
[[nodiscard]] common::SignalCatcher defaultSignalCatcher(Port& port);

} // namespace nioc::terminus

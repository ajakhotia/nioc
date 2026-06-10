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

/// @brief Returns a SignalCatcher wired to a Port's two-stage shutdown protocol.
///
/// The first SIGINT or SIGTERM requests a graceful shutdown (see @ref Port::shutdown) and a repeat
/// requests an immediate halt (see @ref Port::abort); SIGABRT halts immediately. Keep the returned
/// catcher alive for as long as the signals should drive the @p port — signal handlers are
/// process-global, so construct at most one catcher at a time.
///
/// @param port Hub the signal actions drive; must outlive the returned catcher.
[[nodiscard]] common::SignalCatcher defaultSignalCatcher(Port& port);

} // namespace nioc::terminus

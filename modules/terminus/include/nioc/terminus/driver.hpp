////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "msg.hpp"
#include "port.hpp"
#include <nioc/concurrent/routine.hpp>
#include <string>
#include <utility>

namespace nioc::terminus
{

/// @brief A source @ref Routine that publishes messages onto a @ref Port but receives none.
///
/// A Driver is the entry point for data that originates outside the process — a sensor reader, a
/// file reader, a synthetic generator. It owns no inbox: a subclass implements @ref run to
/// produce work and calls the method @ref publish to emit typed messages onto the Port. Contrast
/// @ref Component, which also receives messages through subscriptions.
///
/// Construct through a subclass. The Driver holds the Port by reference, so the Port must outlive
/// every Driver bound to it.
class Driver: public concurrent::Routine
{
public:
  Driver(const Driver&) = delete;
  Driver(Driver&&) noexcept = delete;
  Driver& operator=(const Driver&) = delete;
  Driver& operator=(Driver&&) noexcept = delete;
  ~Driver() noexcept override = default;

  /// @brief Runs one iteration: invokes @ref run and converts any failure into a clean finish.
  ///
  /// Catches every exception @ref run may throw, logs it, and reports @ref State::Done so a failing
  /// driver winds down gracefully rather than escalating. Never throws.
  ///
  /// @return Whatever @ref run returns, or @ref State::Done if @ref run throws.
  [[nodiscard]] State step() noexcept final;

protected:
  /// @brief Binds the driver to the Port it publishes onto.
  /// @param port Hub the driver publishes onto; must outlive this driver.
  /// @param name Human-readable identity for this driver (see @ref Routine::name).
  Driver(Port& port, std::string name);

  /// @brief Publishes a typed message onto the bound Port.
  ///
  /// @tparam Schema Cap'n Proto schema of the message.
  ///
  /// @param topic Topic the message is published on.
  ///
  /// @param msgPtr Message to publish; ownership passes to the Port.
  template<typename Schema>
  void publish(const std::string_view& topic, ConstMsgPtr<Schema> msgPtr)
  {
    mPort.publish(makeChannelId(Msg<Schema>::kMsgId, topic), std::move(msgPtr));
  }

private:
  Port& mPort;

  /// @brief Produces one iteration of work, emitting messages through the @ref publish method.
  ///
  /// A subclass implements this to generate and publish the next message(s). It is invoked once per
  /// iteration by the @ref step method.
  ///
  /// @return @ref State::Continue to be scheduled again immediately, @ref State::Waiting when no
  /// work is ready yet, or @ref State::Done when the source is exhausted.
  ///
  /// @throws std::exception A subclass may let any exception escape; the @ref step method catches
  /// it.
  [[nodiscard]] virtual State run() = 0;
};

} // namespace nioc::terminus

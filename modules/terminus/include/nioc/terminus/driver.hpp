////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "port.hpp"
#include <nioc/concurrent/routine.hpp>
#include <string>
#include <utility>

namespace nioc::terminus
{

/// @brief A source @ref Routine that publishes messages onto a @ref Port but receives none.
///
/// A Driver is the entry point for data that originates outside the process — a sensor reader, a
/// file reader, a synthetic generator. It owns no inbox: a subclass implements the @ref step to
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

protected:
  /// @brief Binds the driver to the Port it publishes onto.
  /// @param port Hub the driver publishes onto; must outlive this driver.
  /// @param name Human-readable identity for this driver (see @ref Routine::name).
  Driver(Port& port, std::string name): Routine(std::move(name)), mPort(port) {}

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
    mPort.publish<Schema>(topic, std::move(msgPtr));
  }

private:
  Port& mPort;
};

} // namespace nioc::terminus

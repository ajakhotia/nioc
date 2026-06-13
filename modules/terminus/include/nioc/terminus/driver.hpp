////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "msg.hpp"
#include "port.hpp"
#include <nioc/concurrent/routine.hpp>
#include <nioc/terminus/config/driverConfig.capnp.h>
#include <stop_token>
#include <string>
#include <utility>

namespace nioc::terminus
{

/// @brief A @ref Routine that publishes messages onto a @ref Port and receives none.
///
/// Use a Driver to bring outside data into the process: a sensor reader, a file reader, a
/// generator. A subclass implements @ref run to produce work and calls @ref publish to send typed
/// messages to the Port. A Driver has no inbox; use @ref Component instead if you also need to
/// receive messages.
///
/// Construct through a subclass. The Port is held by reference and must outlive the Driver.
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
  /// @param port Port to publish onto; must outlive this driver.
  /// @param name Name for this driver (see @ref Routine::name).
  Driver(Port& port, std::string name);

  /// @brief Configures the driver base from a config block.
  ///
  /// Reads the routine's name (see @ref Routine::name) from @p config. A subclass passes the
  /// `driver` subsection of its own config block here.
  ///
  /// @param port Port to publish onto; must outlive this driver.
  ///
  /// @param config The driver's config block (see driverConfig.capnp).
  ///
  /// @throws std::invalid_argument If the name is empty. The config must supply it.
  Driver(Port& port, DriverConfig::Reader config);

  /// @brief Returns the token set when the bound Port is asked to shut down.
  ///
  /// Check this token in @ref run. Once shutdown is requested, stop producing, finish in-flight
  /// work, and return @ref State::Done.
  [[nodiscard]] const std::stop_token& shutdownToken() const noexcept;

  /// @brief Publishes a typed message onto the bound Port.
  ///
  /// @tparam Schema Cap'n Proto schema of the message.
  ///
  /// @param topic Topic to publish on.
  ///
  /// @param msgPtr Message to publish. Ownership passes to the Port.
  template<typename Schema>
  void publish(const std::string_view& topic, ConstMsgPtr<Schema> msgPtr)
  {
    mPort.publish<Schema>(topic, std::move(msgPtr));
  }

private:
  Port& mPort;
  const std::stop_token mShutdownToken;

  /// @brief Runs one iteration by calling @ref run, turning any failure into a clean finish.
  ///
  /// Catches and logs any exception from @ref run, then returns @ref State::Done so a failing
  /// driver stops cleanly. Never throws.
  ///
  /// @return Whatever @ref run returns, or @ref State::Done if @ref run throws.
  [[nodiscard]] State step() noexcept final;

  /// @brief Produces one iteration of work, sending messages via @ref publish.
  ///
  /// Implement this to generate and publish the next message(s). Called once per iteration.
  ///
  /// @return @ref State::Continue to run again right away, @ref State::Waiting when no work is
  /// ready yet, or @ref State::Done when the source is exhausted.
  ///
  /// @throws std::exception Any exception may escape; @ref step catches it.
  [[nodiscard]] virtual State run() = 0;
};

} // namespace nioc::terminus

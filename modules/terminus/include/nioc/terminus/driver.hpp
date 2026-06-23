////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "port.hpp"
#include "publisher.hpp"
#include <nioc/concurrent/routine.hpp>
#include <nioc/terminus/config/driverConfig.capnp.h>
#include <stop_token>
#include <string>
#include <string_view>

namespace nioc::terminus
{

/// @brief A @ref Routine that publishes messages onto a @ref Port and receives none.
///
/// Use a Driver to bring outside data into the process: a sensor reader, a file reader, a
/// generator. A subclass mints @ref Publisher handles with @ref publisher at construction,
/// implements @ref run to produce work, and publishes through those handles. A Driver has no inbox;
/// use @ref Component instead if you also need to receive messages.
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

  /// @brief Returns the Port this driver publishes onto.
  ///
  /// Most drivers produce through @ref publisher handles; reach for the Port directly only to
  /// deliver pre-built frames a typed publisher cannot mint, as a @ref LogPlayer does on replay.
  [[nodiscard]] Port& port() noexcept;

  /// @brief Mints a producer handle for a topic.
  ///
  /// @tparam Schema Cap'n Proto schema of the messages.
  ///
  /// @param topic Topic to publish on.
  ///
  /// @return A @ref Publisher bound to the topic.
  template<typename Schema>
  [[nodiscard]] Publisher<Schema> publisher(const std::string_view& topic)
  {
    return mPort.publisher<Schema>(topic);
  }

private:
  Port& mPort;
  const std::stop_token mShutdownToken;

  /// @brief Runs one iteration by calling @ref run, turning any failure into a clean finish.
  ///
  /// @return Whatever @ref run returns, or @ref State::Done if @ref run throws.
  [[nodiscard]] State step() noexcept final;

  /// @brief Produces one iteration of work, publishing through held @ref Publisher handles.
  ///
  /// @return @ref State::Continue to run again right away, @ref State::Waiting when no work is
  /// ready yet, or @ref State::Done when the source is exhausted.
  ///
  /// @throws std::exception Any exception may escape; @ref step catches it.
  [[nodiscard]] virtual State run() = 0;
};

} // namespace nioc::terminus

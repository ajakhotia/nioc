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

/// @brief Abstract base for a source routine that originates data and publishes it onto a Port.
///
/// A Driver is the producer end of a run. Unlike a message-driven Component, it has no inbox: it
/// pulls from a sensor, file, or clock and publishes the result. Derive from Driver, supply a name,
/// open publishers with @ref publisher, and implement @ref run to do one non-blocking slice of work
/// per tick. The driving Runner ticks the driver until @ref run reports `State::Done`.
///
/// Example:
///
///     class ClockDriver : public Driver
///     {
///     public:
///       ClockDriver(Port& port) : Driver(port, "clock"), mPub(publisher<TickSchema>("tick")) {}
///     private:
///       State run() override
///       {
///         if(shutdownToken().stop_requested()) return State::Done;
///         auto draft = mPub.draft();
///         // ... fill the draft ...
///         mPub.publish(draft.seal());
///         return State::Continue;
///       }
///       Publisher<TickSchema> mPub;
///     };
///
/// Non-copyable and non-movable. The owning Port holds the driver via `shared_ptr` and tears it
/// down while winding the run down, so the Port always outlives the driver that references it.
///
/// @see Port, Publisher, concurrent::Routine
class Driver: public concurrent::Routine
{
public:
  Driver(const Driver&) = delete;
  Driver(Driver&&) noexcept = delete;
  Driver& operator=(const Driver&) = delete;
  Driver& operator=(Driver&&) noexcept = delete;
  ~Driver() noexcept override = default;

protected:
  /// @brief Construct bound to @p port and labeled @p name.
  ///
  /// @param port The Port this driver publishes onto. Must outlive the driver; held by reference.
  ///
  /// @param name Identifying label, fixed for the driver's lifetime.
  Driver(Port& port, std::string name);

  /// @brief Construct bound to @p port, taking the driver's name from @p config.
  ///
  /// @param port The Port this driver publishes onto. Must outlive the driver; held by reference.
  ///
  /// @param config Driver configuration; its `name` field becomes the driver's name.
  ///
  /// @throws std::invalid_argument If the config's name is empty.
  Driver(Port& port, DriverConfig::Reader config);

  /// @brief The run's cooperative shutdown token; becomes stopped when the run begins winding down.
  ///
  /// Poll it from @ref run and return `State::Done` once a stop is requested. The reference stays
  /// valid for the driver's lifetime.
  [[nodiscard]] const std::stop_token& shutdownToken() const noexcept;

  /// @brief The Port this driver publishes onto; outlives the driver.
  [[nodiscard]] Port& port() noexcept;

  /// @brief Open a publisher for @p topic carrying messages of @p Schema, recording the topic on
  /// the run.
  ///
  /// @tparam Schema The Cap'n Proto message schema; must be named explicitly.
  ///
  /// @param topic Topic name. The `(Schema, topic)` pair identifies one channel.
  ///
  /// @return A publisher bound to this driver's Port and channel. It borrows from the Port and must
  /// not outlive the driver.
  ///
  /// @throws std::logic_error If the run does not record a chronicle.
  ///
  /// @see Port::publisher
  template<typename Schema>
  [[nodiscard]] Publisher<Schema> publisher(const std::string_view& topic)
  {
    return mPort.publisher<Schema>(topic);
  }

private:
  /// The Port this driver publishes onto. Held by reference; the Port outlives the driver.
  Port& mPort;

  /// The run's cooperative shutdown token, captured at construction and stopped when the run winds
  /// down. Exposed to subclasses through @ref shutdownToken.
  const std::stop_token mShutdownToken;

  /// @brief Drive one tick: invoke @ref run, translate any thrown exception into `State::Done`, and
  /// report the resulting State to the Runner.
  ///
  /// This is the Routine hook the Runner calls; it never throws. Catches any exception from
  /// @ref run, logs it under the driver's name, and reports `State::Done` so the driver ends
  /// cleanly.
  ///
  /// @return The State returned by @ref run, or `State::Done` if @ref run threw.
  [[nodiscard]] State step() noexcept final;

  /// @brief Produce one non-blocking slice of work and report what to do next.
  ///
  /// Override to implement the driver. Called serially by the driving Runner.
  ///
  /// @return `State::Continue` when more work is ready now, `State::Waiting` when none is ready, or
  /// `State::Done` when finished.
  ///
  /// @note May throw. The driver's `step()` catches any exception, logs it under the driver's name,
  /// and ends the driver by reporting `State::Done`.
  [[nodiscard]] virtual State run() = 0;
};

} // namespace nioc::terminus

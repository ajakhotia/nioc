////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <atomic>
#include <cstddef>
#include <nioc/sensors/idl/imu.capnp.h>
#include <nioc/sensors/idl/pointCloud.capnp.h>
#include <nioc/terminus/component.hpp>
#include <nioc/terminus/msg.hpp>
#include <nioc/terminus/port.hpp>
#include <nioc/terminus/subscription.hpp>
#include <string>
#include <string_view>

namespace nioc::example
{

/// @brief Example component that subscribes to Imu and PointCloud topics and tallies what arrives.
///
/// The tallies are bumped from the subscription callbacks (invoked on Port's dispatch thread). The
/// iteration loop is inherited from @ref terminus::Component. A minimal, readable template for a
/// subscribing component.
class MessageCounter final: public terminus::Component
{
public:
  /// @brief Subscribes to the Imu and PointCloud topics on @p port.
  ///
  /// @param port Port the subscriptions are registered with; must outlive this component.
  ///
  /// @param imuTopic Topic carrying @ref nioc::sensors::Imu messages.
  ///
  /// @param pointCloudTopic Topic carrying @ref nioc::sensors::PointCloud messages.
  ///
  /// @param inboxCapacity Maximum number of undrained inbound messages held by the component's
  /// inbox.
  ///
  /// @param overflowPolicy What the inbox does when a message arrives while full.
  MessageCounter(
      terminus::Port& port,
      std::string imuTopic,
      std::string pointCloudTopic,
      std::size_t inboxCapacity,
      terminus::OverflowPolicy overflowPolicy);

  [[nodiscard]] std::string_view name() const final;

  /// @brief Returns the number of Imu messages received so far.
  [[nodiscard]] std::uint64_t imuCount() const noexcept;

  /// @brief Returns the number of PointCloud messages received so far.
  [[nodiscard]] std::uint64_t pointCloudCount() const noexcept;

private:
  void onImu(const terminus::Msg<sensors::Imu>& message);

  void onPointCloud(const terminus::Msg<sensors::PointCloud>& message);

  std::string mImuTopic;
  std::string mPointCloudTopic;
  std::atomic<std::uint64_t> mImuCount{ 0 };
  std::atomic<std::uint64_t> mPointCloudCount{ 0 };

  // Declared last so they are torn down first: each Subscription unsubscribes (and waits out any
  // in-flight delivery) before the counters it touches are destroyed.
  terminus::Subscription mImuSubscription;
  terminus::Subscription mPointCloudSubscription;
};

} // namespace nioc::example

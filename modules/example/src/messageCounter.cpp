////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <nioc/example/messageCounter.hpp>
#include <utility>

namespace nioc::example
{

MessageCounter::MessageCounter(
    terminus::Port& port,
    std::string imuTopic,
    std::string pointCloudTopic,
    const std::size_t inboxCapacity,
    const terminus::OverflowPolicy overflowPolicy):
    terminus::Component{ inboxCapacity, overflowPolicy },
    mImuTopic{ std::move(imuTopic) },
    mPointCloudTopic{ std::move(pointCloudTopic) },
    mImuSubscription{ port.subscribe<sensors::Imu>(mImuTopic, this, &MessageCounter::onImu) },
    mPointCloudSubscription{
      port.subscribe<sensors::PointCloud>(mPointCloudTopic, this, &MessageCounter::onPointCloud)
    }
{
}

std::string_view MessageCounter::name() const
{
  return "MessageCounter";
}

std::uint64_t MessageCounter::imuCount() const noexcept
{
  return mImuCount.load();
}

std::uint64_t MessageCounter::pointCloudCount() const noexcept
{
  return mPointCloudCount.load();
}

void MessageCounter::onImu(const terminus::Msg<sensors::Imu>& /*message*/)
{
  ++mImuCount;
}

void MessageCounter::onPointCloud(const terminus::Msg<sensors::PointCloud>& /*message*/)
{
  ++mPointCloudCount;
}

} // namespace nioc::example

////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <cstddef>
#include <filesystem>
#include <memory>
#include <nioc/concurrent/asyncProcessor.hpp>
#include <nioc/concurrent/threadedRunner.hpp>
#include <nioc/logger/logger.hpp>
#include <nioc/terminus/asyncChronicleWriter.hpp>
#include <utility>

namespace nioc::terminus
{
namespace fs = std::filesystem;

namespace
{

/// Inbox capacity. Applies only under a bounded BufferMode; AsyncProcessor ignores it under
/// BufferMode::Unbounded, which this writer uses.
constexpr auto kQueueCapacity = std::size_t{1024};

/// Creates @p dir if it does not exist and returns it, so a chronicle::Writer can be initialized
/// from it directly in a member initializer.
fs::path ensureDirectory(const fs::path& dir)
{
  fs::create_directories(dir);
  return dir;
}

} // namespace

AsyncChronicleWriter::AsyncChronicleWriter(const fs::path& chronicleDir):
  mWriter{ensureDirectory(chronicleDir)},
  mProcessor{std::make_shared<Processor>(
      "chronicleWriter",
      concurrent::BufferMode::Unbounded,
      kQueueCapacity,
      [this](const auto& item)
      {
        write(*item.second.mMsgBasePtr, item.first, mWriter);
      })},
  mRunner{std::make_shared<concurrent::ThreadedRunner>()}
{
  mRunner->launch(mProcessor);
  logger::debug("chronicle writer started");
}

AsyncChronicleWriter::~AsyncChronicleWriter()
{
  mRunner->requestStop();
  mRunner->waitUntilStopped();
}

void AsyncChronicleWriter::push(const ChannelId channelId, Consignment consignment)
{
  processor().push({channelId, std::move(consignment)});
}

AsyncChronicleWriter::Processor& AsyncChronicleWriter::processor()
{
  return *mProcessor;
}

const AsyncChronicleWriter::Processor& AsyncChronicleWriter::processor() const
{
  return *mProcessor;
}

} // namespace nioc::terminus

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

/// Inbox capacity used if the writer ever runs bounded. It currently runs BufferMode::Unbounded
/// (lossless, no backpressure), so this value is presently ignored.
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
        logger::trace("writing channel {} to chronicle", item.first.mValue);
        write(*item.second, item.first, mWriter);
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

void AsyncChronicleWriter::push(const ChannelId channelId, const ConstMsgBasePtr& msgPtr)
{
  processor().push(std::make_pair(channelId, msgPtr));
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

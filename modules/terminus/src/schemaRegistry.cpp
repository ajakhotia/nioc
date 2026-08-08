////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <capnp/message.h>
#include <capnp/serialize.h>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <kj/exception.h>
#include <nioc/common/exception.hpp>
#include <nioc/common/utils.hpp>
#include <nioc/containers/mmapConstArray.hpp>
#include <nioc/logger/logger.hpp>
#include <nioc/terminus/arenaMessageBuilder.hpp>
#include <nioc/terminus/idl/schemaClosure.capnp.h>
#include <nioc/terminus/schemaRegistry.hpp>
#include <nioc/terminus/utils.hpp>
#include <ranges>
#include <span>
#include <stdexcept>
#include <utility>

namespace nioc::terminus
{
namespace fs = std::filesystem;

namespace
{

// The schemas file: a single flat-array frame rooted at a SchemaClosure. The single definition
// both write() and the loading constructor read.
constexpr auto kFileName = "schemas.bin";

} // namespace

SchemaRegistry::SchemaRegistry(const fs::path& recording)
{
  // A live run names no input log; there is nothing to adopt.
  if(recording.empty())
  {
    return;
  }

  const auto schemasPath = recording / kFileName;
  try
  {
    if(not fs::is_regular_file(schemasPath))
    {
      common::throwException<std::runtime_error>("The recording carries no {} file.", kFileName);
    }

    // The file is the flat serialization of one message; map it and read the frame in place.
    const auto mapping = containers::MmapConstArray<std::byte>{schemasPath};
    auto reader = capnp::FlatArrayMessageReader{
        asWords(std::span<const std::byte>{mapping.data(), mapping.size()})};

    const auto nodes = reader.getRoot<SchemaClosure>().getNodes();
    for(const auto node: nodes)
    {
      mLoader.load(node);
    }
  }
  catch(const kj::Exception& error)
  {
    logger::error(
        "Could not adopt schemas from {}; the replay continues without dynamic reading. Cause: {}",
        recording.string(),
        error.getDescription().cStr());
  }
  catch(const std::exception& error)
  {
    // Schemas that cannot be read cost dynamic reading, not the replay: the timeline still
    // delivers by channel. Report it and carry on with whatever was read.
    logger::error(
        "Could not adopt schemas from {}; the replay continues without dynamic reading. Cause: {}",
        recording.string(),
        error.what());
  }
}

void SchemaRegistry::write(const fs::path& directory) const
{
  const auto loaded = mLoader.getAllLoaded();

  auto builder = capnp::MallocMessageBuilder{};
  auto nodes = builder.initRoot<SchemaClosure>().initNodes(
      static_cast<unsigned int>(loaded.size()));

  for(const auto& [index, schema]: std::views::enumerate(loaded))
  {
    nodes.setWithCaveats(static_cast<unsigned int>(index), schema.getProto());
  }

  flattenAndWrite(builder, directory / kFileName);
}

bool SchemaRegistry::contains(const std::uint64_t schemaId) const
{
  return mLoader.tryGet(schemaId) != nullptr;
}

capnp::StructSchema SchemaRegistry::at(const std::uint64_t schemaId) const
{
  auto lookup = mLoader.tryGet(schemaId);
  if(lookup == nullptr)
  {
    common::throwException<std::out_of_range>(
        "No schema is recorded under id {}.",
        common::hexString(schemaId));
  }

  // The null-schema default is unreachable; the lookup was just checked to hold a value.
  const auto schema = std::move(lookup).orDefault(capnp::Schema{});
  if(not schema.getProto().isStruct())
  {
    common::throwException<std::invalid_argument>(
        "The schema recorded under id {} ('{}') is not a struct schema.",
        common::hexString(schemaId),
        schema.getProto().getDisplayName().cStr());
  }

  return schema.asStruct();
}

} // namespace nioc::terminus

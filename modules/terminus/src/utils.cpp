////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <capnp/compat/json.h>
#include <capnp/dynamic.h>
#include <capnp/message.h>
#include <capnp/schema.h>
#include <memory>
#include <nioc/common/exception.hpp>
#include <nioc/terminus/utils.hpp>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace nioc::terminus
{

nlohmann::json makeDefaultJson(const capnp::StructSchema schema)
{
  const auto codec = capnp::JsonCodec{};

  // A default-initialized message sets no fields, so reading any field yields its schema default.
  auto defaultsMessage = capnp::MallocMessageBuilder{};
  const auto rootStruct = defaultsMessage.initRoot<capnp::DynamicStruct>(schema).asReader();

  // A struct yet to be walked, paired with where its fields land in the JSON tree.
  struct PendingStruct
  {
    capnp::DynamicStruct::Reader mStruct;
    nlohmann::json::json_pointer mTreePath;
  };

  auto defaults = nlohmann::json::object();
  auto unwalkedStructs = std::vector<PendingStruct>{};
  unwalkedStructs.push_back(
      PendingStruct{.mStruct = rootStruct, .mTreePath = nlohmann::json::json_pointer{}});

  while(not unwalkedStructs.empty())
  {
    const auto current = unwalkedStructs.back();
    unwalkedStructs.pop_back();

    for(const auto& field: current.mStruct.getSchema().getFields())
    {
      const auto fieldPath = current.mTreePath / field.getProto().getName().cStr();
      if(field.getType().isStruct())
      {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
        defaults[fieldPath] = nlohmann::json::object();
        unwalkedStructs.push_back(
            PendingStruct{
                .mStruct = current.mStruct.get(field).as<capnp::DynamicStruct>(),
                .mTreePath = fieldPath});
      }
      else
      {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
        defaults[fieldPath] = nlohmann::json::parse(
            codec.encode(current.mStruct.get(field), field.getType()).cStr());
      }
    }
  }

  return defaults;
}

void overrideField(
    nlohmann::json& tree,
    const nlohmann::json::json_pointer& fieldPath,
    nlohmann::json value)
{
  if(not tree.contains(fieldPath))
  {
    common::throwException<std::out_of_range>(
        "Cannot override a field that is not already present: {}",
        fieldPath.to_string());
  }

  auto patch = nlohmann::json::object();
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
  patch[fieldPath] = std::move(value);
  tree.merge_patch(patch);
}

std::unique_ptr<capnp::MallocMessageBuilder> decodeMessage(
    const std::string& jsonText,
    const capnp::StructSchema schema)
{
  auto codec = capnp::JsonCodec{};

  // Tolerate fields outside the schema (e.g. ones written by a newer build) instead of failing the
  // decode, so the same text decodes across schema versions.
  codec.setRejectUnknownFields(false);

  auto message = std::make_unique<capnp::MallocMessageBuilder>();
  codec.decode(
      kj::ArrayPtr<const char>{jsonText.data(), jsonText.size()},
      message->initRoot<capnp::DynamicStruct>(schema));
  return message;
}

} // namespace nioc::terminus

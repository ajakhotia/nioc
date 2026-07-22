////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <capnp/compat/json.h>
#include <capnp/dynamic.h>
#include <capnp/message.h>
#include <capnp/schema.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <nioc/common/exception.hpp>
#include <nioc/terminus/utils.hpp>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace nioc::terminus
{

nlohmann::json readJsonFile(const std::filesystem::path& path)
{
  auto file = std::ifstream(path);
  if(not file)
  {
    common::throwException<std::runtime_error>("Cannot open JSON file: {}", path.string());
  }

  return nlohmann::json::parse(file);
}

void writeJsonFile(const std::filesystem::path& path, const nlohmann::json& json)
{
  auto file = std::ofstream(path);
  if(not file)
  {
    common::throwException<std::runtime_error>("Cannot write {}", path.string());
  }

  file << json.dump(2) << '\n';
}

nlohmann::json encodeAsJson(const capnp::StructSchema schema)
{
  const auto codec = capnp::JsonCodec{};

  // A default-initialized message sets no fields, so reading any field yields its schema default.
  auto defaultsMessage = capnp::MallocMessageBuilder{};
  const auto rootStruct = defaultsMessage.initRoot<capnp::DynamicStruct>(schema).asReader();

  // Walk the tree field by field rather than encoding the whole struct at once: a whole-struct
  // encode of this bare message would drop its null pointer fields (nested structs, lists, text)
  // instead of materializing their defaults. Encoding each field on its own forces every default to
  // appear.

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

nlohmann::json encodeAsJson(capnp::MessageBuilder& builder, const capnp::StructSchema schema)
{
  const auto codec = capnp::JsonCodec{};
  return nlohmann::json::parse(
      codec.encode(builder.getRoot<capnp::DynamicStruct>(schema).asReader()).cStr());
}

std::unique_ptr<capnp::MallocMessageBuilder> decodeFromJson(
    const std::string& jsonText,
    const capnp::StructSchema schema)
{
  auto codec = capnp::JsonCodec{};

  // Tolerate fields outside the schema (e.g. ones written by a newer build) instead of failing the
  // decode, so the same text decodes across schema versions.
  codec.setRejectUnknownFields(false);

  auto builder = std::make_unique<capnp::MallocMessageBuilder>();
  codec.decode(
      kj::ArrayPtr<const char>{jsonText.data(), jsonText.size()},
      builder->initRoot<capnp::DynamicStruct>(schema));
  return builder;
}

} // namespace nioc::terminus

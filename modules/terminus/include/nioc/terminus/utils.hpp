////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <algorithm>
#include <capnp/any.h>
#include <capnp/dynamic.h>
#include <capnp/message.h>
#include <capnp/schema.h>
#include <filesystem>
#include <functional>
#include <iterator>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nioc::terminus
{

/// @brief Parse the JSON file at @p path.
///
/// @param path The JSON file to read.
///
/// @return The parsed document.
///
/// @throws std::runtime_error if @p path cannot be opened.
///
/// @throws nlohmann::json::parse_error if the file is not valid JSON.
[[nodiscard]] nlohmann::json readJsonFile(const std::filesystem::path& path);

/// @brief Write @p json to @p path as pretty-printed text with a trailing newline, overwriting any
/// existing file.
///
/// @param path The destination file.
///
/// @param json The document to write.
///
/// @throws std::runtime_error if @p path cannot be opened for writing.
void writeJsonFile(const std::filesystem::path& path, const nlohmann::json& json);

/// @brief Encode a Cap'n Proto struct as a JSON tree mirroring its shape: every field explicit,
/// nested structs as nested objects, 64-bit integers as quoted strings.
///
/// This overload encodes @p schema's defaults, materializing every field with its schema default.
/// (The sibling overload encodes a supplied message's values instead.)
///
/// Example, given the following capnp schema:
///
///     struct Inner {
///         value @0 :Int64 = 3;
///         tag   @1 :Text  = "leaf";
///     }
///     struct Outer {
///         name  @0 :Text;
///         count @1 :UInt32 = 7;
///         inner @2 :Inner  = (value = 11);   # struct-literal default; tag left unset
///     }
///
/// `encodeAsJson(capnp::Schema::from<Outer>())` returns:
///
///     {
///       "name": "",
///       "count": 7,
///       "inner": { "value": "11", "tag": "leaf" }
///     }
///
/// @param schema The Cap'n Proto struct schema whose defaults to encode.
///
/// @return A JSON object mirroring @p schema, every field populated with its default value.
[[nodiscard]] nlohmann::json encodeAsJson(capnp::StructSchema schema);

/// @brief Encode @p builder's struct against @p schema as a JSON tree, every field explicit.
///
/// The value counterpart to the defaults overload above: it encodes the values actually held in
/// @p builder rather than the schema defaults.
///
/// @param builder Holds the struct to encode; read through @p schema.
///
/// @param schema The Cap'n Proto struct schema to encode against.
///
/// @return A JSON object of @p builder's values.
[[nodiscard]] nlohmann::json encodeAsJson(
    capnp::MessageBuilder& builder,
    capnp::StructSchema schema);

/// @brief Re-root @p builder's message into a single contiguous segment.
///
/// The serialized size of the message bounds its collapsed size, so the rebuild's first segment is
/// sized to land the whole re-root in one segment. The segment lives in the returned builder's
/// memory and is reachable through its `getSegmentsForOutput()`; keep the builder alive for as
/// long as the segment is in use.
///
/// @param builder The message to collapse; read through its root and left unchanged.
///
/// @return The rebuilt message, holding the whole re-root as its one segment.
///
/// @throws std::logic_error If the re-root does not collapse to a single segment.
[[nodiscard]] std::unique_ptr<capnp::MallocMessageBuilder> collapseToSingleSegment(
    capnp::MessageBuilder& builder);

/// @brief Flatten @p builder into a single segment and write it to @p binPath as a bare flat-array
/// frame, overwriting any existing file.
///
/// The frame on disk is word-aligned and self-contained, so it reads back by mapping the file and
/// rooting a `capnp::FlatArrayMessageReader` over its words (see @ref asWords). The config
/// artifacts and the schema registry both persist through this.
///
/// @param builder The message to write; read through its root and left unchanged.
///
/// @param binPath The destination file.
///
/// @throws std::logic_error If the message does not collapse to a single segment.
///
/// @throws std::runtime_error If the file cannot be created or sized.
void flattenAndWrite(capnp::MessageBuilder& builder, const std::filesystem::path& binPath);

/// @brief Decode JSON text into a typed Cap'n Proto message conforming to @p schema, the inverse
/// direction of @ref encodeAsJson.
///
/// Fields present in @p jsonText but outside @p schema are ignored rather than rejected, so the
/// same text decodes across schema versions. The returned message owns the decoded data; read it
/// through `getRoot<...>(schema)`.
///
/// @param jsonText The JSON document to decode.
///
/// @param schema The Cap'n Proto struct schema to decode against.
///
/// @return A message owning the struct decoded from @p jsonText.
[[nodiscard]] std::unique_ptr<capnp::MallocMessageBuilder> decodeFromJson(
    const std::string& jsonText,
    capnp::StructSchema schema);

/// @brief Resolve the dotted @p fieldPath through @p schema into the chain of field handles that
/// reaches the nested field, or nothing if a segment is missing or a non-terminal segment is not
/// a struct.
///
/// Resolution walks the schema's metadata alone; no message is needed. Each handle in the chain
/// descends one level, so a reader holding the chain reaches the leaf with index lookups rather
/// than name lookups. @ref dynamicFieldExtractor packages exactly that walk into a callable.
///
/// @param schema The struct schema the path descends from.
///
/// @param fieldPath Dot-separated field names, e.g. "header.timestamp.nanosecondSinceEpoch".
[[nodiscard]] std::optional<std::vector<capnp::StructSchema::Field>> buildFieldNodeChain(
    capnp::StructSchema schema,
    std::string_view fieldPath);

/// @brief Reflect over @p schema for the dotted @p fieldPath and produce the extractor that reads
/// that field out of a message of that schema, or nothing if the schema does not carry the path.
///
/// Resolution happens here, once per schema, by walking the schema's own metadata; no message is
/// needed. The returned extractor holds the resolved field handles, so each per-message read is a
/// chain of index lookups rather than name lookups. The leaf is read as @p Value through Cap'n
/// Proto's dynamic conversion, which throws on a type mismatch at read time.
///
/// @tparam Value The type to read the leaf field as, e.g. `std::int64_t`.
///
/// @param schema The struct schema describing the message payload.
///
/// @param fieldPath Dot-separated field names from the payload root to the leaf, e.g.
/// "header.timestamp.nanosecondSinceEpoch". Every non-terminal segment must be a struct field.
template<typename Value>
[[nodiscard]] std::optional<std::function<Value(capnp::AnyPointer::Reader)>> dynamicFieldExtractor(
    const capnp::StructSchema schema,
    const std::string_view fieldPath)
{
  auto chain = buildFieldNodeChain(schema, fieldPath);
  if(not chain.has_value())
  {
    return std::nullopt;
  }

  return std::function<Value(capnp::AnyPointer::Reader)>{
      [schema, chain = std::move(*chain)](const capnp::AnyPointer::Reader reader)
      {
        auto node = reader.getAs<capnp::DynamicStruct>(schema);
        std::for_each(
            chain.cbegin(),
            std::prev(chain.cend()),
            [&node](const capnp::StructSchema::Field& field)
            { node = node.get(field).as<capnp::DynamicStruct>(); });

        return node.get(chain.back()).as<Value>();
      }};
}

} // namespace nioc::terminus

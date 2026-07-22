////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <capnp/message.h>
#include <capnp/schema.h>
#include <filesystem>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

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

} // namespace nioc::terminus

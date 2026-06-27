////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <capnp/message.h>
#include <capnp/schema.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

namespace nioc::terminus
{

/// @brief Render a Cap'n Proto struct schema's default values as a JSON tree mirroring its shape.
///
/// Every field appears explicitly with its schema default; nested structs become nested objects.
/// 64-bit integers are emitted as quoted strings.
///
/// Example — given the following capnp schema:
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
/// `makeDefaultJson(capnp::Schema::from<Outer>())` returns:
///
///     {
///       "name": "",
///       "count": 7,
///       "inner": { "value": "11", "tag": "leaf" }
///     }
///
/// @param schema The Cap'n Proto struct schema to materialize.
///
/// @return A JSON object mirroring @p schema, every field populated with its default value.
[[nodiscard]] nlohmann::json makeDefaultJson(capnp::StructSchema schema);

/// @brief Decode JSON text into a typed Cap'n Proto message conforming to @p schema — the inverse
/// direction to @ref makeDefaultJson.
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
[[nodiscard]] std::unique_ptr<capnp::MallocMessageBuilder> decodeMessage(
    const std::string& jsonText,
    capnp::StructSchema schema);

/// @brief Override an existing field, at an arbitrarily deep path, in a JSON tree.
///
/// @p value is applied as a JSON merge-patch, so a `null` value deletes the field. The field must
/// already be present: this throws rather than creating it, catching typos and stale keys.
///
/// Example — for `tree`:
///
///     { "log": { "level": "info", "file": "out.txt" } }
///
///     overrideField(tree, "/log/level"_json_pointer, "debug");  // log.level becomes "debug"
///     overrideField(tree, "/log/file"_json_pointer,  nullptr);  // log.file is removed
///     overrideField(tree, "/log/rate"_json_pointer,  5);        // throws: /log/rate is absent
///
/// @param tree The JSON object to modify in place.
///
/// @param fieldPath JSON Pointer to the field to override.
///
/// @param value The new value, applied as a merge-patch; a `null` deletes the field.
///
/// @throws std::out_of_range if @p fieldPath is not already present in the @p tree.
void overrideField(
    nlohmann::json& tree,
    const nlohmann::json::json_pointer& fieldPath,
    nlohmann::json value);

} // namespace nioc::terminus

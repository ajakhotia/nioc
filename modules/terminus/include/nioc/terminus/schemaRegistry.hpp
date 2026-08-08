////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <capnp/schema-loader.h>
#include <capnp/schema.h>
#include <cstdint>
#include <filesystem>

namespace nioc::terminus
{

/// @brief The schemas a recording carries, and the sole owner of the `schemas.bin` convention.
///
/// The counterpart of @ref TopicRegistry: where the topic registry records which schema each
/// channel carries (by id), this registry records what those schemas *are*, as serialized Cap'n
/// Proto schema nodes. Together they let a reader that compiled against none of the payload types
/// reconstruct live schemas and read a recording's messages dynamically. On disk the file is a
/// flat-array frame rooted at the `SchemaClosure` struct of `idl/schemaClosure.capnp`, which is
/// the format's definition.
///
/// A recording run default-constructs an empty registry and @ref record "records" each publisher's
/// schema as the graph is wired; @ref write then persists the accumulated closure beside
/// `topics.json`. A replay adopts a recording's registry through the loading constructor and joins
/// it to channels via the schema ids in the topic registry, reading live schemas back with
/// @ref at.
///
/// The unit recorded is a schema's transitive closure, not the schema alone: a serialized node
/// references its dependencies only by id, so @ref record folds in every node the schema reaches
/// (its header, timestamp, geometry types, ...). Closures recorded across publishers share nodes,
/// which are stored once, keyed by their stable 64-bit type ids.
///
/// Loading twice under one id keeps the newer version: the underlying loader compares the two
/// against Cap'n Proto's schema-evolution rules and prefers the one that extends the other, so a
/// binary whose compiled schema has outgrown a recording's reads through its own richer view (the
/// recorded data simply lacks the new fields, which read as defaults). Incompatible versions are
/// refused with an error.
///
/// @see TopicRegistry, SchemaClosure, capnp::SchemaLoader
class SchemaRegistry
{
public:
  /// @brief An empty registry, as a live run starts with.
  SchemaRegistry() = default;

  /// @brief Adopt the schemas of the recording at @p recording, if it has any.
  ///
  /// An empty @p recording (a live run naming no input log) yields an empty registry. A
  /// @p recording that carries no readable, well-formed `schemas.bin` also yields an empty
  /// registry, with the reason logged: a replay can still deliver by channel, so this is a loss of
  /// dynamic reading, not a failed run.
  ///
  /// @param recording Path to a recording directory, or an empty path.
  explicit SchemaRegistry(const std::filesystem::path& recording);

  SchemaRegistry(const SchemaRegistry&) = delete;

  SchemaRegistry(SchemaRegistry&&) noexcept = delete;

  ~SchemaRegistry() noexcept = default;

  SchemaRegistry& operator=(const SchemaRegistry&) = delete;

  SchemaRegistry& operator=(SchemaRegistry&&) noexcept = delete;

  /// @brief Record @p Schema and its transitive closure, deduplicated against everything already
  /// recorded.
  ///
  /// Called by `Port::publisher` for each publisher's payload schema; the compiled type is
  /// required, which is why this lives on the recording side only. On a registry adopted from a
  /// recording, this instead overlays the compiled version onto the recorded one, with the newer
  /// of the two winning per node (see the class doc).
  ///
  /// @tparam Schema A generated Cap'n Proto struct type.
  template<typename Schema>
  void record()
  {
    mLoader.loadCompiledTypeAndDependencies<Schema>();
  }

  /// @brief Write the recorded closure to `schemas.bin` under @p directory.
  ///
  /// @param directory An existing directory to write into.
  ///
  /// @throws std::runtime_error If the file cannot be written.
  void write(const std::filesystem::path& directory) const;

  /// @brief Report whether a schema is recorded under @p schemaId.
  [[nodiscard]] bool contains(std::uint64_t schemaId) const;

  /// @brief The struct schema recorded under @p schemaId, as a live schema to read messages with.
  ///
  /// @throws std::out_of_range If no schema is recorded under @p schemaId.
  ///
  /// @throws std::invalid_argument If the recorded schema is not a struct schema.
  [[nodiscard]] capnp::StructSchema at(std::uint64_t schemaId) const;

private:
  /// Holds every recorded node, keyed by type id; the engine behind both directions. It collects
  /// compiled closures on the recording side and revives serialized nodes on the replay side,
  /// arbitrating duplicate ids by keeping the newer version.
  capnp::SchemaLoader mLoader;
};

} // namespace nioc::terminus

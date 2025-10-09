////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "defines.hpp"
#include "memoryCrate.hpp"
#include <filesystem>
#include <memory>
#include <span>
#include <vector>

namespace nioc::chronicle
{

/// @brief Writes data to a chronicle for later playback.
///
/// Records data to channels in the order it arrives. This preserves the exact
/// sequence of events for replay.
class Writer
{
public:
  static constexpr auto kDefaultMaxFileSizeInBytes = 128ULL * 1024ULL * 1024ULL;

  /// @brief Constructs a Writer.
  /// @param logRoot Root directory for the chronicle. A timestamped subdirectory will be created.
  /// @param maxFileSizeInBytes Maximum size of individual data files.
  explicit Writer(
      std::filesystem::path logRoot = std::filesystem::temp_directory_path() / "niocLogs",
      std::size_t maxFileSizeInBytes = kDefaultMaxFileSizeInBytes);

  Writer(const Writer&) = delete;

  Writer(Writer&& rhs) noexcept;

  ~Writer();

  Writer& operator=(const Writer&) = delete;

  Writer& operator=(Writer&&) noexcept = delete;

  /// @brief Writes a data frame to a channel.
  /// @param channelId Channel identifier.
  /// @param data Data to write.
  void write(ChannelId channelId, const std::span<const std::byte>& data);

  /// @brief Writes multiple data spans as a single frame to a channel.
  /// @param channelId Channel identifier.
  /// @param data Data spans to write as one frame.
  void write(ChannelId channelId, const std::vector<std::span<const std::byte>>& data);

  /// @brief Gets the chronicle directory path.
  /// @return Path to the chronicle directory.
  [[nodiscard]] const std::filesystem::path& path() const noexcept;

private:
  class LoggerImpl;
  std::unique_ptr<LoggerImpl> mLoggerImpl;
};

/// @brief A single entry from a chronicle.
///
/// Contains the channel ID and data for one frame.
struct Entry
{
  ChannelId mChannelId; ///< Channel identifier.

  MemoryCrate mMemoryCrate; ///< Frame data.
};

/// @brief Reads data from a chronicle for playback.
///
/// Reads entries in the same order they were written.
class Reader
{
public:
  /// @brief Constructs a Reader.
  /// @param logRoot Path to the chronicle directory.
  explicit Reader(std::filesystem::path logRoot);

  Reader(const Reader&) = delete;

  Reader(Reader&& reader) noexcept;

  ~Reader();

  Reader& operator=(const Reader&) = delete;

  Reader& operator=(Reader&& reader) noexcept;

  /// @brief Reads the next entry.
  /// @return Entry with channel ID and data.
  /// @throws std::runtime_error When end of chronicle is reached.
  Entry read();

private:
  class LogReaderImpl;
  std::unique_ptr<LogReaderImpl> mLogReaderImpl;
};

} // namespace nioc::chronicle

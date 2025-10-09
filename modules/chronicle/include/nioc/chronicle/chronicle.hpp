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

// Writer class (formerly Logger)
class Writer
{
public:
  static constexpr auto kDefaultMaxFileSizeInBytes = 128ULL * 1024ULL * 1024ULL;

  /// @brief Constructor
  /// @param logRoot      Root directory to store the chronicle at. The chronicle is written to a
  /// child
  ///                     directory with the local date and time as the directory name.
  ///
  /// @param maxFileSizeInBytes     Size of files allocated to store the data.
  explicit Writer(
      std::filesystem::path logRoot = std::filesystem::temp_directory_path() / "niocLogs",
      std::size_t maxFileSizeInBytes = kDefaultMaxFileSizeInBytes);

  Writer(const Writer&) = delete;

  Writer(Writer&& rhs) noexcept;

  ~Writer();

  Writer& operator=(const Writer&) = delete;

  Writer& operator=(Writer&&) noexcept = delete;

  void write(ChannelId channelId, const std::span<const std::byte>& data);

  void write(ChannelId channelId, const std::vector<std::span<const std::byte>>& data);

  [[nodiscard]] const std::filesystem::path& path() const noexcept;

private:
  class LoggerImpl;
  std::unique_ptr<LoggerImpl> mLoggerImpl;
};

// Entry read from chronicle
struct Entry
{
  ChannelId mChannelId;

  MemoryCrate mMemoryCrate;
};

// Reader class (formerly LogReader)
class Reader
{
public:
  explicit Reader(std::filesystem::path logRoot);

  Reader(const Reader&) = delete;

  Reader(Reader&& reader) noexcept;

  ~Reader();

  Reader& operator=(const Reader&) = delete;

  Reader& operator=(Reader&& reader) noexcept;

  Entry read();

private:
  class LogReaderImpl;
  std::unique_ptr<LogReaderImpl> mLogReaderImpl;
};

} // namespace nioc::chronicle

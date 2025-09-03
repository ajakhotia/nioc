////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "defines.hpp"
#include <filesystem>
#include <span>
#include <vector>

namespace nioc::logger
{
class Logger
{
public:
  static constexpr auto kDefaultMaxFileSizeInBytes = 128ULL * 1024ULL * 1024ULL;

  /// @brief Constructor
  /// @param logRoot      Root directory to store the log at. The log is written to a child
  ///                     directory with the local date and time as the directory name.
  ///
  /// @param maxFileSizeInBytes     Size of files allocated to store the data.
  explicit Logger(
      std::filesystem::path logRoot = std::filesystem::temp_directory_path() / "niocLogs",
      std::size_t maxFileSizeInBytes = kDefaultMaxFileSizeInBytes);

  Logger(const Logger&) = delete;

  Logger(Logger&& rhs) noexcept;

  ~Logger();

  Logger& operator=(const Logger&) = delete;

  Logger& operator=(Logger&&) noexcept = delete;

  void write(ChannelId channelId, const std::span<const std::byte>& data);

  void write(ChannelId channelId, const std::vector<std::span<const std::byte>>& data);

  [[nodiscard]] const std::filesystem::path& path() const noexcept;

private:
  class LoggerImpl;
  std::unique_ptr<LoggerImpl> mLoggerImpl;
};

} // namespace nioc::logger

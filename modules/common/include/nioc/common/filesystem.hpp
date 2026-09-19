////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <filesystem>

namespace nioc::common
{

/// @brief Returns @p path if it names an existing directory; throws otherwise.
///
/// Use as a guard at an API boundary to reject bad inputs early. It returns the path, so it reads
/// well inline:
///
///     auto dir = requireExistingDirectory(arg);
///
/// @param path The directory to check. Passed and returned by value.
///
/// @return @p path, unchanged.
///
/// @throws std::invalid_argument if @p path does not exist or is not a directory.
///
/// @throws std::filesystem::filesystem_error if a filesystem status query fails (for example, a
/// permission error while looking up the path).
///
/// @see requireEmptyDirectory
std::filesystem::path requireExistingDirectory(std::filesystem::path path);

/// @brief Returns @p path if it names an existing regular file; throws otherwise.
///
/// The file counterpart of requireExistingDirectory, for the same boundary-guard use.
///
/// @param path The file to check. Passed and returned by value.
///
/// @return @p path, unchanged.
///
/// @throws std::invalid_argument if @p path does not exist or is not a regular file.
///
/// @throws std::filesystem::filesystem_error if a filesystem status query fails (for example, a
/// permission error while looking up the path).
///
/// @see requireExistingDirectory
std::filesystem::path requireExistingFile(std::filesystem::path path);

/// @brief Returns @p path if it names an existing directory that is empty; throws otherwise.
///
/// Use to claim a directory as a fresh output location before writing into it.
///
/// @param path The directory to check. Passed and returned by value.
///
/// @return @p path, unchanged.
///
/// @throws std::invalid_argument if @p path does not exist, is not a directory, or is not empty.
///
/// @throws std::filesystem::filesystem_error if a filesystem status query fails (for example, a
/// permission error while looking up the path).
///
/// @see requireExistingDirectory, requireExistingFile
std::filesystem::path requireEmptyDirectory(std::filesystem::path path);

/// @brief A directory owned for a scope: created fresh and empty on construction (any prior
/// contents are removed), deleted with everything in it on destruction.
///
/// Example:
///
///     const auto scratch = ScratchDirectory{std::filesystem::temp_directory_path() / "stage"};
///     writeFile(scratch.path() / "input.bin");
///     // the directory and its contents are gone when scratch leaves scope
///
/// Non-copyable, non-movable. Removal on destruction ignores failure.
class ScratchDirectory
{
public:
  /// @brief Take ownership of @p path: remove whatever is there and create it empty, along with
  /// any missing parents.
  ///
  /// @throws std::filesystem::filesystem_error if the directory cannot be created.
  explicit ScratchDirectory(std::filesystem::path path);

  ScratchDirectory(const ScratchDirectory&) = delete;

  ScratchDirectory(ScratchDirectory&&) noexcept = delete;

  /// @brief Remove the directory and everything beneath it.
  ~ScratchDirectory();

  ScratchDirectory& operator=(const ScratchDirectory&) = delete;

  ScratchDirectory& operator=(ScratchDirectory&&) noexcept = delete;

  /// @brief The owned directory.
  [[nodiscard]] const std::filesystem::path& path() const noexcept;

private:
  std::filesystem::path mPath;
};

} // namespace nioc::common

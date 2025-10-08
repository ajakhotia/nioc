////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "utils.hpp"
#include <bit>
#include <boost/endian.hpp>
#include <format>
#include <fstream>
#include <numeric>

namespace nioc::logger
{

std::string iso8601UtcFormat(std::chrono::system_clock::time_point timePoint)
{
  return std::format("{:%FT%TZ}", timePoint);
}

std::string padString(const std::string& input, const uint64_t paddedLength, const char paddingChar)
{
  return std::string(paddedLength - std::min(paddedLength, input.size()), paddingChar) + input;
}

std::string buildRollName(const std::uint64_t rollId)
{
  return kRollFileNamePrefix + padString(std::to_string(rollId), kPaddedRollNumberLength) +
         kRollFileNameExtension;
}

bool fileHasSpace(
    std::ofstream& file, const std::uint64_t spaceRequired, const std::uint64_t maxFileSizeInBytes)
{
  if(spaceRequired > maxFileSizeInBytes)
  {
    throw std::invalid_argument("[logger::Channel] Space requested on a file is greater than "
                                "the maximum allowed size of the file. This is an impossible "
                                "constraint to satisfy.");
  }

  return (maxFileSizeInBytes - file.tellp()) >= spaceRequired;
}

std::uint64_t computeTotalSizeInBytes(const std::vector<std::span<const std::byte>>& dataCollection)
{
  return std::accumulate(
      dataCollection.begin(),
      dataCollection.end(),
      0ULL,
      [](const std::uint64_t accumulatedSize, const std::span<const std::byte>& data)
      {
        return accumulatedSize + data.size_bytes();
      });
}

template<>
void ReadWriteUtil<SequenceEntry>::write(std::ostream& stream, SequenceEntry value)
{
  boost::endian::native_to_little_inplace(value.mChannelId);
  stream.write(std::bit_cast<const char*>(&value), sizeof(value));
}

template<>
SequenceEntry ReadWriteUtil<SequenceEntry>::read(const char* ptr, std::uint64_t /*unused*/)
{
  auto value = *std::bit_cast<const SequenceEntry*>(ptr);
  boost::endian::little_to_native_inplace(value.mChannelId);
  return value;
}

template<>
void ReadWriteUtil<IndexEntry>::write(std::ostream& stream, IndexEntry value)
{
  boost::endian::native_to_little_inplace(value.mRollId);
  boost::endian::native_to_little_inplace(value.mRollPosition);
  boost::endian::native_to_little_inplace(value.mDataSize);

  stream.write(std::bit_cast<const char*>(&value), sizeof(value));
}

template<>
IndexEntry ReadWriteUtil<IndexEntry>::read(const char* ptr, const std::uint64_t /*unused*/)
{
  auto value = *std::bit_cast<const IndexEntry*>(ptr);

  boost::endian::little_to_native_inplace(value.mRollId);
  boost::endian::little_to_native_inplace(value.mRollPosition);
  boost::endian::little_to_native_inplace(value.mDataSize);
  return value;
}

template<>
void ReadWriteUtil<std::span<const std::byte>>::write(
    std::ostream& stream, std::span<const std::byte> value)
{
  stream.write(std::bit_cast<const char*>(value.data()), std::ssize(value));
}

template<>
std::span<const std::byte>
ReadWriteUtil<std::span<const std::byte>>::read(const char* ptr, const std::uint64_t size)
{
  return std::as_bytes(std::span(ptr, size));
}

std::filesystem::path validatePath(std::filesystem::path path)
{
  if(not std::filesystem::exists(path))
  {
    throw std::invalid_argument("[Logger::utils] Directory " + path.string() + " does not exist.");
  }

  return path;
}


} // namespace nioc::logger

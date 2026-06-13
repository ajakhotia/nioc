////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <numeric>
#include <utils.hpp>

namespace nioc::chronicle
{
namespace
{

std::vector<char> generateData()
{
  static constexpr auto kSize = 20UL;
  static constexpr auto kStartValue = 63;
  auto data = std::vector<char>(kSize);
  std::ranges::iota(data, kStartValue);
  return data;
}

} // namespace

TEST(ChronicleUtils, padString)
{
  const auto input = std::string{"682"};

  {
    const auto output = padString(input, 13U, '0');
    EXPECT_EQ("0000000000682", output);
  }
  {
    const auto output = padString(input, 2U, '0');
    EXPECT_EQ("682", output);
  }
  {
    const auto output = padString(input, 13U, 'Z');
    EXPECT_EQ("ZZZZZZZZZZ682", output);
  }
}

TEST(ChronicleUtils, buildRollName)
{
  EXPECT_EQ("roll00000000000000000000.nioc", buildRollName(0U));
  EXPECT_EQ("roll00000000000000000001.nioc", buildRollName(1U));
  EXPECT_EQ("roll00000003519894239162.nioc", buildRollName(3519894239162U));
}

TEST(ChronicleUtils, toHexString)
{
  constexpr auto integer = 255U;
  EXPECT_EQ("0xff", hexString(integer));
}

TEST(ChronicleUtils, hexStringToInteger)
{
  constexpr auto hexString = "0xff";
  EXPECT_EQ(255, integerFromHex<uint64_t>(hexString));
}

TEST(ChronicleUtils, computeTotalSizeInBytes)
{
  const auto data = generateData();
  const auto value = std::as_bytes(std::span(data));
  constexpr auto numRepetitions = 32U;

  const auto spanCollection = std::vector<std::span<const std::byte>>(numRepetitions, value);

  EXPECT_EQ(numRepetitions * data.size(), computeTotalSizeInBytes(spanCollection));
}

TEST(ChronicleUtils, ReadWriteUtilSequenceEntry)
{
  auto stream = std::stringstream{};
  const auto value = SequenceEntry{ChannelId{53519839189237}};

  ReadWriteUtil<SequenceEntry>::write(stream, value);
  const auto readValue = ReadWriteUtil<SequenceEntry>::read(stream.str().data());

  EXPECT_EQ(value.mChannelId, readValue.mChannelId);
}

TEST(ChronicleUtils, ReadWriteUtilIndexEntry)
{
  auto stream = std::stringstream{};
  const auto value =
      IndexEntry{.mRollId = 53519839189237, .mOffset = 9065316618953, .mSize = 281591230};

  ReadWriteUtil<IndexEntry>::write(stream, value);
  const auto readValue = ReadWriteUtil<IndexEntry>::read(stream.str().data());

  EXPECT_EQ(value.mRollId, readValue.mRollId);
  EXPECT_EQ(value.mOffset, readValue.mOffset);
  EXPECT_EQ(value.mSize, readValue.mSize);
}

TEST(ChronicleUtils, onDiskEntryLayoutIsPackedLittleEndian)
{
  // Pins the on-disk format: a recording written today must stay replayable, so the entry layout
  // is part of the contract — 8-byte sequence entries and 24-byte index entries, little-endian,
  // no padding.
  {
    constexpr auto kChannelValue = 0x0102030405060708ULL;
    auto stream = std::stringstream{};
    ReadWriteUtil<SequenceEntry>::write(stream, SequenceEntry{ChannelId{kChannelValue}});

    const auto bytes = stream.str();
    ASSERT_EQ(8U, bytes.size());
    EXPECT_EQ(std::string("\x08\x07\x06\x05\x04\x03\x02\x01", 8), bytes);
  }

  {
    constexpr auto kRollId = 0x11ULL;
    constexpr auto kOffset = 0x2233ULL;
    constexpr auto kSize = 0x44556677ULL;
    auto stream = std::stringstream{};
    ReadWriteUtil<IndexEntry>::write(
        stream,
        IndexEntry{.mRollId = kRollId, .mOffset = kOffset, .mSize = kSize});

    const auto bytes = stream.str();
    ASSERT_EQ(24U, bytes.size());
    const auto expected = std::string{
        "\x11\x00\x00\x00\x00\x00\x00\x00"
        "\x33\x22\x00\x00\x00\x00\x00\x00"
        "\x77\x66\x55\x44\x00\x00\x00\x00",
        24};
    EXPECT_EQ(expected, bytes);
  }
}

TEST(ChronicleUtils, ReadWriteByteSpan)
{
  auto stream = std::stringstream{};
  const auto data = generateData();
  const auto value = std::as_bytes(std::span(data));
  const auto size = value.size();

  ReadWriteUtil<std::span<const std::byte>>::write(stream, value);
  const auto str = stream.str();
  const auto readValue = ReadWriteUtil<std::span<const std::byte>>::read(str.data(), size);

  EXPECT_TRUE(std::ranges::equal(value, readValue));
}


} // namespace nioc::chronicle

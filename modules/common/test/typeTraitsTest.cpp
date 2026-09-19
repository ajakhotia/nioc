////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <gtest/gtest.h>
#include <nioc/common/typeTraits.hpp>
#include <numeric>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

namespace nioc::common
{
TEST(TypeTraits, IsSpecialization)
{
  static_assert(IsSpecialization<std::vector<int>, std::vector>::value);
  static_assert(not IsSpecialization<std::vector<int>, std::deque>::value);
  static_assert(isSpecialization<std::vector<int>, std::vector>);
  static_assert(not isSpecialization<std::vector<int>, std::deque>);

  EXPECT_TRUE(bool(IsSpecialization<std::vector<int>, std::vector>::value));
  EXPECT_FALSE(bool(IsSpecialization<std::vector<int>, std::deque>::value));
  EXPECT_TRUE(bool(isSpecialization<std::vector<int>, std::vector>));
  EXPECT_FALSE(bool(isSpecialization<std::vector<int>, std::deque>));
}


class TestType;
class AnotherTestType;

TEST(TypeTraits, prettyName)
{
  static_assert("nioc::common::TestType" == prettyName<TestType>());
  static_assert("nioc::common::TestType" != prettyName<AnotherTestType>());
  EXPECT_EQ("nioc::common::TestType", prettyName<TestType>());
  EXPECT_NE("nioc::common::TestType", prettyName<AnotherTestType>());
}

namespace
{

struct Header
{
  std::uint32_t mMagic{0U};
  std::uint16_t mVersion{0U};
  std::uint16_t mFlags{0U};
};

constexpr auto kMagic = std::uint32_t{0xCAFEBABE};

} // namespace

TEST(TypeTraits, isImplicitLifetime)
{
  static_assert(isImplicitLifetime<int>);
  static_assert(isImplicitLifetime<std::byte>);
  static_assert(isImplicitLifetime<double*>);
  static_assert(isImplicitLifetime<Header>); // an aggregate, despite its member initializers
  static_assert(isImplicitLifetime<std::array<int, 4>>);
  static_assert(not isImplicitLifetime<std::string>);
  static_assert(not isImplicitLifetime<std::vector<int>>);
}

TEST(TypeTraits, startLifetimeAsViewsBytesAsObjects)
{
  constexpr auto kCount = std::size_t{3};
  alignas(Header) auto storage = std::array<std::byte, kCount * sizeof(Header)>{};

  auto* const headers = startLifetimeAs<Header>(storage.data());
  static_assert(std::is_same_v<decltype(headers), Header* const>);
  const auto span = std::span{headers, kCount};
  span.front() = Header{.mMagic = kMagic, .mVersion = 2, .mFlags = 1};
  span.back() = Header{.mMagic = 7, .mVersion = 0, .mFlags = 0};

  // The bytes underneath are the objects' representation.
  auto magic = std::uint32_t{};
  std::memcpy(&magic, storage.data(), sizeof(magic));
  EXPECT_EQ(magic, kMagic);

  const auto& constStorage = storage;
  const auto* const constHeaders = startLifetimeAs<Header>(constStorage.data());
  static_assert(std::is_same_v<decltype(constHeaders), const Header* const>);
  const auto constSpan = std::span{constHeaders, kCount};
  EXPECT_EQ(constSpan.back().mMagic, 7U);
}

TEST(TypeTraits, startLifetimeAsArraySizesBySpan)
{
  constexpr auto kCount = std::size_t{5};
  alignas(std::uint32_t) auto storage =
      std::array<std::byte, (kCount * sizeof(std::uint32_t)) + 3>{}; // 3 trailing bytes

  auto words = startLifetimeAsArray<std::uint32_t>(std::span{storage});
  static_assert(std::is_same_v<decltype(words), std::span<std::uint32_t>>);
  ASSERT_EQ(words.size(), kCount);
  std::ranges::iota(words, 1U);

  const auto constWords = startLifetimeAsArray<std::uint32_t>(std::span<const std::byte>{storage});
  static_assert(std::is_same_v<decltype(constWords), const std::span<const std::uint32_t>>);
  EXPECT_EQ(constWords.size(), kCount);
  EXPECT_EQ(constWords.back(), kCount);

  EXPECT_TRUE(startLifetimeAsArray<std::uint64_t>(std::span{storage}.subspan(0, 7)).empty());
}

} // namespace nioc::common

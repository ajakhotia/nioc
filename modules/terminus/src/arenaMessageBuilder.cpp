////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <array>
#include <bit>
#include <capnp/message.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <kj/array.h>
#include <kj/common.h>
#include <nioc/common/exception.hpp>
#include <nioc/terminus/arenaMessageBuilder.hpp>
#include <span>
#include <stdexcept>
#include <utility>

namespace nioc::terminus
{

ArenaMessageBuilder::ArenaMessageBuilder(const std::span<std::byte> arena): mArena{arena} {}

bool ArenaMessageBuilder::overflowed() const
{
  return not mArenaOverflow.empty();
}

std::span<std::byte> ArenaMessageBuilder::frame()
{
  const auto segments = getSegmentsForOutput();
  if(segments.size() != 1 or segments.front().begin() != firstSegment().begin())
  {
    common::throwException<std::logic_error>(
        "Cannot frame a {}-segment build; a single in-arena segment is required.",
        segments.size());
  }

  const auto segmentWords = segments.front().size();
  writeHeader(mArena.data(), segmentWords);

  return mArena.first(frameSize(segmentWords));
}

std::size_t ArenaMessageBuilder::frameSize(const std::size_t segmentWords) noexcept
{
  return kHeaderBytes + (segmentWords * sizeof(capnp::word));
}

std::span<std::byte> ArenaMessageBuilder::writeFrame(
    const std::span<std::byte> destination,
    const kj::ArrayPtr<const capnp::word> segment)
{
  writeHeader(destination.data(), segment.size());

  const auto segmentBytes = asByteSpan(segment);
  std::memcpy(destination.subspan(kHeaderBytes).data(), segmentBytes.data(), segmentBytes.size());

  return destination.first(frameSize(segment.size()));
}

kj::ArrayPtr<capnp::word> ArenaMessageBuilder::allocateSegment(const unsigned int minimumSize)
{
  const auto arena = firstSegment();
  if(not mArenaTaken and arena.size() >= minimumSize)
  {
    mArenaTaken = true;
    return arena;
  }

  // Overflow (or an arena too small for the root): a heap segment, zeroed to meet Cap'n Proto's
  // zero-filled-segment contract.
  auto segment = kj::heapArray<capnp::word>(minimumSize);
  std::memset(segment.begin(), 0, segment.size() * sizeof(capnp::word));
  const auto view = segment.asPtr();
  mArenaOverflow.push_back(std::move(segment));
  return view;
}

kj::ArrayPtr<capnp::word> ArenaMessageBuilder::firstSegment() const
{
  if(mArena.size() <= kHeaderBytes)
  {
    return {};
  }
  return asWords(mArena.subspan(kHeaderBytes));
}

void ArenaMessageBuilder::writeHeader(std::byte* const destination, const std::size_t segmentWords)
{
  static_assert(
      std::endian::native == std::endian::little,
      "flat-array framing assumes little-endian");

  const auto header = std::array{0U, static_cast<std::uint32_t>(segmentWords)};
  std::memcpy(destination, header.data(), kHeaderBytes);
}

kj::ArrayPtr<const capnp::word> asWords(const std::span<const std::byte> bytes)
{
  // The words already exist in this read-only storage; reinterpret to retype the pointer.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto* const words = reinterpret_cast<const capnp::word*>(bytes.data());
  return kj::arrayPtr(std::launder(words), bytes.size() / sizeof(capnp::word));
}

kj::ArrayPtr<capnp::word> asWords(const std::span<std::byte> bytes)
{
  const auto wordCount = bytes.size() / sizeof(capnp::word);
  // memmove begins the word objects' lifetimes; with src == dest it is elided, so no bytes move.
  return kj::arrayPtr(
      std::launder(
          static_cast<capnp::word*>(
              std::memmove(bytes.data(), bytes.data(), wordCount * sizeof(capnp::word)))),
      wordCount);
}

std::span<const std::byte> asByteSpan(kj::ArrayPtr<const capnp::word> words)
{
  return std::as_bytes(std::span{words.begin(), words.size()});
}

std::span<std::byte> asByteSpan(kj::ArrayPtr<capnp::word> words)
{
  return std::as_writable_bytes(std::span{words.begin(), words.size()});
}

} // namespace nioc::terminus

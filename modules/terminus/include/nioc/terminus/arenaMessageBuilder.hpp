////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <bit>
#include <capnp/message.h>
#include <cstddef>
#include <kj/array.h>
#include <kj/common.h>
#include <span>
#include <type_traits>
#include <vector>

namespace nioc::terminus
{

/// @brief A Cap'n Proto builder that frames its message into a caller-provided arena.
///
/// The arena's leading word is held back for the single-segment flat-array header; the message
/// builds into the words that follow, spilling onto the heap only if it outgrows them. So a message
/// that fits leaves the arena holding a complete, ready-to-record frame: `[header][segment]`.
/// @ref single reports whether that happened, and @ref frame stamps the header and hands back the
/// frame.
///
/// The arena must be zero-filled, per Cap'n Proto's zero-filled-segment contract.
class ArenaMessageBuilder final: public capnp::MessageBuilder
{
public:
  /// @brief Frames into @p arena, holding back its leading word for the header.
  /// @param arena Zero-filled bytes to build the frame into.
  explicit ArenaMessageBuilder(std::span<std::byte> arena);

  ArenaMessageBuilder(const ArenaMessageBuilder&) = delete;

  ArenaMessageBuilder(ArenaMessageBuilder&&) = delete;

  // NOLINTNEXTLINE(cppcoreguidelines-noexcept-destructor,performance-noexcept-destructor)
  ~ArenaMessageBuilder() final = default;

  ArenaMessageBuilder& operator=(const ArenaMessageBuilder&) = delete;

  ArenaMessageBuilder& operator=(ArenaMessageBuilder&&) = delete;

  /// @brief Whether the message spilled onto the heap instead of staying within the arena.
  ///
  /// False is the zero-copy case: the whole message is the single arena segment, so the frame is
  /// already in place and @ref frame just stamps its header. True means it must be re-rooted into a
  /// fresh arena before it can be framed.
  [[nodiscard]] bool overflowed() const;

  /// @brief Stamps the flat-array header into the held-back word and returns the whole frame.
  ///
  /// The returned span is `[header][segment]`, ready to record. Precondition: not @ref overflowed —
  /// the build is one segment in the arena.
  [[nodiscard]] std::span<std::byte> frame();

  /// @brief Byte length of a frame whose single segment is @p segmentWords words: header + segment.
  ///
  /// Size a destination to this before @ref writeFrame.
  [[nodiscard]] static std::size_t frameSize(std::size_t segmentWords) noexcept;

  /// @brief Writes a `[header][segment]` frame for @p segment into @p destination and returns it.
  ///
  /// Frames a single segment built elsewhere — e.g. a message re-rooted into one segment — into a
  /// caller's buffer, the same format @ref frame produces in place. @p destination must hold
  /// @ref frameSize bytes for @p segment.
  static std::span<std::byte> writeFrame(
      std::span<std::byte> destination,
      kj::ArrayPtr<const capnp::word> segment);

  kj::ArrayPtr<capnp::word> allocateSegment(unsigned int minimumSize) final;

private:
  /// Bytes held back at the front of the arena for the single-segment flat-array header, which is
  /// `[segmentCount - 1 == 0][segmentWords]` as two little-endian 32-bit words.
  static constexpr std::size_t kHeaderBytes = 8;

  /// @brief The first segment the message builds into: the arena past its header. Empty when the
  /// arena cannot hold even the header, which sends the build to the heap.
  [[nodiscard]] kj::ArrayPtr<capnp::word> firstSegment() const;

  /// @brief Stamps the flat-array header for a @p segmentWords-word segment at @p destination.
  static void writeHeader(std::byte* destination, std::size_t segmentWords);

  std::span<std::byte> mArena;

  /// Whether @ref firstSegment has been handed to Cap'n Proto. It is offered only for the first
  /// segment request; without this flag a build that fills the arena and asks for another segment
  /// would be handed the arena again and overwrite itself (the single-segment case never touches
  /// @ref mArenaOverflow, so its emptiness cannot stand in for this).
  bool mArenaTaken{false};

  std::vector<kj::Array<capnp::word>> mArenaOverflow;
};

/// @brief Views @p bytes as Cap'n Proto words, preserving constness (inverse of @ref asByteSpan).
///
/// kj omits this direction (its `ArrayPtr::asBytes` only goes word→byte) because the bytes must be
/// word-aligned; a chronicle frame is, since every claim is word-sized over a page-aligned roll. A
/// writable byte span yields writable words; a read-only one yields read-only words.
///
/// @param bytes Word-aligned bytes.
template<typename Byte>
  requires std::is_same_v<std::remove_const_t<Byte>, std::byte>
[[nodiscard]] auto asWords(std::span<Byte> bytes)
{
  using Word = std::conditional_t<std::is_const_v<Byte>, const capnp::word, capnp::word>;
  return kj::arrayPtr(std::bit_cast<Word*>(bytes.data()), bytes.size() / sizeof(capnp::word));
}

/// @brief Views Cap'n Proto @p words as a byte span, preserving constness (inverse of @ref
/// asWords).
///
/// @param words Cap'n Proto words.
template<typename Word>
  requires std::is_same_v<std::remove_const_t<Word>, capnp::word>
[[nodiscard]] auto asByteSpan(kj::ArrayPtr<Word> words)
{
  using Byte = std::conditional_t<std::is_const_v<Word>, const std::byte, std::byte>;
  return std::span<Byte>{std::bit_cast<Byte*>(words.begin()), words.size() * sizeof(capnp::word)};
}

} // namespace nioc::terminus

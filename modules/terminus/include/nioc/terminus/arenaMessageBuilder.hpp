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

/// @brief A `capnp::MessageBuilder` that builds a message in place inside a caller-owned byte
/// buffer and frames it for the wire with zero extra copies.
///
/// Build your message as usual through the `capnp::MessageBuilder` interface (e.g. `initRoot`),
/// then call `frame` to get a contiguous, ready-to-send frame: an 8-byte flat-array header followed
/// by the single message segment, all living in the buffer you supplied. This is the fast path for
/// the common case where one message fits in one preallocated buffer.
///
/// Example:
///
///     std::array<std::byte, 4096> buffer{};
///     ArenaMessageBuilder builder{buffer};
///     auto root = builder.initRoot<MyMessage>();
///     // ... fill root ...
///     if(not builder.overflowed())
///     {
///       std::span<std::byte> wireFrame = builder.frame();  // aliases buffer
///     }
///
/// The buffer (`arena`) must outlive the builder and every view it returns; the builder does not
/// own it. The first 8 bytes are reserved for the header and the message segment fills the rest. If
/// the message needs more room than the buffer holds, the build spills into heap segments and can
/// no longer be framed; always check `overflowed` before calling `frame`.
///
/// Not copyable, not movable, not thread-safe.
///
/// @see asWords, asByteSpan
class ArenaMessageBuilder final: public capnp::MessageBuilder
{
public:
  /// @brief Construct a builder that builds into `arena`, with `arena`'s first 8 bytes reserved for
  /// the frame header and the rest available for the message segment.
  ///
  /// @param arena Backing buffer. Must outlive the builder and any view returned by `frame`.
  explicit ArenaMessageBuilder(std::span<std::byte> arena);

  ArenaMessageBuilder(const ArenaMessageBuilder&) = delete;

  ArenaMessageBuilder(ArenaMessageBuilder&&) = delete;

  // NOLINTNEXTLINE(cppcoreguidelines-noexcept-destructor,performance-noexcept-destructor)
  ~ArenaMessageBuilder() final = default;

  ArenaMessageBuilder& operator=(const ArenaMessageBuilder&) = delete;

  ArenaMessageBuilder& operator=(ArenaMessageBuilder&&) = delete;

  /// @brief Report whether any part of the message landed in heap segments instead of the arena.
  ///
  /// Returns true once the message outgrew the arena. An overflowed build cannot be framed; check
  /// this before calling `frame`.
  [[nodiscard]] bool overflowed() const;

  /// @brief Write the frame header into the arena and return the contiguous frame: the 8-byte
  /// header followed by the message segment.
  ///
  /// The returned span aliases the arena and stays valid only while the arena lives.
  ///
  /// @return A prefix of the arena holding the complete wire frame.
  ///
  /// @throws std::logic_error If the build is not a single segment placed in the arena, i.e. if it
  /// overflowed or no message was written.
  [[nodiscard]] std::span<std::byte> frame();

  /// @brief Return the total framed byte length for a segment of `segmentWords` words (header plus
  /// segment).
  [[nodiscard]] static std::size_t frameSize(std::size_t segmentWords) noexcept;

  /// @brief Copy `segment` into `destination` behind a fresh frame header and return the frame
  /// view.
  ///
  /// Standalone framing helper for a segment that was built elsewhere; does not need an
  /// `ArenaMessageBuilder` instance.
  ///
  /// @param destination Output buffer. Must hold at least `frameSize(segment.size())` bytes and
  /// must not overlap `segment`.
  ///
  /// @param segment The single message segment to frame.
  ///
  /// @return A prefix of `destination` holding the complete wire frame.
  static std::span<std::byte> writeFrame(
      std::span<std::byte> destination,
      kj::ArrayPtr<const capnp::word> segment);

  /// @brief Supply Cap'n Proto a segment of at least `minimumSize` words; called by the framework,
  /// not by users.
  ///
  /// Serves the root segment from the arena once. Every later request, or a root that does not fit
  /// the arena, is served from a zero-filled heap segment and sets `overflowed`.
  kj::ArrayPtr<capnp::word> allocateSegment(unsigned int minimumSize) final;

private:
  /// The number of bytes the frame header occupies at the front of the arena.
  static constexpr std::size_t kHeaderBytes = 8;

  /// @brief Return the word view of the arena region reserved for the message segment, i.e. the
  /// arena past the header, rounded down to a whole number of words.
  ///
  /// This is the region handed out on the first `allocateSegment` call.
  [[nodiscard]] kj::ArrayPtr<capnp::word> firstSegment() const;

  /// @brief Write the 8-byte frame header for a single segment of `segmentWords` words at
  /// `destination`.
  ///
  /// @param destination Start of the header. Must have room for at least `kHeaderBytes` bytes.
  ///
  /// @param segmentWords Length of the framed segment in words.
  static void writeHeader(std::byte* destination, std::size_t segmentWords);

  /// The caller-owned backing buffer. The first `kHeaderBytes` bytes hold the header and the rest
  /// backs the message segment. Not owned by the builder.
  std::span<std::byte> mArena;

  /// Whether the arena has already been served as the root segment, so further allocations must
  /// spill to the heap.
  bool mArenaTaken{false};

  /// Heap segments allocated once the message outgrew the arena. A non-empty vector means the build
  /// overflowed and cannot be framed.
  std::vector<kj::Array<capnp::word>> mArenaOverflow;
};

/// @brief View a byte span as Cap'n Proto words without copying, preserving constness.
///
/// The result is `const`-qualified when `Byte` is `const std::byte`.
///
/// @tparam Byte Either `std::byte` or `const std::byte`.
///
/// @param bytes Source bytes. `bytes.data()` must be word-aligned. Trailing bytes that do not fill
/// a whole word are dropped. The result aliases `bytes` and shares its lifetime.
template<typename Byte>
  requires std::is_same_v<std::remove_const_t<Byte>, std::byte>
[[nodiscard]] auto asWords(std::span<Byte> bytes)
{
  using Word = std::conditional_t<std::is_const_v<Byte>, const capnp::word, capnp::word>;
  return kj::arrayPtr(std::bit_cast<Word*>(bytes.data()), bytes.size() / sizeof(capnp::word));
}

/// @brief View a Cap'n Proto word array as a byte span without copying, preserving constness.
///
/// The result is `const`-qualified when `Word` is `const capnp::word`.
///
/// @tparam Word Either `capnp::word` or `const capnp::word`.
///
/// @param words Source words. The result aliases `words` and shares its lifetime.
template<typename Word>
  requires std::is_same_v<std::remove_const_t<Word>, capnp::word>
[[nodiscard]] auto asByteSpan(kj::ArrayPtr<Word> words)
{
  using Byte = std::conditional_t<std::is_const_v<Word>, const std::byte, std::byte>;
  return std::span<Byte>{std::bit_cast<Byte*>(words.begin()), words.size() * sizeof(capnp::word)};
}

} // namespace nioc::terminus

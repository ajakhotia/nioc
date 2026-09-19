////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstddef>
#include <cstdint>
#include <nioc/containers/file.hpp>
#include <nioc/containers/mapping.hpp>
#include <optional>
#include <set>
#include <span>
#include <string_view>

namespace nioc::chronicle
{

/// @brief A half-open interval of byte offsets within a file, [mBegin, mEnd): the coordinates
/// file advice takes. Offsets locate bytes in the file itself, independent of any mapping of it
/// into memory; a mapping's span is obtained from an offset with the mapping's base.
struct FileRange
{
  /// Offset of the first byte in the range, from the start of the file.
  std::uint64_t mBegin{0ULL};

  /// Offset one past the last byte in the range.
  std::uint64_t mEnd{0ULL};

  /// @brief Grow the range to also cover @p size bytes at @p offset.
  void cover(std::uint64_t offset, std::uint64_t size) noexcept;

  [[nodiscard]] std::uint64_t length() const noexcept;
};

/// @brief File advice (posix_fadvise WILLNEED and DONTNEED) issued asynchronously through a
/// dedicated io_uring where the kernel offers one, and synchronously otherwise, so callers never
/// see the difference and never wait on the kernel's readahead or page-release work when the ring
/// is available.
///
/// Queue requests with advise() and hand a batch to the kernel with submit(). Completions are
/// fire-and-forget and reaped opportunistically; a failing advice op is logged once per error kind
/// and never affects the caller. Every request names a file by descriptor: the kernel takes its
/// own reference during submit(), so the caller may close the descriptor as soon as submit()
/// returns, even while requests are still in flight.
///
/// Construction probes the kernel. Where io_uring or its fadvise opcode is unavailable (absent,
/// seccomp-filtered, disabled by sysctl), and after any submission failure or stall, the ring
/// applies every request synchronously instead. Non-copyable, non-movable. Not thread-safe.
class AdviceRing
{
public:
  /// @brief Probe the kernel and build a ring of @p entryCount submission slots, or fall back to
  /// synchronous advice when no ring can be had.
  ///
  /// @param entryCount Number of submission-queue slots; rounded up by the kernel to a power of
  /// two.
  explicit AdviceRing(std::uint32_t entryCount);

  AdviceRing(const AdviceRing&) = delete;

  AdviceRing(AdviceRing&&) noexcept = delete;

  /// @brief Release the ring's kernel resources. In-flight operations complete on their own; the
  /// kernel holds its own file references, so none dangle.
  ~AdviceRing();

  AdviceRing& operator=(const AdviceRing&) = delete;

  AdviceRing& operator=(AdviceRing&&) noexcept = delete;

  /// @brief Apply fadvise over a byte range of a file: queued for the next submit() when the ring
  /// is asynchronous, applied immediately otherwise. A full queue submits first to make room.
  ///
  /// The range is issued in chunks sized to the verb: POSIX_FADV_WILLNEED at the kernel's
  /// per-request readahead limit, since a longer request silently drops its tail; everything else
  /// in coarse chunks below the 32-bit length older kernels carry. For POSIX_FADV_DONTNEED, pages
  /// still referenced by a mapping's page tables survive; evict those first with
  /// Mapping::evict on the mapping.
  ///
  /// @param fileDescriptor The file the advice applies to. Must stay open until the next submit().
  ///
  /// @param range The byte range of the file the advice applies to.
  ///
  /// @param advice The POSIX_FADV_* value to apply.
  void advise(int fileDescriptor, FileRange range, int advice);

  /// @brief Hand every queued request to the kernel and reap available completions.
  ///
  /// Returns immediately; the advice work itself proceeds asynchronously. No-op when the ring is
  /// synchronous. Called automatically when the queue fills, so explicit calls only bound
  /// submission latency.
  void submit();

  /// @brief Whether requests currently go through a live io_uring rather than synchronous calls.
  [[nodiscard]] bool asynchronous() const noexcept;

private:
  /// @brief A live io_uring: its kernel resources, geometry, and the shared cursors resolved from
  /// the kernel's setup offsets. Present only while the ring is asynchronous.
  struct Ring
  {
    /// The io_uring instance; owned, closed on destruction.
    containers::File mInstance;

    /// The single mapping holding both ring headers (submission and completion); the cursors
    /// below point into it.
    containers::Mapping mHeaders;

    /// The mapping holding the submission-entry array that advise() writes into.
    containers::Mapping mSubmissionEntries;

    /// Ring geometry and shared-memory cursors. The pointed-to values live in mHeaders and are
    /// shared with the kernel: tail stores use release ordering and head loads acquire ordering,
    /// per the io_uring contract.
    std::uint32_t* mSubmissionHead;
    std::uint32_t* mSubmissionTail;
    std::uint32_t mSubmissionMask;
    std::uint32_t* mCompletionHead;
    std::uint32_t* mCompletionTail;
    std::uint32_t mCompletionMask;

    /// The completion entries' bytes within mHeaders, indexed by (position & mCompletionMask).
    std::span<std::byte> mCompletionEntries;

    /// Requests queued locally since the last submit(): written to entry slots but not yet
    /// handed to the kernel.
    std::uint32_t mPendingCount{0U};


    /// Requests handed to the kernel whose completions have not been reaped yet; bounded below
    /// the completion queue's capacity by submit().
    std::uint32_t mInFlightCount{0U};
  };

  /// The live ring, or std::nullopt when advice is applied synchronously.
  std::optional<Ring> mRing;

  /// The negated errno values already reported by reapCompletions, so each failure kind is
  /// logged once per ring.
  std::set<int> mReportedErrors;

  /// @brief Set up an io_uring with @p entryCount slots and resolve its geometry.
  ///
  /// @return The ring, or std::nullopt when the kernel lacks io_uring, its fadvise opcode, or
  /// the single-mapping feature this class relies on.
  [[nodiscard]] static std::optional<Ring> setUp(std::uint32_t entryCount);

  /// @brief Wait until every pending request can complete without overrunning the completion
  /// queue, reaping as completions arrive.
  ///
  /// @return True once there is room; false when the ring failed or stalled and was torn down
  /// (@p ring is then dangling; callers return at once).
  [[nodiscard]] bool awaitCompletionCapacity(Ring& ring);

  /// @brief Pop every available completion of @p ring, logging the first failure of each errno
  /// kind.
  void reapCompletions(Ring& ring);

  /// @brief Issue one request of at most the ring's per-request length over part of a range.
  void adviseChunk(int fileDescriptor, std::uint64_t offset, std::uint32_t length, int advice);

  /// @brief Tear the ring down, dropping requests queued but not yet submitted, and log
  /// @p reason once; every later request is applied synchronously.
  void fail(std::string_view reason);
};

} // namespace nioc::chronicle

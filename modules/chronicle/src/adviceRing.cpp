////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "adviceRing.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <fcntl.h>
#include <linux/io_uring.h>
#include <nioc/common/typeTraits.hpp>
#include <nioc/logger/logger.hpp>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <sys/syscall.h>
#include <system_error>
#include <unistd.h>
#include <utility>
#include <vector>

namespace nioc::chronicle
{
namespace
{

// NOLINTBEGIN(cppcoreguidelines-pro-type-vararg,hicpp-vararg): syscall is the kernel's API.

int ioUringSetup(const std::uint32_t entryCount, io_uring_params& params) noexcept
{
  return static_cast<int>(::syscall(__NR_io_uring_setup, entryCount, &params));
}

int ioUringEnter(
    const int ringFileDescriptor,
    const std::uint32_t submitCount,
    const std::uint32_t minCompleteCount,
    const std::uint32_t flags) noexcept
{
  return static_cast<int>(::syscall(
      __NR_io_uring_enter,
      ringFileDescriptor,
      submitCount,
      minCompleteCount,
      flags,
      nullptr,
      std::size_t{0}));
}

int ioUringRegister(
    const int ringFileDescriptor,
    const std::uint32_t opcode,
    void* const argument,
    const std::uint32_t argumentCount) noexcept
{
  return static_cast<int>(
      ::syscall(__NR_io_uring_register, ringFileDescriptor, opcode, argument, argumentCount));
}

// NOLINTEND(cppcoreguidelines-pro-type-vararg,hicpp-vararg)

/// Bounded waits for completion-queue room before the ring is declared stalled.
constexpr auto kMaxCompletionWaits = 1000;

/// One WILLNEED request's maximum length: the kernel clamps each readahead request to the file's
/// readahead limit (128 KiB by default), so a longer request silently drops its tail.
constexpr auto kReadAheadChunkBytes = std::uint64_t{128ULL * 1024ULL};

/// Any other request's maximum length: older kernels carry the fadvise length in 32 bits, so
/// ranges are chunked well below that.
constexpr auto kChunkBytes = std::uint64_t{1024ULL * 1024ULL * 1024ULL};

/// True when the kernel reports the fadvise opcode as usable on this ring.
bool supportsFadvise(const int ringFileDescriptor)
{
  // Probe every opcode up to and including fadvise; the kernel fills what it knows.
  constexpr auto kProbedOpCount = std::uint32_t{IORING_OP_FADVISE + 1U};
  auto probeBytes = std::vector<std::byte>(
      sizeof(io_uring_probe) + (kProbedOpCount * sizeof(io_uring_probe_op)));
  if(ioUringRegister(ringFileDescriptor, IORING_REGISTER_PROBE, probeBytes.data(), kProbedOpCount) <
     0)
  {
    return false;
  }

  const auto probeSpan = std::span<const std::byte>{probeBytes};
  const auto* const probe = common::startLifetimeAs<io_uring_probe>(probeSpan.data());
  const auto* const fadviseOp = common::startLifetimeAs<io_uring_probe_op>(
      probeSpan.subspan(sizeof(io_uring_probe) + (IORING_OP_FADVISE * sizeof(io_uring_probe_op)))
          .data());
  return probe->ops_len > IORING_OP_FADVISE and (fadviseOp->flags & IO_URING_OP_SUPPORTED) != 0U;
}

/// A shared ring cursor: one of the head/tail counters the kernel exports by byte offset into
/// the ring mapping.
std::uint32_t* ringCursor(const std::span<std::byte> ringBytes, const std::uint32_t offset)
{
  return common::startLifetimeAs<std::uint32_t>(ringBytes.subspan(offset).data());
}

} // namespace

void FileRange::cover(const std::uint64_t offset, const std::uint64_t size) noexcept
{
  mBegin = std::min(mBegin, offset);
  mEnd = std::max(mEnd, offset + size);
}

std::uint64_t FileRange::length() const noexcept
{
  return mEnd - mBegin;
}

AdviceRing::AdviceRing(const std::uint32_t entryCount): mRing{setUp(entryCount)} {}

AdviceRing::~AdviceRing() = default;

std::optional<AdviceRing::Ring> AdviceRing::setUp(const std::uint32_t entryCount)
{
  // No setup flags on purpose. A Reader may be constructed on one thread and iterated on
  // another, so submissions migrate threads; the cooperative task-running modes tie completion
  // delivery to the original submitter re-entering the ring and deadlock once that thread goes
  // idle. The default signal-based delivery posts completions regardless.
  auto params = io_uring_params{};
  auto instance = containers::File{ioUringSetup(entryCount, params)};
  if(instance.nativeHandle() < 0 or
     (params.features & IORING_FEAT_SINGLE_MMAP) == 0U or
     not supportsFadvise(instance.nativeHandle()))
  {
    return std::nullopt;
  }

  // Advice work needs little parallelism; cap the kernel's worker pool so a deep queue cannot
  // spawn a thread herd. Best effort: an old kernel without the register op still works.
  auto workerCaps = std::array<std::uint32_t, 2>{4U, 4U};
  static_cast<void>(ioUringRegister(
      instance.nativeHandle(),
      IORING_REGISTER_IOWQ_MAX_WORKERS,
      workerCaps.data(),
      workerCaps.size()));

  // Both ring headers share one mapping under IORING_FEAT_SINGLE_MMAP; its size is whichever of
  // the submission indirection array and the completion entries ends later.
  const auto submissionArrayEnd = params.sq_off.array + (params.sq_entries * sizeof(std::uint32_t));
  const auto completionEntriesEnd = params.cq_off.cqes + (params.cq_entries * sizeof(io_uring_cqe));
  try
  {
    auto headers = containers::Mapping::readWrite(
        instance,
        std::max<std::size_t>(submissionArrayEnd, completionEntriesEnd),
        IORING_OFF_SQ_RING);
    auto submissionEntries = containers::Mapping::readWrite(
        instance,
        params.sq_entries * sizeof(io_uring_sqe),
        IORING_OFF_SQES);

    const auto headerBytes = headers.bytes();
    const auto submissionMask = *ringCursor(headerBytes, params.sq_off.ring_mask);

    // The submission indirection array never changes: slot i always submits entry i.
    std::ranges::iota(
        common::startLifetimeAsArray<std::uint32_t>(headerBytes.subspan(
            params.sq_off.array,
            (submissionMask + std::size_t{1}) * sizeof(std::uint32_t))),
        0U);

    return Ring{
        .mInstance = std::move(instance),
        .mHeaders = std::move(headers),
        .mSubmissionEntries = std::move(submissionEntries),
        .mSubmissionHead = ringCursor(headerBytes, params.sq_off.head),
        .mSubmissionTail = ringCursor(headerBytes, params.sq_off.tail),
        .mSubmissionMask = submissionMask,
        .mCompletionHead = ringCursor(headerBytes, params.cq_off.head),
        .mCompletionTail = ringCursor(headerBytes, params.cq_off.tail),
        .mCompletionMask = *ringCursor(headerBytes, params.cq_off.ring_mask),
        .mCompletionEntries = headerBytes.subspan(params.cq_off.cqes)};
  }
  catch(const std::runtime_error& error)
  {
    logger::debug(
        "The advice ring could not be mapped ({}); advice stays synchronous.",
        error.what());
    return std::nullopt;
  }
}

bool AdviceRing::asynchronous() const noexcept
{
  return mRing.has_value();
}

void AdviceRing::advise(const int fileDescriptor, const FileRange range, const int advice)
{
  const auto chunkBytes = advice == POSIX_FADV_WILLNEED ? kReadAheadChunkBytes : kChunkBytes;
  for(auto chunkBegin = range.mBegin; chunkBegin < range.mEnd; chunkBegin += chunkBytes)
  {
    const auto chunkLength = std::min(chunkBytes, range.mEnd - chunkBegin);
    adviseChunk(fileDescriptor, chunkBegin, static_cast<std::uint32_t>(chunkLength), advice);
  }
}

void AdviceRing::adviseChunk(
    const int fileDescriptor, // NOLINT(bugprone-easily-swappable-parameters)
    const std::uint64_t offset,
    const std::uint32_t length,
    const int advice)
{
  // A full queue submits to make room; submit() may consume fewer entries than asked or fail, so
  // room is re-checked until it exists or the ring gives up.
  while(mRing and *mRing->mSubmissionTail -
                          std::atomic_ref{*mRing->mSubmissionHead}.load(std::memory_order_acquire) >
                      mRing->mSubmissionMask)
  {
    submit();
  }

  if(not mRing)
  {
    static_cast<void>(::posix_fadvise(
        fileDescriptor,
        static_cast<off_t>(offset),
        static_cast<off_t>(length),
        advice));
    return;
  }

  auto& ring = *mRing;
  const auto slot = *ring.mSubmissionTail & ring.mSubmissionMask;
  auto* const entry = common::startLifetimeAs<io_uring_sqe>(
      ring.mSubmissionEntries.bytes().subspan(slot * sizeof(io_uring_sqe)).data());
  *entry = io_uring_sqe{};
  entry->opcode = IORING_OP_FADVISE;
  entry->fd = fileDescriptor;
  // NOLINTBEGIN(cppcoreguidelines-pro-type-union-access): the submission entry overlays these
  // fields in unions by kernel definition; naming them is the ABI.
  entry->off = offset;
  entry->len = length;
  entry->fadvise_advice = static_cast<std::uint32_t>(advice);
  // NOLINTEND(cppcoreguidelines-pro-type-union-access)

  std::atomic_ref{*ring.mSubmissionTail}.store(
      *ring.mSubmissionTail + 1U,
      std::memory_order_release);
  ++ring.mPendingCount;
}

void AdviceRing::submit()
{
  if(not mRing)
  {
    return;
  }
  auto& ring = *mRing;
  reapCompletions(ring);

  if(ring.mPendingCount == 0U or not awaitCompletionCapacity(ring))
  {
    return;
  }

  const auto submitted = ioUringEnter(ring.mInstance.nativeHandle(), ring.mPendingCount, 0U, 0U);
  if(submitted >= 0)
  {
    ring.mInFlightCount += static_cast<std::uint32_t>(submitted);
    ring.mPendingCount -= static_cast<std::uint32_t>(submitted);
  }
  else if(errno != EINTR)
  {
    fail(std::generic_category().message(errno));
  }
}

bool AdviceRing::awaitCompletionCapacity(Ring& ring)
{
  // The completion queue is twice the submission queue; keeping in-flight below the completion
  // capacity guarantees no completion is ever dropped. Advice completes in microseconds to
  // milliseconds, so waiting here is rare and short; a ring that stops making progress is torn
  // down rather than waited on forever.
  const auto completionCapacity = ring.mCompletionMask + 1U;
  auto waitCount = 0;
  while(ring.mInFlightCount + ring.mPendingCount > completionCapacity)
  {
    if(ioUringEnter(ring.mInstance.nativeHandle(), 0U, 1U, IORING_ENTER_GETEVENTS) < 0 and
       errno != EINTR)
    {
      fail(std::generic_category().message(errno));
      return false;
    }
    if(++waitCount > kMaxCompletionWaits)
    {
      fail("the kernel stopped completing advice requests");
      return false;
    }
    reapCompletions(ring);
  }
  return true;
}

void AdviceRing::fail(const std::string_view reason)
{
  mRing.reset();
  logger::warn(
      "The advice ring stopped accepting submissions ({}); falling back to synchronous advice.",
      reason);
}

void AdviceRing::reapCompletions(Ring& ring)
{
  const auto tail = std::atomic_ref{*ring.mCompletionTail}.load(std::memory_order_acquire);
  auto head = *ring.mCompletionHead;

  while(head != tail)
  {
    const auto slot = head & ring.mCompletionMask;
    const auto* const completion = common::startLifetimeAs<io_uring_cqe>(
        std::span<const std::byte>{ring.mCompletionEntries}
            .subspan(slot * sizeof(io_uring_cqe))
            .data());
    if(completion->res < 0 and mReportedErrors.insert(completion->res).second)
    {
      // Advice is best effort; report each failure kind once so a systematically failing
      // opcode is visible without flooding the log.
      logger::debug(
          "An asynchronous file-advice operation failed: {}",
          std::generic_category().message(-completion->res));
    }
    ++head;
    --ring.mInFlightCount;
  }

  std::atomic_ref{*ring.mCompletionHead}.store(head, std::memory_order_release);
}

} // namespace nioc::chronicle

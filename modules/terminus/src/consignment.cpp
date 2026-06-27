////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <atomic>
#include <nioc/chronicle/crate.hpp>
#include <nioc/terminus/consignment.hpp>
#include <utility>

namespace nioc::terminus
{

Consignment::Consignment(chronicle::Crate crate, std::atomic_uint32_t& counter):
  mCrate{std::move(crate)},
  mCounter{&counter}
{
  mCounter->fetch_add(1, std::memory_order_relaxed);
}

Consignment::Consignment(Consignment&& other) noexcept:
  mCrate{std::move(other.mCrate)},
  mCounter{std::exchange(other.mCounter, nullptr)}
{
}

Consignment& Consignment::operator=(Consignment&& other) noexcept
{
  if(this != &other)
  {
    if(mCounter != nullptr && mCounter->fetch_sub(1, std::memory_order_release) == 1)
    {
      mCounter->notify_all();
    }

    mCrate = std::move(other.mCrate);
    mCounter = std::exchange(other.mCounter, nullptr);
  }

  return *this;
}

Consignment::~Consignment()
{
  if(mCounter != nullptr && mCounter->fetch_sub(1, std::memory_order_release) == 1)
  {
    mCounter->notify_all();
  }
}

const chronicle::Crate& Consignment::crate() const noexcept
{
  return mCrate;
}

} // namespace nioc::terminus

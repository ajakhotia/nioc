////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <iterator>
#include <nioc/terminus/logPlayer.hpp>
#include <utility>

namespace nioc::terminus
{

LogPlayer::LogPlayer(Port& port, std::filesystem::path inputLog):
  Driver{port, "LogPlayer"},
  mReader{std::move(inputLog)},
  mCursor{mReader.begin()}
{
}

LogPlayer::State LogPlayer::run()
{
  if(shutdownToken().stop_requested() or mCursor == std::default_sentinel)
  {
    return State::Done;
  }

  port().deliver(mCursor->mChannelId, mCursor->mCrate);
  ++mCursor;

  return mCursor == std::default_sentinel ? State::Done : State::Continue;
}

} // namespace nioc::terminus

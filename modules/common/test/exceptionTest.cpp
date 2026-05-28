////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <nioc/common/exception.hpp>
#include <stdexcept>
#include <string>

namespace nioc::common
{
namespace
{

TEST(ThrowException, ThrowsRequestedTypeWithFormattedMessage)
{
  try
  {
    throwException<std::invalid_argument>("value {} is out of range [{}, {}]", 7, 0, 5);
  }
  catch(const std::invalid_argument& e)
  {
    const auto what = std::string{ e.what() };
    EXPECT_NE(what.find("value 7 is out of range [0, 5]"), std::string::npos);

    // The call site is captured automatically and prepended as "[file:line] ".
    EXPECT_NE(what.find("exceptionTest.cpp:"), std::string::npos);
    return;
  }
  ADD_FAILURE() << "throwException did not throw";
}

TEST(ThrowException, PropagatesRequestedExceptionType)
{
  EXPECT_THROW(throwException<std::runtime_error>("boom"), std::runtime_error);
}

} // namespace
} // namespace nioc::common

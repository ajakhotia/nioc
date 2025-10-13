////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <nioc/chronicle/defines.hpp>
#include <nioc/common/makeBimap.hpp>

using namespace std::string_literals;

namespace nioc::chronicle
{
namespace
{

const auto& ioMechanismBimap()
{
  static const auto kBimap = common::makeBimap({
      std::make_pair(IoMechanism::Stream, "Stream"s),
      std::make_pair(IoMechanism::Mmap, "Mmap"s),
  });
  return kBimap;
}

} // namespace

std::string stringFromIoMechanism(const IoMechanism mechanism)
{
  const auto& bimap = ioMechanismBimap();
  const auto iter = bimap.left.find(mechanism);

  if(iter == bimap.left.end())
  {
    throw std::out_of_range(
        "[Chronicle] Unknown IoMechanism: " + std::to_string(static_cast<int>(mechanism)));
  }

  return iter->second;
}

IoMechanism ioMechanismFromString(const std::string& str)
{
  const auto& bimap = ioMechanismBimap();
  const auto iter = bimap.right.find(str);

  if(iter == bimap.right.end())
  {
    throw std::out_of_range("[Chronicle] Unknown IoMechanism string: '" + str + "'");
  }

  return iter->second;
}

} // namespace nioc::chronicle

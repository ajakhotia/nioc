////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace nioc::geometry
{
/// @brief  To facilitate looking up traits of a class defined in
///         the geometry module. Use may define the traits for a give type
///         by simply defining a specialization of this class for the said type.
template<typename>
struct Traits;

} // End of namespace nioc::geometry.

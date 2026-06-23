////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <nioc/chronicle/reservation.hpp>
#include <type_traits>

namespace nioc::chronicle
{

static_assert(std::is_move_constructible_v<Reservation>);
static_assert(std::is_move_assignable_v<Reservation>);
static_assert(not std::is_copy_constructible_v<Reservation>);

} // namespace nioc::chronicle

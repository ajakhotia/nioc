////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstdint>

namespace nioc::terminus
{

/// @brief The Cap'n Proto type id of @p Schema.
template<typename Schema>
inline constexpr std::uint64_t kSchemaId = static_cast<std::uint64_t>(
    Schema::_capnpPrivate::typeId);

} // namespace nioc::terminus

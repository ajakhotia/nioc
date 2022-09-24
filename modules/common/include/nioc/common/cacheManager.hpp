////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                             /
// Project  : nioc                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <optional>

namespace nioc::common
{
/// @brief  Manages an implementation of CacheBase. This class hides the cache implementation
///         privately and checks for the validity of the cache and resets if the check return
///         false.
/// @tparam Cache   The type of cache to be managed. This type must derive from CacheBase and
///                 must provide access to ControlParameter type and imp
template<typename Cache>
class CacheManager
{
public:
    CacheManager(): mCacheOpt(std::nullopt) {}

    CacheManager(const CacheManager&) = default;

    CacheManager(CacheManager&&) noexcept = default;

    ~CacheManager() = default;

    CacheManager& operator=(const CacheManager&) = default;

    CacheManager& operator=(CacheManager&&) noexcept = default;


    template<typename... Args>
    Cache& access(Args&&... args)
    {
        if(not mCacheOpt || not mCacheOpt->validate(args...))
        {
            mCacheOpt = std::make_optional<Cache>(std::forward<Args>(args)...);
        }

        return *mCacheOpt;
    }

private:
    std::optional<Cache> mCacheOpt;
};


} // namespace nioc::common

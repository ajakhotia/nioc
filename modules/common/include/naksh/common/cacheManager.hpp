////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021.                                                                                                 /
// Project  : Naksh                                                                                                    /
// Author   : Anurag Jakhotia                                                                                          /
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <optional>

namespace naksh::common
{


/// @brief  Base class to implement a cache to be managed by CacheManager.
/// @tparam ControlParameter_   Parameter type to the valid(...) that determines
///                             whether or not the cache is valid for re-use.
template<typename ControlParameter_>
class CacheBase
{
public:
    using ControlParameter = ControlParameter_;

    CacheBase() = default;

    CacheBase(const CacheBase&) = default;

    CacheBase(CacheBase&&) noexcept = default;

    virtual ~CacheBase() = default;

    CacheBase& operator=(const CacheBase&) = default;

    CacheBase& operator=(CacheBase&&) noexcept = default;

    [[nodiscard]] virtual bool valid(const ControlParameter& controlParameter) const noexcept = 0;
};


/// @brief  Manages an implementation of CacheBase. This class hides the cache implementation
///         privately and checks for the validity of the cache and resets if the check return
///         false.
/// @tparam Cache   The type of cache to be managed. This type must derive from CacheBase and
///                 must provide access to ControlParameter type and imp
template<typename Cache>
class CacheManager
{
public:
    static_assert(
        std::is_base_of_v<CacheBase<typename Cache::ControlParameter>, Cache>,
        "Cache type must derive from CacheBase for use with CacheManager.");

    CacheManager(): mCacheOpt(std::nullopt)
    {
    }

    CacheManager(const CacheManager&) = default;

    CacheManager(CacheManager&&) noexcept = default;

    ~CacheManager() = default;

    CacheManager& operator=(const CacheManager&) = default;

    CacheManager& operator=(CacheManager&&) noexcept = default;


    template<typename... Args>
    Cache& access(Args&&... args)
    {
        if(not mCacheOpt || not mCacheOpt->valid(args...))
        {
            mCacheOpt = std::make_optional<Cache>(std::forward<Args>(args)...);
        }

        return *mCacheOpt;
    }

private:
    std::optional<Cache> mCacheOpt;
};


} // End of namespace naksh::common.

////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <optional>

namespace nioc::common
{
/// @brief Caches a value and rebuilds it only when it goes stale.
///
/// On each @ref access the cached value is reused if it is still valid for the given arguments,
/// otherwise it is rebuilt from them.
///
/// @tparam Cache Cached type. Must be constructible from the access arguments and expose
/// `bool validate(args...)` returning whether the current value is still valid for those arguments.
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

  /// @brief Returns the cache, rebuilding it first if it is stale or not yet built.
  ///
  /// @param args Forwarded to `Cache::validate` and, when a rebuild is needed, to the Cache
  /// constructor.
  ///
  /// @return Reference to the valid cache.
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

////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.
// Project  : nioc
// Author   : Anurag Jakhotia
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <optional>

namespace nioc::common
{
/// @brief Lazy cache that rebuilds when stale.
///
/// Holds a cache and checks if it's still valid on each access. Rebuilds only when needed.
///
/// Cache type must provide: `bool validate(args...)` to check validity.
///
/// @tparam Cache Type of cache to manage.
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

  /// @brief Gets the cache, rebuilding if needed.
  /// @param args Passed to cache constructor and validate() method.
  /// @return Reference to valid cache.
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

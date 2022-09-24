////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : nioc                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <memory>
#include <span>

namespace nioc::logger
{

class MemoryCrate
{
public:
    class MemoryCrateImpl;

    explicit MemoryCrate(std::shared_ptr<MemoryCrateImpl> memoryCrateImplPtr);

    MemoryCrate(const MemoryCrate& memoryCrate);

    MemoryCrate(MemoryCrate&& memoryCrate) noexcept;

    ~MemoryCrate();

    MemoryCrate& operator=(const MemoryCrate& memoryCrate);

    MemoryCrate& operator=(MemoryCrate&& memoryCrate) noexcept;

    [[nodiscard]] std::span<const std::byte> span() const;

private:
    std::shared_ptr<MemoryCrateImpl> mMemoryCrateImplPtr;
};


} // namespace nioc::logger

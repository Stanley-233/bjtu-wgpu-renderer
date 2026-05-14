#pragma once

#include <cstdint>

template <typename T>
struct AssetId {
    uint32_t value = 0;

    [[nodiscard]] bool IsValid() const {
        return value != 0;
    }

    constexpr bool operator==(const AssetId&) const = default;
};

#pragma once

#include "AssetId.h"

template <typename T>
struct AssetHandle {
    AssetId<T> id{};
    const T*   asset = nullptr;

    [[nodiscard]] bool IsLoaded() const {
        return id.IsValid() && asset != nullptr;
    }

    explicit operator bool() const {
        return IsLoaded();
    }
};

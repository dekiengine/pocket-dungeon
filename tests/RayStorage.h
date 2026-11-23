#pragma once

// A raycaster's working memory for the previews, which use one at a time:
// room for the biggest view.

#include "../src/render/Raycaster.h"

#include <cstdint>
#include <vector>

// More than StorageBytes(320, 320, 1) on a PC, where pointers are wide.
inline constexpr size_t kRayStorageBytes = 96 * 1024;

inline void* RayStorage()
{
    static std::vector<uint64_t> storage(kRayStorageBytes / 8);
    return storage.data();
}

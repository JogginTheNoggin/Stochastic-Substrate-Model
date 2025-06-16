// LayerType.h (or a common types header)
#pragma once
#include <cstdint>

enum class LayerType : uint8_t {
    INPUT_LAYER = 0,
    OUTPUT_LAYER = 1,
    // Skipping 2 for now as per your numbering
    INTERNAL_LAYER = 3 
};
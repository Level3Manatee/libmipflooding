#include <cstdint>
#include <cmath>
#include <algorithm>

#include "../libmipflooding.h"


uint8_t libmipflooding::get_mip_count(const uint_fast16_t width, const uint_fast16_t height)
{
    return static_cast<uint8_t>(std::log2(std::min(width, height)));
}


uint8_t libmipflooding::channel_mask_from_array(const bool* array, const uint_fast8_t element_count)
{
    uint8_t mask = 0;
    for (uint_fast8_t i = 0; i < element_count; ++i)
    {
        if (array[i])
            mask |= 1 << i;
    }
    return mask;
}


void libmipflooding::free_mips_memory(const uint_fast8_t mip_count, float** mips_output, uint8_t** masks_output)
{
    for (int i = 0; i < mip_count; ++i)
    {
        delete[] mips_output[i];
        delete[] masks_output[i];
    }
}
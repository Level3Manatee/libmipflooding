#pragma once

inline float to_linear(const float in_sRGB) {
    if (in_sRGB <= 0.04045f)
        return in_sRGB / 12.92f;
    else
        return pow((in_sRGB + 0.055f) / 1.055f, 2.4f);
}


inline float to_sRGB(const float in_linear) {
    if (in_linear <= 0.0031308f)
        return in_linear * 12.92f;
    else
        return 1.055f * pow(in_linear, 1.0f / 2.4f) - 0.055f;
}


namespace libmipflooding
{
    inline uint8_t get_mip_count(const uint_fast16_t width, const uint_fast16_t height)
    {
        return static_cast<uint8_t>(std::log2(std::min(width, height)));
    }


    inline uint8_t channel_mask_from_array(const bool* array, const uint_fast8_t element_count)
    {
        uint8_t mask = 0;
        for (uint_fast8_t i = 0; i < element_count; ++i)
        {
            if (array[i])
                mask |= 1 << i;
        }
        return mask;
    }


    inline void free_mips_memory(const uint_fast8_t mip_count, float** mips_output, uint8_t** masks_output)
    {
        for (int i = 0; i < mip_count; ++i)
        {
            delete[] mips_output[i];
            delete[] masks_output[i];
        }
    }
}
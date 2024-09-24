#pragma once

#include <cmath>

inline float to_linear(const float in_sRGB) {
    if (in_sRGB <= 0.04045f)
        return in_sRGB / 12.92f;
    else
        return powf((in_sRGB + 0.055f) / 1.055f, 2.4f);
}


inline float to_sRGB(const float in_linear) {
    if (in_linear <= 0.0031308f)
        return in_linear * 12.92f;
    else
        return 1.055f * powf(in_linear, 1.0f / 2.4f) - 0.055f;
}

inline bool is_power_of_two(const size_t number)
{
    // powers of two have exactly 1 bit set, ANDing them with number-1 yields 0
    // e.g. 16: 10000 & 01000 == 00000, but 18: 10010 & 10001 == 10000
    return number > 0 && ((number & (number - 1)) == 0);
}
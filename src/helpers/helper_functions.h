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

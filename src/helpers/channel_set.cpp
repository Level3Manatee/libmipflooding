#include <cstdint>
#ifdef _DEBUG
#include <sstream>
#endif

#include "channel_set.h"

channel_set::channel_set(const uint8_t channel_mask, const uint_fast8_t channel_stride): channel_count_(0)
{
    if (channel_mask == 0)
    {
        channel_count_ = channel_stride;
        for (uint_fast8_t i = 0; i < channel_stride; ++i)
        {
            channel_mask_[i] = true;
            channels_[i] = i;
        }
        return;
    }
    for (uint_fast8_t i = 0; i < channel_stride; ++i)
    {
        if((channel_mask & (1 << i)) != 0)
        {
            channel_mask_[i] = true;
            channels_[channel_count_] = i;
            ++channel_count_;
        }
    }
}

channel_set::channel_set(const bool* array, const uint_fast8_t channel_count): channel_count_(0)
{
    for (uint_fast8_t i = 0; i < channel_count; ++i)
    {
        if (array[i])
        {
            channel_mask_[i] = true;
            channels_[channel_count_] = i;
            ++channel_count_;
        }
    }
}

channel_set::channel_set(const uint_fast8_t channel_count): channel_count_(channel_count)
{
    for (uint_fast8_t i = 0; i < channel_count; ++i)
    {
        channel_mask_[i] = true;
        channels_[i] = i;
    }
}

std::string channel_set::to_string() const
{
    std::stringstream str;
    str << "[";
    for (int i = 0; i < channel_count_; ++i)
    {
        str << std::to_string(channels_[i]);
        if (i < channel_count_ - 1)
            str << ',';
    }
    str << "]";
    return str.str();
}

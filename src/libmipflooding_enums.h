#pragma once

#include <cstdint>


/**
 * Data type (for C interface)
 */
enum LMF_DATA_TYPE : uint8_t
{
    UINT8 = 0,
    UINT16 = 1,
    FLOAT32 = 2,

    MAX_VAL = 3
};

/**
 * Status returns
 */
enum LMF_STATUS : uint8_t
{
    UNKNOWN = 0,
            
    SUCCESS = 1,

    UNSUPPORTED_DIMENSIONS = 2,
    UNSUPPORTED_DATA_TYPE = 3,
    UNSUPPORTED_CHANNEL_STRIDE = 4,
    START_ROW_OUT_OF_BOUNDS = 5
};

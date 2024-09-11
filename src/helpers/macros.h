// explicit template instantiation
#define INSTANTIATE_TYPES_1(FUNC) \
    template FUNC(uint8_t); \
    template FUNC(uint16_t); \
    template FUNC(float);
#define INSTANTIATE_TYPES_INNER(FUNC, OUTER_T) \
    template FUNC(OUTER_T, uint8_t); \
    template FUNC(OUTER_T, uint16_t); \
    template FUNC(OUTER_T, float);
#define INSTANTIATE_TYPES_2(FUNC) \
    INSTANTIATE_TYPES_INNER(FUNC, uint8_t) \
    INSTANTIATE_TYPES_INNER(FUNC, uint16_t) \
    INSTANTIATE_TYPES_INNER(FUNC, float)


// extern C function call permutations
#define CALL_1_PARAM(PARAM) \
    switch (PARAM) \
    { \
        case UINT8: \
            CALL(uint8_t); \
            break; \
        case UINT16: \
            CALL(uint16_t); \
            break; \
        case FLOAT32: \
            CALL(float); \
            break; \
    }
#define CALL_2_PARAMS_INNER(OUTER_CASE, OUTER_T, INNER_PARAM) \
    case OUTER_CASE: \
        switch (INNER_PARAM) \
        { \
            case UINT8: \
                CALL(OUTER_T, uint8_t); \
                break; \
            case UINT16: \
                CALL(OUTER_T, uint16_t); \
                break; \
            case FLOAT32: \
                CALL(OUTER_T, float); \
                break; \
        } \
        break;
#define CALL_2_PARAMS(OUTER_PARAM, INNER_PARAM) \
    switch (OUTER_PARAM) \
    { \
        CALL_2_PARAMS_INNER(UINT8, uint8_t, INNER_PARAM) \
        CALL_2_PARAMS_INNER(UINT16, uint16_t, INNER_PARAM) \
        CALL_2_PARAMS_INNER(FLOAT32, float, INNER_PARAM) \
    }

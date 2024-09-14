#pragma once

/**
 * Template instantiation for templated functions with 1 type parameter
 *
 * This allows linking of template functions for the predefined types. Macro to reduce LOC and improve maintainability.
 * 
 * @param FUNC Function template that accepts one type argument
 */
#define INSTANTIATE_TYPES_1(FUNC) \
    template FUNC(uint8_t); \
    template FUNC(uint16_t); \
    template FUNC(float);

/**
 * Template instantiation for templated functions with 2 type parameters, inner macro
 * 
 * @param FUNC Function template that accepts two type arguments
 */
#define INSTANTIATE_TYPES_INNER(FUNC, OUTER_T) \
    template FUNC(OUTER_T, uint8_t); \
    template FUNC(OUTER_T, uint16_t); \
    template FUNC(OUTER_T, float);

/**
 * Template instantiation for templated functions with 2 type parameters
 *
 * This allows linking of template functions for the predefined types. Macro to reduce LOC and improve maintainability.
 * 
 * @param FUNC Function template that accepts two type arguments
 */
#define INSTANTIATE_TYPES_2(FUNC) \
    INSTANTIATE_TYPES_INNER(FUNC, uint8_t) \
    INSTANTIATE_TYPES_INNER(FUNC, uint16_t) \
    INSTANTIATE_TYPES_INNER(FUNC, float)


/**
 * Type switch statement macro for extern C functions (1 type parameter)
 *
 * Enables calling the typed C++ functions. Macro to reduce LOC and improve maintainability.
 * 
 * @param PARAM Parameter name containing a DATA_TYPE enum value
 */
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

/**
 * Type switch statement macro for extern C functions (2 type parameters, inner macro)
 * 
 * @param OUTER_CASE  First data type (enum DATA_TYPE value)
 * @param OUTER_T     First data type (C type)
 * @param INNER_PARAM Inner data type (C type)
 */
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

/**
 * Type switch statement macro for extern C functions (2 type parameters)
 *
 * Enables calling the typed C++ functions. Macro to reduce LOC and improve maintainability.
 * 
 * @param OUTER_PARAM First parameter name containing a DATA_TYPE enum value
 * @param INNER_PARAM Second parameter name containing a DATA_TYPE enum value
 */
#define CALL_2_PARAMS(OUTER_PARAM, INNER_PARAM) \
    switch (OUTER_PARAM) \
    { \
        CALL_2_PARAMS_INNER(UINT8, uint8_t, INNER_PARAM) \
        CALL_2_PARAMS_INNER(UINT16, uint16_t, INNER_PARAM) \
        CALL_2_PARAMS_INNER(FLOAT32, float, INNER_PARAM) \
    }

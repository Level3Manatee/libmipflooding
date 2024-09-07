#pragma once
#include <cstdint>

#define LIBMIPFLOODING_VERSION_MAJOR 0
#define LIBMIPFLOODING_VERSION_MINOR 1

#ifdef _WIN32
    #define EXPORT_SYMBOL __declspec(dllexport)
#else
    #define EXPORT_SYMBOL
#endif

#ifdef __cplusplus
namespace libmipflooding_c
{
    extern "C" {
#endif
        enum DATA_TYPE : uint8_t
        {
            UINT8,
            UINT16,
            FLOAT32
        };
        
        /*******************************************
        * HELPER FUNCTIONS
        *******************************************/
        #pragma region helper_functions
        
        EXPORT_SYMBOL uint8_t channel_mask_from_array(
            const bool* array,
            const uint8_t element_count
        );
        
        EXPORT_SYMBOL uint8_t get_mip_count(
            const uint16_t width,
            const uint16_t height
        );
        
        EXPORT_SYMBOL void free_mips_memory(
            const uint_fast8_t mip_count,
            float** mips_output,
            uint8_t** masks_output
        );
        
        EXPORT_SYMBOL void convert_linear_to_srgb(
            const uint_fast16_t width,
            const uint_fast16_t height_or_end_row,
            const uint_fast8_t channel_stride,
            float* image_in_out,
            const uint8_t channel_mask,
            const uint16_t start_row
        );
        
        EXPORT_SYMBOL void convert_linear_to_srgb_threaded(
            const uint_fast16_t width,
            const uint_fast16_t height_or_end_row,
            const uint_fast8_t channel_stride,
            float* image_in_out,
            const uint8_t channel_mask,
            const uint8_t max_threads
        );
        
        #pragma endregion helper_functions

    
        /*******************************************
        * SUBROUTINES
        *******************************************/
        #pragma region subroutines
        
        EXPORT_SYMBOL void convert_and_scale_down_weighted(
            const uint16_t output_width,
            const uint16_t output_height_or_end_row,
            const uint8_t channel_stride,
            const void* input_image,
            const DATA_TYPE input_data_type,
            const void* input_mask,
            const DATA_TYPE input_mask_data_type,
            float* output_image,
            uint8_t* output_mask,
            const float coverage_threshold = 0.999f,
            const bool convert_srgb_to_linear = false,
            const bool is_normal_map = false,
            const uint8_t channel_mask = 0,
            const bool scale_alpha_unweighted = false,
            const uint16_t start_row = 0
        );
        
        EXPORT_SYMBOL void convert_and_scale_down_weighted_threaded(
            const uint16_t output_width,
            const uint16_t output_height,
            const uint8_t channel_stride,
            const void* input_image,
            const DATA_TYPE input_data_type,
            const void* input_mask,
            const DATA_TYPE input_mask_data_type,
            float* output_image,
            uint8_t* output_mask,
            const float coverage_threshold = 0.999f,
            const bool convert_srgb_to_linear = false,
            const bool is_normal_map = false,
            const uint8_t channel_mask = 0,
            const bool scale_alpha_unweighted = false,
            const uint8_t max_threads = 0
        );
        
        EXPORT_SYMBOL void scale_down_weighted(
            const uint16_t output_width,
            const uint16_t output_height_or_end_row,
            const uint8_t channel_stride,
            const float* input_image,
            const uint8_t* input_mask,
            float* output_image,
            uint8_t* output_mask,
            const bool is_normal_map,
            const uint8_t channel_mask = 0,
            const bool scale_alpha_unweighted = false,
            const uint16_t start_row = 0
        );
        
        EXPORT_SYMBOL void scale_down_weighted_threaded(
            const uint16_t output_width,
            const uint16_t output_height,
            const uint8_t channel_stride,
            const float* input_image,
            const uint8_t* input_mask,
            float* output_image,
            uint8_t* output_mask,
            const bool is_normal_map,
            const uint8_t channel_mask = 0,
            const bool scale_alpha_unweighted = false,
            const uint8_t max_threads = 0
        );
        
        EXPORT_SYMBOL void composite_up(
            const uint16_t input_width,
            const uint16_t input_height_or_end_row,
            const uint8_t channel_stride,
            const float* input_image,
            float* output_image,
            const uint8_t* output_mask,
            const uint8_t channel_mask = 0,
            const uint16_t start_row = 0
        );
        
        EXPORT_SYMBOL void composite_up_threaded(
            const uint16_t input_width,
            const uint16_t input_height,
            const uint8_t channel_stride,
            const float* input_image,
            float* output_image,
            const uint8_t* output_mask,
            const uint8_t channel_mask = 0,
            const uint8_t max_threads = 0
        );
        
        EXPORT_SYMBOL void final_composite_and_convert(
            const uint16_t input_width,
            const uint16_t input_height_or_end_row,
            const uint8_t channel_stride,
            const float* input_image,
            void* output_image,
            const DATA_TYPE output_data_type,
            const void* mask,
            const DATA_TYPE mask_data_type,
            const float coverage_threshold = 0.999f,
            const bool convert_linear_to_srgb = false,
            const uint8_t channel_mask = 0,
            const uint16_t start_row = 0
        );
        
        EXPORT_SYMBOL void final_composite_and_convert_threaded(
            const uint16_t input_width,
            const uint16_t input_height,
            const uint8_t channel_stride,
            const float* input_image,
            void* output_image,
            const DATA_TYPE output_data_type,
            const void* mask,
            const DATA_TYPE mask_data_type,
            const float coverage_threshold = 0.999f,
            const bool convert_linear_to_srgb = false,
            const uint8_t channel_mask = 0,
            const uint8_t max_threads = 0
        );
        
        #pragma endregion subroutines
        
        
        /*******************************************
        * CORE FUNCTIONS
        *******************************************/
        #pragma region core_functions
        
        EXPORT_SYMBOL bool generate_mips(
            void* image_in_out,
            const DATA_TYPE image_data_type,
            const uint16_t image_width,
            const uint16_t image_height,
            const uint8_t channel_stride,
            const uint8_t* image_mask,
            float** mips_output,
            uint8_t** masks_output,
            const float coverage_threshold = 0.999f,
            const bool convert_srgb = false,
            const bool is_normal_map = false,
            const uint8_t channel_mask = 0,
            const bool scale_alpha_unweighted = false,
            const uint8_t max_threads = 0
        );
        
        EXPORT_SYMBOL bool composite_mips(
            float** mips_in_out,
            const uint8_t** masks_input,
            const uint16_t image_width,
            const uint16_t image_height,
            const uint8_t channel_stride,
            const uint8_t channel_mask = 0,
            const uint8_t max_threads = 0
        );
        
        EXPORT_SYMBOL bool flood_image(
            void* image_in_out,
            const DATA_TYPE image_data_type,
            const uint16_t image_width,
            const uint16_t image_height,
            const uint8_t channel_stride,
            const void* image_mask,
            const DATA_TYPE mask_data_type,
            const float coverage_threshold = 0.99f,
            const bool convert_srgb = false,
            const bool is_normal_map = false,
            const uint8_t channel_mask = 0,
            const bool scale_alpha_unweighted = false,
            const uint8_t max_threads = 0
        );

        #pragma endregion core_functions

#ifdef __cplusplus
    } // extern "C"
} // namespace
#endif
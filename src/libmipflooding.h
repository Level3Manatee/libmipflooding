#pragma once

#include <cstdint>

#define LIBMIPFLOODING_VERSION_MAJOR 0
#define LIBMIPFLOODING_VERSION_MINOR 1

namespace libmipflooding
{
    /*******************************************
     * HELPER FUNCTIONS
     *******************************************/
    #pragma region helper_functions

    uint8_t get_mip_count(
        const uint_fast16_t width,
        const uint_fast16_t height
    );

    uint8_t channel_mask_from_array(
        const bool* array,
        const uint_fast8_t element_count
    );

    void free_mips_memory(
        const uint_fast8_t mip_count,
        float** mips_output,
        uint8_t** masks_output
    );

    void convert_linear_to_srgb(
        const uint_fast16_t width,
        const uint_fast16_t height_or_end_row,
        const uint_fast8_t channel_stride,
        float* image_in_out,
        const uint8_t channel_mask,
        const uint_fast16_t start_row
    );

    void convert_linear_to_srgb_threaded(
        const uint_fast16_t width,
        const uint_fast16_t height_or_end_row,
        const uint_fast8_t channel_stride,
        float* image_in_out,
        const uint8_t channel_mask,
        const uint_fast8_t max_threads
    );
    
    #pragma endregion helper_functions

    
    /*******************************************
    * SUBROUTINES
    *******************************************/
    #pragma region subroutines

    template <typename InputT, typename InputMaskT>
    void convert_and_scale_down_weighted(
        const uint_fast16_t output_width,
        const uint_fast16_t output_height_or_end_row,
        const uint_fast8_t stride,
        const InputT* input_image,
        const InputMaskT* input_mask,
        float* output_image,
        uint8_t* output_mask,
        const float coverage_threshold = 0.999f,
        const bool convert_srgb_to_linear = false,
        const bool is_normal_map = false,
        const uint8_t channel_mask = 0,
        const bool scale_alpha_unweighted = false,
        const uint_fast16_t start_row = 0
    );
    
    template <typename InputT, typename InputMaskT>
    void convert_and_scale_down_weighted_threaded(
        const uint_fast16_t output_width,
        const uint_fast16_t output_height,
        const uint_fast8_t stride,
        const InputT* input_image,
        const InputMaskT* input_mask,
        float* output_image,
        uint8_t* output_mask,
        const float coverage_threshold = 0.999f,
        const bool convert_srgb_to_linear = false,
        const bool is_normal_map = false,
        const uint8_t channel_mask = 0,
        const bool scale_alpha_unweighted = false,
        const uint_fast8_t max_threads = 0
    );

    void scale_down_weighted(
        const uint_fast16_t output_width,
        const uint_fast16_t output_height_or_end_row,
        const uint_fast8_t stride,
        const float* input_image,
        const uint8_t* input_mask,
        float* output_image,
        uint8_t* output_mask,
        const bool is_normal_map = false,
        const uint8_t channel_mask = 0,
        const bool scale_alpha_unweighted = false,
        const uint_fast16_t start_row = 0
    );
    
    void scale_down_weighted_threaded(
        const uint_fast16_t output_width,
        const uint_fast16_t output_height,
        const uint_fast8_t stride,
        const float* input_image,
        const uint8_t* input_mask,
        float* output_image,
        uint8_t* output_mask,
        const bool is_normal_map = false,
        const uint8_t channel_mask = 0,
        const bool scale_alpha_unweighted = false,
        const uint_fast8_t max_threads = 0
    );

    void composite_up(
        const uint_fast16_t input_width,
        const uint_fast16_t input_height_or_end_row,
        const uint_fast8_t channel_stride,
        const float* input_image,
        float* output_image,
        const uint8_t* output_mask,
        const uint8_t channel_mask = 0,
        const uint_fast16_t start_row = 0
    );

    void composite_up_threaded(
        const uint_fast16_t input_width,
        const uint_fast16_t input_height,
        const uint_fast8_t channel_stride,
        const float* input_image,
        float* output_image,
        const uint8_t* output_mask,
        const uint8_t channel_mask = 0,
        const uint_fast8_t max_threads = 0
    );

    template <typename OutputT, typename MaskT>
    void final_composite_and_convert(
        const uint_fast16_t input_width,
        const uint_fast16_t input_height_or_end_row,
        const uint_fast8_t channel_stride,
        const float* input_image,
        OutputT* output_image,
        const MaskT* mask,
        const float coverage_threshold = 0.999f,
        const bool convert_linear_to_srgb = false,
        const uint8_t channel_mask = 0,
        const uint_fast16_t start_row = 0
    );
    
    template <typename OutputT, typename MaskT>
    void final_composite_and_convert_threaded(
        const uint_fast16_t input_width,
        const uint_fast16_t input_height,
        const uint_fast8_t channel_stride,
        const float* input_image,
        OutputT* output_image,
        const MaskT* mask,
        const float coverage_threshold = 0.999f,
        const bool convert_linear_to_srgb = false,
        const uint8_t channel_mask = 0,
        const uint_fast8_t max_threads = 0
    );

    #pragma endregion subroutines

    
    /*******************************************
    * CORE FUNCTIONS
    *******************************************/
    #pragma region core_functions
    
    template <typename ImageT, typename MaskT>
    bool generate_mips(
        ImageT* image_in_out,
        const uint_fast16_t image_width,
        const uint_fast16_t image_height,
        const uint_fast8_t channel_stride,
        const MaskT* image_mask,
        float** mips_output,
        uint8_t** masks_output,
        const float coverage_threshold = 0.999f,
        const bool convert_srgb = false,
        const bool is_normal_map = false,
        const uint8_t channel_mask = 0,
        const bool scale_alpha_unweighted = false,
        const uint_fast8_t max_threads = 0
    );


    bool composite_mips(
        float** mips_in_out,
        const uint8_t** masks_input,
        const uint_fast16_t image_width,
        const uint_fast16_t image_height,
        const uint_fast8_t channel_stride,
        const uint8_t channel_mask = 0,
        const uint_fast8_t max_threads = 0
    );

    
    template <typename ImageT, typename MaskT>
    bool flood_image(
        ImageT* image_in_out,
        const uint_fast16_t image_width,
        const uint_fast16_t image_height,
        const uint_fast8_t channel_stride,
        const MaskT* image_mask,
        const float coverage_threshold = 0.999f,
        const bool convert_srgb = false,
        const bool is_normal_map = false,
        const uint8_t channel_mask = 0,
        const bool scale_alpha_unweighted = false,
        const uint_fast8_t max_threads = 0
    );

    #pragma endregion core_functions
}

#include "libmipflooding.h"
#include "libmipflooding_extern_c.h"
#include "helpers/macros.h"

/*******************************************
 * C EXPORTS
 *******************************************/
#pragma region c_exports

extern "C" {
    
    uint8_t libmipflooding_c::channel_mask_from_array(const bool* array, const uint8_t element_count)
    {
        return libmipflooding::channel_mask_from_array(array, element_count);
    }
    
    
    EXPORT_SYMBOL void libmipflooding_c::convert_to_type(
            const uint_fast16_t width,
            const uint_fast16_t height_or_end_row,
            const uint_fast8_t channel_stride,
            const float* image_in,
            void* image_out,
            const DATA_TYPE out_data_type,
            const bool convert_srgb,
            const uint8_t channel_mask,
            const uint_fast16_t start_row)
    {
        #define CALL(IMAGE_T) \
            libmipflooding::convert_to_type(width, height_or_end_row, channel_stride, image_in, static_cast<IMAGE_T*>(image_out), convert_srgb, channel_mask, start_row);
        CALL_1_PARAM(out_data_type)
        #undef CALL
    }
    
    
    EXPORT_SYMBOL void libmipflooding_c::convert_to_type_threaded(
            const uint_fast16_t width,
            const uint_fast16_t height_or_end_row,
            const uint_fast8_t channel_stride,
            const float* image_in,
            void* image_out,
            const DATA_TYPE out_data_type,
            const bool convert_srgb,
            const uint8_t channel_mask,
            const uint8_t max_threads)
    {
        #define CALL(IMAGE_T) \
        libmipflooding::convert_to_type_threaded(width, height_or_end_row, channel_stride, image_in, static_cast<IMAGE_T*>(image_out), convert_srgb, channel_mask, max_threads);
        CALL_1_PARAM(out_data_type)
        #undef CALL
    }
    
    
    void libmipflooding_c::free_mips_memory(const uint_fast8_t mip_count, float** mips_output, uint8_t** masks_output)
    {
        libmipflooding::free_mips_memory(mip_count, mips_output, masks_output);
    }
    
    
    void libmipflooding_c::convert_and_scale_down_weighted(
            const uint16_t output_width,
            const uint16_t output_height_or_end_row,
            const uint8_t channel_stride,
            const void* input_image,
            const DATA_TYPE input_data_type,
            const void* input_mask,
            const DATA_TYPE input_mask_data_type,
            float* output_image,
            uint8_t* output_mask,
            const float coverage_threshold,
            const bool convert_srgb_to_linear,
            const bool is_normal_map,
            const uint8_t channel_mask,
            const bool scale_alpha_unweighted,
            const uint16_t start_row)
    {
        #define CALL(IMAGE_T, MASK_T) \
            libmipflooding::convert_and_scale_down_weighted(output_width, output_height_or_end_row, channel_stride, static_cast<const IMAGE_T*>(input_image), static_cast<const MASK_T*>(input_mask), output_image, output_mask, coverage_threshold, convert_srgb_to_linear, is_normal_map, channel_mask, scale_alpha_unweighted, start_row)

        CALL_2_PARAMS(input_data_type, input_mask_data_type)

        #undef CALL
    }
    
    
    void libmipflooding_c::convert_and_scale_down_weighted_threaded(
            const uint16_t output_width,
            const uint16_t output_height,
            const uint8_t channel_stride,
            const void* input_image,
            const DATA_TYPE input_data_type,
            const void* input_mask,
            const DATA_TYPE input_mask_data_type,
            float* output_image,
            uint8_t* output_mask,
            const float coverage_threshold,
            const bool convert_srgb_to_linear,
            const bool is_normal_map,
            const uint8_t channel_mask,
            const bool scale_alpha_unweighted,
            const uint8_t max_threads)
    {
        #define CALL(IMAGE_T, MASK_T) \
            libmipflooding::convert_and_scale_down_weighted_threaded(output_width, output_height, channel_stride, static_cast<const IMAGE_T*>(input_image), static_cast<const MASK_T*>(input_mask), output_image, output_mask, coverage_threshold, convert_srgb_to_linear, is_normal_map, channel_mask, scale_alpha_unweighted, max_threads)
        
        CALL_2_PARAMS(input_data_type, input_mask_data_type)

        #undef CALL
    }
    
    
    void libmipflooding_c::scale_down_weighted(
            const uint16_t output_width,
            const uint16_t output_height_or_end_row,
            const uint8_t channel_stride,
            const float* input_image,
            const uint8_t* input_mask,
            float* output_image,
            uint8_t* output_mask,
            const bool is_normal_map,
            const uint8_t channel_mask,
            const bool scale_alpha_unweighted,
            const uint16_t start_row)
    {
        libmipflooding::scale_down_weighted(output_width, output_height_or_end_row, channel_stride, input_image, input_mask, output_image, output_mask, is_normal_map, channel_mask, scale_alpha_unweighted, start_row);
    }
    
    
    void libmipflooding_c::scale_down_weighted_threaded(
            const uint16_t output_width,
            const uint16_t output_height, 
            const uint8_t channel_stride,
            const float* input_image,
            const uint8_t* input_mask,
            float* output_image,
            uint8_t* output_mask,
            const bool is_normal_map,
            const uint8_t channel_mask,
            const bool scale_alpha_unweighted,
            const uint8_t max_threads)
    {
        libmipflooding::scale_down_weighted_threaded(output_width, output_height, channel_stride, input_image, input_mask, output_image, output_mask, is_normal_map, channel_mask, scale_alpha_unweighted, max_threads);
    }
    
    
    void libmipflooding_c::composite_up(
            const uint16_t input_width,
            const uint16_t input_height_or_end_row,
            const uint8_t channel_stride,
            const float* input_image,
            float* output_image,
            const uint8_t* output_mask,
            const uint8_t channel_mask,
            const uint16_t start_row)
    {
        libmipflooding::composite_up(input_width, input_height_or_end_row, channel_stride, input_image, output_image, output_mask, channel_mask, start_row);
    }
    
    
    void libmipflooding_c::composite_up_threaded(
            const uint16_t input_width,
            const uint16_t input_height,
            const uint8_t channel_stride,
            const float* input_image,
            float* output_image,
            const uint8_t* output_mask,
            const uint8_t channel_mask,
            const uint8_t max_threads)
    {
        libmipflooding::composite_up_threaded(input_width, input_height, channel_stride, input_image, output_image, output_mask, channel_mask, max_threads);
    }
    
    
    void libmipflooding_c::final_composite_and_convert(
            const uint16_t input_width,
            const uint16_t input_height_or_end_row,
            const uint8_t channel_stride,
            const float* input_image,
            void* output_image,
            const DATA_TYPE output_data_type,
            const void* mask,
            const DATA_TYPE mask_data_type,
            const float coverage_threshold,
            const bool convert_linear_to_srgb,
            const uint8_t channel_mask,
            const uint16_t start_row)
    {
        #define CALL(IMAGE_T, MASK_T) \
            libmipflooding::final_composite_and_convert(input_width, input_height_or_end_row, channel_stride, input_image, static_cast<IMAGE_T*>(output_image), static_cast<const MASK_T*>(mask), coverage_threshold, convert_linear_to_srgb, channel_mask, start_row)
        
        CALL_2_PARAMS(output_data_type, mask_data_type)

        #undef CALL
    }
    
    
    void libmipflooding_c::final_composite_and_convert_threaded(
            const uint16_t input_width,
            const uint16_t input_height,
            const uint8_t channel_stride,
            const float* input_image,
            void* output_image,
            const DATA_TYPE output_data_type,
            const void* mask,
            const DATA_TYPE mask_data_type,
            const float coverage_threshold,
            const bool convert_linear_to_srgb,
            const uint8_t channel_mask,
            const uint8_t max_threads)
    {
        #define CALL(IMAGE_T, MASK_T) \
            libmipflooding::final_composite_and_convert_threaded(input_width, input_height, channel_stride, input_image, static_cast<IMAGE_T*>(output_image), static_cast<const MASK_T*>(mask), coverage_threshold, convert_linear_to_srgb, channel_mask, max_threads)

        CALL_2_PARAMS(output_data_type, mask_data_type)
        
        #undef CALL
    }
    
    
    uint8_t libmipflooding_c::get_mip_count(
            const uint16_t width,
            const uint16_t height)
    {
        return libmipflooding::get_mip_count(width, height);
    }
    
    
    bool libmipflooding_c::generate_mips(
            void* image_in_out,
            const DATA_TYPE image_data_type,
            const uint16_t image_width,
            const uint16_t image_height,
            const uint8_t channel_stride,
            const uint8_t* image_mask,
            float** mips_output,
            uint8_t** masks_output,
            const float coverage_threshold,
            const bool convert_srgb,
            const bool is_normal_map,
            const uint8_t channel_mask,
            const bool scale_alpha_unweighted,
            const uint8_t max_threads)
    {
        #define CALL(IMAGE_T) \
            libmipflooding::generate_mips(static_cast<IMAGE_T*>(image_in_out), image_width, image_height, channel_stride, image_mask, mips_output, masks_output, coverage_threshold, convert_srgb, is_normal_map, channel_mask, scale_alpha_unweighted, max_threads)

        CALL_1_PARAM(image_data_type)
        
        return false;
        
        #undef CALL 
    }
    
    
    bool libmipflooding_c::composite_mips(
            float** mips_in_out,
            const uint8_t** masks_input,
            const uint16_t image_width,
            const uint16_t image_height,
            const uint8_t channel_stride,
            const uint8_t channel_mask,
            const uint8_t max_threads)
    {
        return libmipflooding::composite_mips(mips_in_out, masks_input, image_width, image_height, channel_stride, channel_mask, max_threads);
    }
    
    
    bool libmipflooding_c::flood_image(
            void* image_in_out,
            const DATA_TYPE image_data_type,
            const uint16_t image_width,
            const uint16_t image_height,
            const uint8_t channel_stride,
            const void* image_mask,
            const DATA_TYPE mask_data_type,
            const float coverage_threshold,
            const bool convert_srgb,
            const bool is_normal_map,
            const uint8_t channel_mask,
            const bool scale_alpha_unweighted,
            const uint8_t max_threads)
    {
        #define CALL(IMAGE_T, MASK_T) \
            return libmipflooding::flood_image(static_cast<IMAGE_T*>(image_in_out), image_width, image_height, channel_stride, static_cast<const MASK_T*>(image_mask), coverage_threshold, convert_srgb, is_normal_map, channel_mask, scale_alpha_unweighted, max_threads)

        CALL_2_PARAMS(image_data_type, mask_data_type)

        return false;

        #undef CALL 
    }
    
} // extern "C"

#pragma endregion c_exports

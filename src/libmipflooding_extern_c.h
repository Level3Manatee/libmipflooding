#pragma once
#include <cstdint>

#define LIBMIPFLOODING_VERSION_MAJOR 0
#define LIBMIPFLOODING_VERSION_MINOR 1

#ifdef _WIN32
    #define EXPORT_SYMBOL __declspec(dllexport)
#else
    #define EXPORT_SYMBOL __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
namespace libmipflooding_c
{
    extern "C" {
#endif
        enum DATA_TYPE : uint8_t
        {
            UINT8 = 0,
            UINT16 = 1,
            FLOAT32 = 2
        };
        
        /*******************************************
        * HELPER FUNCTIONS
        *******************************************/
        #pragma region helper_functions

        EXPORT_SYMBOL void convert_to_type(
            const uint_fast16_t width,
            const uint_fast16_t height_or_end_row,
            const uint_fast8_t channel_stride,
            const float* image_in,
            void* image_out,
            const DATA_TYPE out_data_type,
            const bool convert_srgb = false,
            const uint8_t channel_mask = 0,
            const uint_fast16_t start_row = 0
        );

        EXPORT_SYMBOL void convert_to_type_threaded(
            const uint_fast16_t width,
            const uint_fast16_t height_or_end_row,
            const uint_fast8_t channel_stride,
            const float* image_in,
            void* image_out,
            const DATA_TYPE out_data_type,
            const bool convert_srgb = false,
            const uint8_t channel_mask = 0,
            const uint8_t max_threads = 0
        );
        
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

        /**
         * Generate coverage-weighted mip maps
         *
         * Generates coverage-weighted mip maps and outputs them as a list of linear float (0..1) arrays
         * (excluding input image / mip 0)
         * 
         * @param image_in_out    Input image as void* pointer
         * @param image_data_type Input image type as DATA_TYPE enum value (UINT8=0 / UINT16=1 / FLOAT32=2)
         * @param image_width     Input image width in pixels (must be power of 2)
         * @param image_height    Input image height pixels (must be power of 2)
         * @param channel_stride  Number of total channels in image data
         * @param image_mask      (optional) Coverage mask of type uint8_t/uint16_t/float.
         *                        Pass nullptr to use last channel of input image instead.
         * @param mips_output     Array of pointers (float*) that is filled with pointers to the generated mip maps
         * @param masks_output    Array of pointers (uint8_t*) that is filled with pointers to the generated coverage masks
         * @param coverage_threshold (optional) Threshold to use for binarizing the input mask. Defaults to 0.999f.
         * @param convert_srgb    (optional) Convert sRGB to linear for correct scaling of sRGB textures (always returns linear data)
         * @param is_normal_map   (optional) Perform processing for normal maps.
         *                        Will re-normalize vectors to unit length at the moment, Slerp is planned.
         * @param channel_mask    (optional) Binary mask of channels to process, 0 = all channels.
         *                        You can use channel_mask_from_array() to generate a mask from an array of booleans.
         * @param scale_alpha_unweighted (optional) Scale the last channel without coverage weighting? i.e. regular box filtering
         * @param max_threads     (optional) Number of threads to use. 0 = auto (half of available threads,
         *                        which amounts to number of hardware cores for machines with SMT/HyperThreading)
         * @return true on success, false on error (always returns true right now)
         */
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

        /**
         * Composite mip levels to fill holes
         *
         * From smallest to largest, mip levels are consecutively scaled (nearest neighbor) and composited into each other. 
         * 
         * @param mips_in_out    Array of pointers to the mip maps (float, 0..1 range), compositing is done in-place
         * @param masks_input    Array of pointers to the mip coverage masks (uint8_t, treated as binary 0/1). Used for compositing. 
         * @param image_width    Width of original image (i.e. double the width of the largest mip map)
         * @param image_height   Height of original image (i.e. double the height of the largest mip map)
         * @param channel_stride Number of total channels in image data
         * @param channel_mask   (optional) Binary mask of channels to process, 0 = all channels.
         *                       You can use channel_mask_from_array() to generate a mask from an array of booleans.
         * @param max_threads    (optional) Number of threads to use. 0 = auto (half of available threads,
         *                       which amounts to number of hardware cores for machines with SMT/HyperThreading)
         * @return true on success, false on error (always returns true right now)
         */
        EXPORT_SYMBOL bool composite_mips(
            float** mips_in_out,
            const uint8_t** masks_input,
            const uint16_t image_width,
            const uint16_t image_height,
            const uint8_t channel_stride,
            const uint8_t channel_mask = 0,
            const uint8_t max_threads = 0
        );

        /**
         * Mip-flood an image
         *
         * Generates a mip-flooded image by generating and compositing coverage-scaled mip maps,
         * and then compositing the unmodified original image on top.
         * 
         * @param image_in_out    Input image of type uint8_t/uint16_t/float
         * @param image_data_type Input image type as DATA_TYPE enum value (UINT8=0 / UINT16=1 / FLOAT32=2)
         * @param image_width     Input image width in pixels (must be power of 2)
         * @param image_height    Input image height pixels (must be power of 2)
         * @param channel_stride  Number of total channels in image data
         * @param image_mask      (optional) Coverage mask of type uint8_t/uint16_t/float.
         *                        Pass nullptr to use last channel of input image instead.
         * @param mask_data_type  Mask data type as DATA_TYPE enum value (UINT8=0 / UINT16=1 / FLOAT32=2)
         * @param coverage_threshold (optional) Threshold to use for binarizing the input mask. Defaults to 0.999f.
         * @param convert_srgb    (optional) Convert sRGB to linear for correct scaling of sRGB textures?
         *                        Always returns linear data.
         * @param is_normal_map   (optional) Perform processing for normal maps. Will re-normalize vectors to unit length
         *                        at the moment, Slerp is planned.
         * @param channel_mask    (optional) Binary mask of channels to process, 0 = all channels.
         *                        You can use channel_mask_from_array() to generate a mask from an array of booleans.
         * @param scale_alpha_unweighted (optional) Scale the last channel without coverage weighting? i.e. regular box filtering
         * @param max_threads     (optional) Number of threads to use. 0 = auto (half of available threads,
         *                        which amounts to number of hardware cores for machines with SMT/HyperThreading)
         * @return true on success, false on error (always returns true right now)
         */
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
#pragma once
#include <cstdint>

#include "libmipflooding_enums.h"

#define LIBMIPFLOODING_VERSION_MAJOR 0
#define LIBMIPFLOODING_VERSION_MINOR 2

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


        /*******************************************
        * HELPER FUNCTIONS
        *******************************************/
        #pragma region helper_functions
        /// @defgroup helper_functions_c Helper functions (C API)
        /// @{
        
        /**
         * Converts a float image to uint8/uint16/float, optionally with sRGB conversion
         * 
         * @param width          Input image width in pixels (must be power of 2) 
         * @param height_or_end_row Input image height in pixels (must be power of 2), or end row for partial processing
         * @param channel_stride Number of total channels in image data
         * @param image_in       Pointer to input image, float*
         * @param image_out      Pointer to pre-allocated target, OutputT*
         * @param out_data_type  Output data type as LMF_DATA_TYPE enum value
         * @param convert_srgb   (optional) Convert linear to sRGB?
         * @param channel_mask   (optional) Bit mask of channels to process, 0 = all channels.
         *                       You can use channel_mask_from_array() to generate a mask from an array of booleans.
         * @param start_row      (optional) Start row for partial processing
         */
        EXPORT_SYMBOL void convert_to_type(
            const uint_fast16_t width,
            const uint_fast16_t height_or_end_row,
            const uint_fast8_t channel_stride,
            const float* image_in,
            void* image_out,
            const LMF_DATA_TYPE out_data_type,
            const bool convert_srgb = false,
            const uint8_t channel_mask = 0,
            const uint_fast16_t start_row = 0
        );

        /**
         * Converts a float image to uint8/uint16/float, optionally with sRGB conversion (threaded version)
         * 
         * @tparam OutputT       Output type, uint8/uint16/float
         * @param width          Input image width in pixels (must be power of 2) 
         * @param height_or_end_row Input image height in pixels (must be power of 2), or end row for partial processing
         * @param channel_stride Number of total channels in image data
         * @param image_in       Pointer to input image, float*
         * @param image_out      Pointer to pre-allocated target, OutputT*
         * @param out_data_type  Output data type as LMF_DATA_TYPE enum value
         * @param convert_srgb   (optional) Convert linear to sRGB?
         * @param channel_mask   (optional) Bit mask of channels to process, 0 = all channels.
         *                       You can use channel_mask_from_array() to generate a mask from an array of booleans.
         * @param max_threads    (optional) Number of threads to use. 0 = auto (half of available threads,
         *                       which amounts to number of hardware cores for machines with SMT/HyperThreading)
         */
        EXPORT_SYMBOL void convert_to_type_threaded(
            const uint_fast16_t width,
            const uint_fast16_t height_or_end_row,
            const uint_fast8_t channel_stride,
            const float* image_in,
            void* image_out,
            const LMF_DATA_TYPE out_data_type,
            const bool convert_srgb = false,
            const uint8_t channel_mask = 0,
            const uint8_t max_threads = 0
        );

        /**
         * Calculate the number of mip levels for a given resolution
         * 
         * @param width 
         * @param height 
         * @return Number of mip levels
         */
        EXPORT_SYMBOL uint8_t get_mip_count(
            const uint16_t width,
            const uint16_t height
        );
        
        /**
         * Returns a channel bit mask, for up to 8 channels
         * 
         * @param array 
         * @param element_count 
         * @return Bit mask
         */
        EXPORT_SYMBOL uint8_t channel_mask_from_array(
            const bool* array,
            const uint8_t element_count
        );

        /**
         * Release memory allocated by generate_mips()
         * 
         * @param mip_count 
         * @param mips_output 
         * @param masks_output 
         */
        EXPORT_SYMBOL void free_mips_memory(
            const uint_fast8_t mip_count,
            float** mips_output,
            uint8_t** masks_output
        );

        /// @}
        #pragma endregion helper_functions

    
        /*******************************************
        * SUBROUTINES
        *******************************************/
        #pragma region subroutines
        /// @defgroup subroutines_c Subroutines (C API)
        /// @{

        /**
         * Pre-process and scale down input image
         *
         * Converts the input image to float, and the input mask to binary (stored as uint8_t). If no input mask is
         * provided (nullptr), the last color channel of the input image is used as the mask (e.g. "A" for an RGBA image).
         *
         * Image and mask are scaled down to half their size, using the mask-weighted average for the image, and a threshold
         * for the mask (i.e. if any of the 4 mask pixels to be scaled is 1, the scaled pixel becomes 1).
         *
         * The input mask is initially processed using the coverage_threshold parameter, which defaults to 0.999f (anything
         * above 0.999f becomes 1, all other values become 0).
         *
         * The last image channel can optionally be scaled unweighted (regular box filtering). This can be useful when e.g.
         * the alpha channel should reflect the average opacity, as opposed to becoming 1.0f through weighted scaling in
         * cases where the alpha channel is identical to the coverage mask.
         * 
         * @param output_width   Output image width in pixels (must be power of 2) 
         * @param output_height_or_end_row Output image height in pixels (must be power of 2),
         *                       or end row for partial processing
         * @param channel_stride Number of total channels in image data
         * @param input_image    Pointer to input image, void*
         * @param input_data_type Input image type as LMF_DATA_TYPE enum value
         * @param input_mask     (optional) Pointer to input mask, void* or nullptr 
         * @param input_mask_data_type Input mask data type as LMF_DATA_TYPE enum value,
         *                       ignored when input mask pointer is nullptr
         * @param output_image   Pointer to pre-allocated output image, float*
         * @param output_mask    Pointer to pre-allocated output mask, uint8_t*
         * @param coverage_threshold (optional) Threshold to use for binarizing the input mask. Defaults to 0.999f.
         * @param convert_srgb_to_linear (optional) Convert sRGB to linear for correct scaling of sRGB textures?
         * @param is_normal_map  (optional) Perform processing for normal maps.
         *                       Will re-normalize vectors to unit length at the moment, Slerp is planned.
         * @param channel_mask   (optional) Bit mask of channels to process, 0 = all channels.
         *                       You can use channel_mask_from_array() to generate a mask from an array of booleans.
         * @param scale_alpha_unweighted (optional) Scale the last channel without coverage weighting?
         *                       i.e. regular box filtering
         * @param start_row      (optional) Start row for partial processing
         */
        EXPORT_SYMBOL void convert_and_scale_down_weighted(
            const uint16_t output_width,
            const uint16_t output_height_or_end_row,
            const uint8_t channel_stride,
            const void* input_image,
            const LMF_DATA_TYPE input_data_type,
            const void* input_mask,
            const LMF_DATA_TYPE input_mask_data_type,
            float* output_image,
            uint8_t* output_mask,
            const float coverage_threshold = 0.999f,
            const bool convert_srgb_to_linear = false,
            const bool is_normal_map = false,
            const uint8_t channel_mask = 0,
            const bool scale_alpha_unweighted = false,
            const uint16_t start_row = 0
        );

        /**
         * Pre-process and scale down input image (threaded version)
         *
         * Converts the input image to float, and the input mask to binary (stored as uint8_t). If no input mask is
         * provided (nullptr), the last color channel of the input image is used as the mask (e.g. "A" for an RGBA image).
         *
         * Image and mask are scaled down to half their size, using the mask-weighted average for the image, and a threshold
         * for the mask (i.e. if any of the 4 mask pixels to be scaled is 1, the scaled pixel becomes 1).
         *
         * The input mask is initially processed using the coverage_threshold parameter, which defaults to 0.999f (anything
         * above 0.999f becomes 1, all other values become 0).
         *
         * The last image channel can optionally be scaled unweighted (regular box filtering). This can be useful when e.g.
         * the alpha channel should reflect the average opacity, as opposed to becoming 1.0f through weighted scaling in
         * cases where the alpha channel is identical to the coverage mask.
         * 
         * @param output_width   Output image width in pixels (must be power of 2) 
         * @param output_height  Output image height in pixels (must be power of 2)
         * @param channel_stride Number of total channels in image data
         * @param input_image    Pointer to input image, void*
         * @param input_data_type Input image type as LMF_DATA_TYPE enum value
         * @param input_mask     (optional) Pointer to input mask, void* or nullptr 
         * @param input_mask_data_type Input mask data type as LMF_DATA_TYPE enum value,
         *                       ignored when input mask pointer is nullptr
         * @param output_image   Pointer to pre-allocated output image, float*
         * @param output_mask    Pointer to pre-allocated output mask, uint8_t*
         * @param coverage_threshold (optional) Threshold to use for binarizing the input mask. Defaults to 0.999f.
         * @param convert_srgb_to_linear (optional) Convert sRGB to linear for correct scaling of sRGB textures?
         * @param is_normal_map  (optional) Perform processing for normal maps.
         *                       Will re-normalize vectors to unit length at the moment, Slerp is planned.
         * @param channel_mask   (optional) Bit mask of channels to process, 0 = all channels.
         *                       You can use channel_mask_from_array() to generate a mask from an array of booleans.
         * @param scale_alpha_unweighted (optional) Scale the last channel without coverage weighting?
         *                       i.e. regular box filtering
         * @param max_threads    (optional) Number of threads to use. 0 = auto (half of available threads,
         *                       which amounts to number of hardware cores for machines with SMT/HyperThreading)
         */
        EXPORT_SYMBOL void convert_and_scale_down_weighted_threaded(
            const uint16_t output_width,
            const uint16_t output_height,
            const uint8_t channel_stride,
            const void* input_image,
            const LMF_DATA_TYPE input_data_type,
            const void* input_mask,
            const LMF_DATA_TYPE input_mask_data_type,
            float* output_image,
            uint8_t* output_mask,
            const float coverage_threshold = 0.999f,
            const bool convert_srgb_to_linear = false,
            const bool is_normal_map = false,
            const uint8_t channel_mask = 0,
            const bool scale_alpha_unweighted = false,
            const uint8_t max_threads = 0
        );

        /**
         * Scale down mip level
         *
         * Input mip level and mask are scaled down to half their size, using the mask-weighted average for the mip, and
         * a threshold for the mask (i.e. if any of the 4 mask pixels to be scaled is 1, the scaled pixel becomes 1).
         *
         * The last image channel can optionally be scaled unweighted (regular box filtering). This can be useful when e.g.
         * the alpha channel should reflect the average opacity, as opposed to becoming 1.0f through weighted scaling in
         * cases where the alpha channel is identical to the coverage mask. 
         * 
         * Use this function in a loop after convert_and_scale_down_weighted() to generate the remaining mip levels.
         * 
         * @param output_width   Output mip level width in pixels (must be power of 2) 
         * @param output_height_or_end_row Output mip level height in pixels (must be power of 2), or end row for partial processing
         * @param channel_stride Number of total channels in image data
         * @param input_image    Pointer to input mip level, float*
         * @param input_mask     Pointer to input mip mask, uint8_t*
         * @param output_image   Pointer to pre-allocated output mip level, float*
         * @param output_mask    Pointer to pre-allocated output mip mask, uint8_t*
         * @param is_normal_map  (optional) Perform processing for normal maps.
         *                       Will re-normalize vectors to unit length at the moment, Slerp is planned.
         * @param channel_mask   (optional) Bit mask of channels to process, 0 = all channels.
         *                       You can use channel_mask_from_array() to generate a mask from an array of booleans.
         * @param scale_alpha_unweighted (optional) Scale the last channel without coverage weighting?
         *                       i.e. regular box filtering
         * @param start_row      (optional) Start row for partial processing
         */
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

        /**
         * Scale down mip level (threaded version)
         *
         * Input mip level and mask are scaled down to half their size, using the mask-weighted average for the mip, and
         * a threshold for the mask (i.e. if any of the 4 mask pixels to be scaled is 1, the scaled pixel becomes 1).
         *
         * The last image channel can optionally be scaled unweighted (regular box filtering). This can be useful when e.g.
         * the alpha channel should reflect the average opacity, as opposed to becoming 1.0f through weighted scaling in
         * cases where the alpha channel is identical to the coverage mask. 
         * 
         * Use this function in a loop after convert_and_scale_down_weighted() to generate the remaining mip levels.
         * 
         * @param output_width   Output mip level width in pixels (must be power of 2) 
         * @param output_height  Output mip level height in pixels (must be power of 2)
         * @param channel_stride Number of total channels in image data
         * @param input_image    Pointer to input mip level, float*
         * @param input_mask     Pointer to input mip mask, uint8_t*
         * @param output_image   Pointer to pre-allocated output mip level, float*
         * @param output_mask    Pointer to pre-allocated output mip mask, uint8_t*
         * @param is_normal_map  (optional) Perform processing for normal maps.
         *                       Will re-normalize vectors to unit length at the moment, Slerp is planned.
         * @param channel_mask   (optional) Bit mask of channels to process, 0 = all channels.
         *                       You can use channel_mask_from_array() to generate a mask from an array of booleans.
         * @param scale_alpha_unweighted (optional) Scale the last channel without coverage weighting? i.e. regular box filtering
         * @param max_threads    (optional) Number of threads to use. 0 = auto (half of available threads,
         *                       which amounts to number of hardware cores for machines with SMT/HyperThreading)
         */
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

        /**
         * Composite mip levels (small to large)
         *
         * The smaller ("input") mip is scaled up using nearest neighbor filtering and composited into the larger mip ("output").
         * This fills/"floods" areas outside the coverage mask.
         *
         * Use this in a loop, smallest to largest mips, after generating the mip levels using scale_down_weighted().
         * 
         * @param input_width    Width in pixels of the smaller mip (must be power of 2).
         * @param input_height_or_end_row Height in pixels of the smaller mip (must be power of 2), or end row for partial processing.
         * @param channel_stride Number of total channels in image data
         * @param input_image    Pointer to smaller mip, float* 
         * @param output_image   Pointer to larger mip (output), float*
         * @param output_mask    Pointer to larger mip mask, uint8_t*
         * @param channel_mask   (optional) Bit mask of channels to process, 0 = all channels.
         *                       You can use channel_mask_from_array() to generate a mask from an array of booleans.
         * @param start_row      (optional) Start row for partial processing
         */
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

        /**
         * Composite mip levels (small to large) (threaded version)
         *
         * The smaller ("input") mip is scaled up using nearest neighbor filtering and composited into the larger mip ("output").
         * This fills/"floods" areas outside the coverage mask.
         *
         * Use this in a loop, smallest to largest mips, after generating the mip levels using scale_down_weighted().
         * 
         * @param input_width    Width in pixels of the smaller mip (must be power of 2).
         * @param input_height   Height in pixels of the smaller mip (must be power of 2)
         * @param channel_stride Number of total channels in image data
         * @param input_image    Pointer to smaller mip, float* 
         * @param output_image   Pointer to larger mip (output), float*
         * @param output_mask    Pointer to larger mip mask, uint8_t*
         * @param channel_mask   (optional) Bit mask of channels to process, 0 = all channels.
         *                       You can use channel_mask_from_array() to generate a mask from an array of booleans.
         * @param max_threads    (optional) Number of threads to use. 0 = auto (half of available threads,
         *                       which amounts to number of hardware cores for machines with SMT/HyperThreading)
         */
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

        /**
         * Composite largest mip with original image
         *
         * The largest ("input") mip is scaled up using nearest neighbor filtering, converted into the output format, and
         * composited into the original image.
         *  
         * This fills/"floods" areas outside the coverage mask.
         *
         * If no mask is provided (nullptr), the last color channel of the output image is used as the mask
         * (e.g. "A" for an RGBA image). The mask is initially processed using the coverage_threshold parameter,
         * which defaults to 0.999f (anything above 0.999f becomes 1, all other values become 0).
         *
         * Use this after compositing the mip levels using composite_up().
         *
         * Set a custom channel mask if you want to preserve any of the original image's channels,
         * e.g. the alpha channel in an RGBA image.
         *
         * @param input_width    Width in pixels of the largest mip (must be power of 2, and half the width of the original image)
         * @param input_height_or_end_row Height in pixels of the largest mip (must be power of 2, and half the height of
         *                       the original image), or end row for partial processing.
         * @param channel_stride Number of total channels in image data
         * @param input_image    Pointer to largest mip, float* 
         * @param output_image   Pointer to output image (expected to contain original image), void*
         * @param output_data_type Original image type as LMF_DATA_TYPE enum value
         * @param mask           (optional) Pointer to mask, void* or nullptr
         * @param mask_data_type Mask data type as LMF_DATA_TYPE enum value,
         *                       ignored when mask pointer is nullptr
         * @param coverage_threshold (optional) Threshold to use for binarizing the mask. Defaults to 0.999f.
         * @param convert_linear_to_srgb (optional) Convert linear to sRGB
         * @param channel_mask   (optional) Bit mask of channels to process, 0 = all channels.
         *                       You can use channel_mask_from_array() to generate a mask from an array of booleans.
         * @param start_row      (optional) Start row for partial processing
         */
        EXPORT_SYMBOL void final_composite_and_convert(
            const uint16_t input_width,
            const uint16_t input_height_or_end_row,
            const uint8_t channel_stride,
            const float* input_image,
            void* output_image,
            const LMF_DATA_TYPE output_data_type,
            const void* mask,
            const LMF_DATA_TYPE mask_data_type,
            const float coverage_threshold = 0.999f,
            const bool convert_linear_to_srgb = false,
            const uint8_t channel_mask = 0,
            const uint16_t start_row = 0
        );

        /**
         * Composite largest mip with original image (threaded version)
         *
         * The largest ("input") mip is scaled up using nearest neighbor filtering, converted into the output format, and
         * composited into the original image.
         *  
         * This fills/"floods" areas outside the coverage mask.
         *
         * If no mask is provided (nullptr), the last color channel of the output image is used as the mask
         * (e.g. "A" for an RGBA image). The mask is initially processed using the coverage_threshold parameter,
         * which defaults to 0.999f (anything above 0.999f becomes 1, all other values become 0).
         *
         * Use this after compositing the mip levels using composite_up().
         *
         * Set a custom channel mask if you want to preserve any of the original image's channels,
         * e.g. the alpha channel in an RGBA image.
         *
         * @param input_width    Width in pixels of the largest mip (must be power of 2, and half the width of the original image)
         * @param input_height   Height in pixels of the largest mip (must be power of 2, and half the height of
         *                       the original image)
         * @param channel_stride Number of total channels in image data
         * @param input_image    Pointer to largest mip, float* 
         * @param output_image   Pointer to output image (expected to contain original image), void*
         * @param output_data_type Original image type as LMF_DATA_TYPE enum value
         * @param mask           (optional) Pointer to mask, void* or nullptr
         * @param mask_data_type Mask data type as LMF_DATA_TYPE enum value,
         *                       ignored when mask pointer is nullptr
         * @param coverage_threshold (optional) Threshold to use for binarizing the mask. Defaults to 0.999f.
         * @param convert_linear_to_srgb (optional) Convert linear to sRGB
         * @param channel_mask   (optional) Bit mask of channels to process, 0 = all channels.
         *                       You can use channel_mask_from_array() to generate a mask from an array of booleans.
         * @param max_threads    (optional) Number of threads to use. 0 = auto (half of available threads,
         *                       which amounts to number of hardware cores for machines with SMT/HyperThreading)
         */
        EXPORT_SYMBOL void final_composite_and_convert_threaded(
            const uint16_t input_width,
            const uint16_t input_height,
            const uint8_t channel_stride,
            const float* input_image,
            void* output_image,
            const LMF_DATA_TYPE output_data_type,
            const void* mask,
            const LMF_DATA_TYPE mask_data_type,
            const float coverage_threshold = 0.999f,
            const bool convert_linear_to_srgb = false,
            const uint8_t channel_mask = 0,
            const uint8_t max_threads = 0
        );

        /// @}
        #pragma endregion subroutines
        
        
        /*******************************************
        * CORE FUNCTIONS
        *******************************************/
        #pragma region core_functions
        /// @defgroup core_functions_c Core functions (C API)
        /// @{

        /**
         * Generate coverage-weighted mip maps
         *
         * Generates coverage-weighted mip maps and outputs them as a list of linear float (0..1) arrays
         * (excluding input image / mip 0)
         *
         * The mips and masks memory allocated here needs to be freed after use (e.g. after composite_mips() and saving),
         * see free_mips_memory().
         *
         * @param image_in_out    Input image as void* pointer
         * @param image_data_type Input image type as LMF_DATA_TYPE enum value
         * @param image_width     Input image width in pixels (must be power of 2)
         * @param image_height    Input image height pixels (must be power of 2)
         * @param channel_stride  Number of total channels in image data
         * @param image_mask      (optional) Coverage mask of type uint8_t/uint16_t/float.
         *                        Pass nullptr to use last channel of input image instead.
         * @param mips_output     Array of pointers (float*) that is filled with pointers to the generated mip maps
         * @param masks_output    Array of pointers (uint8_t*) that is filled with pointers to the generated coverage masks
         * @param coverage_threshold (optional) Threshold to use for binarizing the input mask. Defaults to 0.999f.
         * @param convert_srgb    (optional) Convert sRGB to linear before scaling? Mips output will be linear.
         *                        Necessary when scaling sRGB images, to prevent luminosity shifts.
         *                        Use convert_to_type() to convert the mips back to sRGB if needed.
         * @param is_normal_map   (optional) Perform processing for normal maps.
         *                        Will re-normalize vectors to unit length at the moment, Slerp is planned.
         * @param channel_mask    (optional) Binary mask of channels to process, 0 = all channels.
         *                        You can use channel_mask_from_array() to generate a mask from an array of booleans.
         * @param scale_alpha_unweighted (optional) Scale the last channel without coverage weighting?
         *                        i.e. regular box filtering
         * @param max_threads     (optional) Number of threads to use. 0 = auto (half of available threads,
         *                        which amounts to number of hardware cores for machines with SMT/HyperThreading)
         * @return LMF_STATUS enumerator, SUCCESS / 1 on success
         */
        EXPORT_SYMBOL LMF_STATUS generate_mips(
            void* image_in_out,
            const LMF_DATA_TYPE image_data_type,
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
         * @param masks_input    Array of pointers to the mip coverage masks (uint8_t, treated as binary 0/1).
         *                       Used for compositing. 
         * @param image_width    Width of original image (i.e. double the width of the largest mip map)
         * @param image_height   Height of original image (i.e. double the height of the largest mip map)
         * @param channel_stride Number of total channels in image data
         * @param channel_mask   (optional) Binary mask of channels to process, 0 = all channels.
         *                       You can use channel_mask_from_array() to generate a mask from an array of booleans.
         * @param max_threads    (optional) Number of threads to use. 0 = auto (half of available threads,
         *                       which amounts to number of hardware cores for machines with SMT/HyperThreading)
         * @return LMF_STATUS enumerator, SUCCESS / 1 on success
         */
        EXPORT_SYMBOL LMF_STATUS composite_mips(
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
         * @param image_data_type Input image type as LMF_DATA_TYPE enum value
         * @param image_width     Input image width in pixels (must be power of 2)
         * @param image_height    Input image height pixels (must be power of 2)
         * @param channel_stride  Number of total channels in image data
         * @param image_mask      (optional) Coverage mask of type uint8_t/uint16_t/float.
         *                        Pass nullptr to use last channel of input image instead.
         * @param mask_data_type  Mask data type as LMF_DATA_TYPE enum value
         * @param coverage_threshold (optional) Threshold to use for binarizing the input mask. Defaults to 0.999f.
         * @param convert_srgb    (optional) Convert sRGB to linear and back for correct scaling of sRGB textures?
         * @param is_normal_map   (optional) Perform processing for normal maps. Will re-normalize vectors to unit length
         *                        at the moment, Slerp is planned.
         * @param channel_mask    (optional) Binary mask of channels to process, 0 = all channels.
         *                        You can use channel_mask_from_array() to generate a mask from an array of booleans.
         * @param scale_alpha_unweighted (optional) Scale the last channel without coverage weighting?
         *                        i.e. regular box filtering
         * @param max_threads     (optional) Number of threads to use. 0 = auto (half of available threads,
         *                        which amounts to number of hardware cores for machines with SMT/HyperThreading)
         * @return LMF_STATUS enumerator, SUCCESS / 1 on success
         */
        EXPORT_SYMBOL LMF_STATUS flood_image(
            void* image_in_out,
            const LMF_DATA_TYPE image_data_type,
            const uint16_t image_width,
            const uint16_t image_height,
            const uint8_t channel_stride,
            const void* image_mask = nullptr,
            const LMF_DATA_TYPE mask_data_type = LMF_DATA_TYPE::UINT8,
            const float coverage_threshold = 0.99f,
            const bool convert_srgb = false,
            const bool is_normal_map = false,
            const uint8_t channel_mask = 0,
            const bool scale_alpha_unweighted = false,
            const uint8_t max_threads = 0
        );

        /// @}
        #pragma endregion core_functions

#ifdef __cplusplus
    } // extern "C"
} // namespace
#endif
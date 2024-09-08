#include <vector>
#include <cstdint>
#include <thread>
#ifdef _DEBUG
#include <iostream>
#endif

#include "libmipflooding.h"
#include "helpers/macros.h"


/*******************************************
 * CORE FUNCTIONS
 *******************************************/
#pragma region core_functions

#define FUNC(IMAGE_T, MASK_T) \
bool libmipflooding::generate_mips( \
        IMAGE_T* image_in_out, \
        const uint_fast16_t image_width, \
        const uint_fast16_t image_height, \
        const uint8_t channel_stride, \
        const MASK_T* image_mask, \
        float** mips_output, \
        uint8_t** masks_output, \
        const float coverage_threshold, \
        const bool convert_srgb, \
        const bool is_normal_map, \
        const uint8_t channel_mask, \
        const bool scale_alpha_unweighted, \
        const uint_fast8_t max_threads)

template <typename ImageT, typename MaskT>
FUNC(ImageT, MaskT)
{

    #ifdef _DEBUG
    std::cout << "Generating mips, image type " << typeid(ImageT).name() << ", mask type " << typeid(MaskT).name() << ", size " << std::to_string(image_width) << "x" << std::to_string(image_height) << "\n";
    #endif

    const uint8_t mip_count = get_mip_count(image_width, image_height);

    uint_fast16_t mip_width = image_width / 2;
    uint_fast16_t mip_height = image_height / 2;

    /** Initial type conversion & scale down */
    float* output_mip = new float[static_cast<uint32_t>(mip_width * mip_height * channel_stride)];
    uint8_t* output_mask = new uint8_t[static_cast<uint32_t>(mip_width * mip_height)];
    if (max_threads != 1)
        convert_and_scale_down_weighted_threaded(mip_width, mip_height, channel_stride, image_in_out, image_mask, output_mip, output_mask, coverage_threshold, convert_srgb, is_normal_map, channel_mask, scale_alpha_unweighted, max_threads);
    else
        convert_and_scale_down_weighted(mip_width, mip_height, channel_stride, image_in_out, image_mask, output_mip, output_mask, coverage_threshold, convert_srgb, is_normal_map, channel_mask, scale_alpha_unweighted);

    #ifdef _DEBUG
    std::cout << "Finished initial conversion & scale\n";
    #endif
    
    mips_output[0] = output_mip;
    masks_output[0] = output_mask;

    /** Keep scaling down to 1x1 */
    for (int i = 1; i < mip_count; ++i)
    {
        mip_width /= 2;
        mip_height /= 2;

        output_mip = new float[static_cast<uint32_t>(mip_width * mip_height * channel_stride)];
        output_mask = new uint8_t[static_cast<uint32_t>(mip_width * mip_height)];
        if (max_threads != 1)
            scale_down_weighted_threaded(mip_width, mip_height, channel_stride, mips_output[i-1], masks_output[i-1], output_mip, output_mask, is_normal_map, channel_mask, scale_alpha_unweighted, max_threads);
        else
            scale_down_weighted(mip_width, mip_height, channel_stride, mips_output[i-1], masks_output[i-1], output_mip, output_mask, is_normal_map, channel_mask, scale_alpha_unweighted);

        mips_output[i] = output_mip;
        masks_output[i] = output_mask;

        #ifdef _DEBUG
        std::cout << "Generated mip with resolution " << std::to_string(mip_width) << "x" << std::to_string(mip_height) << "\n";
        #endif
    }
    
    return true;
}
INSTANTIATE_TYPES_2(FUNC)
#undef FUNC


bool libmipflooding::composite_mips(
        float** mips_in_out,
        const uint8_t** masks_input,
        const uint_fast16_t image_width,
        const uint_fast16_t image_height,
        const uint_fast8_t channel_stride,
        const uint8_t channel_mask,
        const uint_fast8_t max_threads)
{
    const uint8_t mip_count = get_mip_count(image_width, image_height);

    uint_fast16_t mip_width = image_width > image_height ? image_width / image_height : 1;
    uint_fast16_t mip_height = image_width > image_height ? 1 : image_width / image_height;
    
    /** Composite back up */
    for (int i = mip_count - 2; i >= 0; --i)
    {
        if (max_threads != 1)
            composite_up_threaded(mip_width, mip_height, channel_stride, mips_in_out[i+1], mips_in_out[i], masks_input[i], channel_mask, max_threads);
        else
            composite_up(mip_width, mip_height, channel_stride, mips_in_out[i+1], mips_in_out[i], masks_input[i], channel_mask);
        
        mip_width *= 2;
        mip_height *= 2;

        #ifdef _DEBUG
        std::cout << "Composited mips up to resolution " << std::to_string(mip_width) << "x" << std::to_string(mip_height) << "\n";
        #endif
    }

    return true;
}


#define FUNC(IMAGE_T, MASK_T) \
bool libmipflooding::flood_image( \
        IMAGE_T* image_in_out, \
        const uint_fast16_t image_width, \
        const uint_fast16_t image_height, \
        const uint_fast8_t channel_stride, \
        const MASK_T* image_mask, \
        const float coverage_threshold, \
        const bool convert_srgb, \
        const bool is_normal_map, \
        const uint8_t channel_mask, \
        const bool scale_alpha_unweighted, \
        const uint_fast8_t max_threads)

template <typename ImageT, typename MaskT>
    FUNC(ImageT, MaskT)
{
    #ifdef _DEBUG
    std::cout << "Flooding image\n";
    #endif
    
    const uint_fast8_t mip_count = get_mip_count(image_width, image_height);
    // index 0 is the traditional mip level 1
    float** mips_output = new float*[mip_count];
    uint8_t** masks_output = new uint8_t*[mip_count];

    generate_mips(image_in_out, image_width, image_height, channel_stride, image_mask, mips_output, masks_output, coverage_threshold, convert_srgb, is_normal_map, channel_mask, scale_alpha_unweighted, max_threads);

    #ifdef _DEBUG
    std::cout << "Done generating mips\n";
    #endif
    
    composite_mips(mips_output, const_cast<const uint8_t**>(masks_output), image_width, image_height, channel_stride, channel_mask, max_threads);

    #ifdef _DEBUG
    std::cout << "Done compositing mips\n";
    #endif
        
    /** Final composite */
    if (max_threads != 1)
        final_composite_and_convert_threaded(image_width/2, image_width/2, channel_stride, mips_output[0], image_in_out, image_mask, coverage_threshold, convert_srgb, channel_mask, max_threads);
    else
        final_composite_and_convert(image_width/2, image_width/2, channel_stride, mips_output[0], image_in_out, image_mask, coverage_threshold, convert_srgb, channel_mask);

    #ifdef _DEBUG
    std::cout << "Done compositing mips and image\n";
    #endif
    
    free_mips_memory(mip_count, mips_output, masks_output);
    
    #ifdef _DEBUG
        std::cout << "Released mips and masks memory\n";
    #endif

    return false;
}
INSTANTIATE_TYPES_2(FUNC)
#undef FUNC


#pragma endregion core_functions

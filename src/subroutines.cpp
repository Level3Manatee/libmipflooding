#include <limits>
#include <type_traits>
#include <cmath>
#include <vector>
#include <thread>

#include "libmipflooding.h"
#include "helpers/channel_set.h"
#include "helpers/macros.h"
#include "helpers/helper_functions.h"


template <typename Function, typename... Args>
void run_threaded(
        Function&& function,
        const uint_fast8_t max_threads, 
        const uint_fast16_t row_width,
        const uint_fast16_t row_count,
        const uint_fast8_t channel_stride,
        Args&&... args)
{
    // Limit number of threads when image size is very small; assumes alignment to 4KB in memory 
    const uint_fast32_t max_threads_based_on_data = (row_width * row_count * channel_stride * static_cast<uint_fast32_t>(sizeof(float))) / 4096;
    // Assumes SMT/HyperThreading is active; we don't want that -> divide by 2; manually set max_threads to override
    const uint_fast16_t hardware_threads = max_threads == 0 ? std::thread::hardware_concurrency() / 2 : max_threads;

    const uint_fast16_t num_threads = std::max(std::min(max_threads_based_on_data, hardware_threads),
                                               static_cast<uint_fast16_t>(1));
    
    std::vector<std::thread> threads;

    const uint_fast16_t elements_per_thread = row_count / num_threads;
    const uint_fast16_t remaining_elements = row_count % num_threads;
    
    for (unsigned int thread_id = 0; thread_id < num_threads; ++thread_id) {
        uint_fast16_t start_idx = thread_id * elements_per_thread;
        uint_fast16_t end_idx = (thread_id == num_threads - 1)
                                 ? (start_idx + elements_per_thread + remaining_elements)
                                 : (start_idx + elements_per_thread);
        threads.emplace_back(std::forward<Function>(function), row_width, end_idx, channel_stride, std::forward<Args>(args)..., start_idx);
    }
    
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}


#define FUNC(OUTPUT_T) \
void libmipflooding::convert_to_type( \
        const uint_fast16_t width, \
        const uint_fast16_t height_or_end_row, \
        const uint_fast8_t channel_stride, \
        const float* image_in, \
        OUTPUT_T* image_out, \
        const bool convert_srgb, \
        const uint8_t channel_mask, \
        const uint_fast16_t start_row)

template <typename OutputT>
FUNC(OutputT)
{
    const channel_set channels = channel_set(channel_mask, channel_stride);
    const float image_type_factor = std::is_floating_point<OutputT>::value ? 1.0f : static_cast<float>(std::numeric_limits<OutputT>::max());
    #ifdef _DEBUG
    const uint_fast16_t output_width = width * 2;
    std::printf("Convert to type, start row %hu, end row %hu, output width %hu, channel stride %hhu, convert to sRGB?: %s, channel_mask %s\n", start_row, height_or_end_row, output_width, channel_stride, (convert_srgb ? "yes" : "no"), channels.to_string().c_str());
    #endif    
    for (uint_fast16_t y = start_row; y < height_or_end_row; ++y)
    {
        for (uint_fast16_t x = 0; x < width; ++x)
        {
            const uint32_t idx = (y * width + x) * channel_stride;
            for (const uint8_t c : channels)
            {
                if (convert_srgb)
                    image_out[idx + c] = static_cast<OutputT>(to_sRGB(image_in[idx + c]) * image_type_factor + (std::is_floating_point<OutputT>::value ? 0 : 0.5f));
                else
                    image_out[idx + c] = static_cast<OutputT>(image_in[idx + c] * image_type_factor + (std::is_floating_point<OutputT>::value ? 0 : 0.5f));
            }
        }
    }
}
INSTANTIATE_TYPES_1(FUNC)
#undef FUNC


#define FUNC(OUTPUT_T) \
void libmipflooding::convert_to_type_threaded( \
        const uint_fast16_t width, \
        const uint_fast16_t height_or_end_row, \
        const uint_fast8_t channel_stride, \
        const float* image_in, \
        OUTPUT_T* image_out, \
        const bool convert_srgb, \
        const uint8_t channel_mask, \
        const uint8_t max_threads)

template <typename OutputT>
FUNC(OutputT)
{
    run_threaded(convert_to_type<OutputT>, max_threads, width, height_or_end_row, channel_stride, image_in, image_out, convert_srgb, channel_mask);
}
INSTANTIATE_TYPES_1(FUNC)
#undef FUNC


/**
 * Re-normalization to unit vector for normal maps
 */
#define NORMALIZE_CHANNELS \
    float sum = 0.0f; \
    for (const uint_fast8_t c : channels) \
    { \
        const float component = output_image[output_idx * channel_stride + c] * 2.0f - 1.0f; \
        sum += component * component; \
    } \
    if (sum < 0.0001f) \
        continue; \
    const float scale = std::sqrt(sum); \
    for (const uint_fast8_t c : channels) \
    { \
        const float component = output_image[output_idx * channel_stride + c] * 2.0f - 1.0f; \
        output_image[output_idx * channel_stride + c] = (component / scale + 1.0f) / 2.0f; \
    }




/*******************************************
 * SUBROUTINES
 *******************************************/
#pragma region subroutines


#define FUNC(INPUT_T, INPUT_MASK_T) \
void libmipflooding::convert_and_scale_down_weighted( \
        const uint_fast16_t output_width, \
        const uint_fast16_t output_height_or_end_row, \
        const uint_fast8_t channel_stride, \
        const INPUT_T* input_image, \
        const INPUT_MASK_T* input_mask, \
        float* output_image, \
        uint8_t* output_mask, \
        const float coverage_threshold, \
        const bool convert_srgb_to_linear, \
        const bool is_normal_map, \
        const uint8_t channel_mask, \
        const bool scale_alpha_unweighted, \
        const uint_fast16_t start_row)

template <typename InputT, typename InputMaskT>
FUNC(InputT, InputMaskT)
{
    const bool last_channel_is_mask = (input_mask == nullptr);
    const channel_set channels = channel_set(channel_mask, channel_stride);
    const uint_fast16_t input_width = output_width * 2;
    const float image_type_factor = std::is_floating_point<InputT>::value ? 1.0f : static_cast<float>(std::numeric_limits<InputT>::max());
    const float mask_type_factor = std::is_floating_point<InputMaskT>::value ? 1.0f : static_cast<float>(std::numeric_limits<InputMaskT>::max());
    #ifdef _DEBUG
    uint32_t covered_pixels = 0;
    #endif    
    for (uint_fast32_t y = start_row; y < output_height_or_end_row; ++y)
    {
        for (uint_fast32_t x = 0; x < output_width; ++x)
        {
            uint_fast32_t idx0 =  2 * y      * input_width +  2 * x;
            uint_fast32_t idx1 =  2 * y      * input_width + (2 * x + 1);
            uint_fast32_t idx2 = (2 * y + 1) * input_width +  2 * x;
            uint_fast32_t idx3 = (2 * y + 1) * input_width + (2 * x + 1);

            const uint_fast32_t output_idx = y * output_width + x;

            float mask_input_0 = 0, mask_input_1 = 0, mask_input_2 = 0, mask_input_3 = 0;
            if (!last_channel_is_mask)
            {
                mask_input_0 = input_mask[idx0] / mask_type_factor;
                mask_input_1 = input_mask[idx1] / mask_type_factor;
                mask_input_2 = input_mask[idx2] / mask_type_factor;
                mask_input_3 = input_mask[idx3] / mask_type_factor;
            }
            // all input accesses after this point are strided
            idx0 *= channel_stride;
            idx1 *= channel_stride;
            idx2 *= channel_stride;
            idx3 *= channel_stride;
            // extract mask from last channel, usually alpha (ignoring miniscule sRGB difference here)
            if (last_channel_is_mask)
            {
                mask_input_0 = input_image[idx0 + channel_stride - 1] / image_type_factor;
                mask_input_1 = input_image[idx1 + channel_stride - 1] / image_type_factor;
                mask_input_2 = input_image[idx2 + channel_stride - 1] / image_type_factor;
                mask_input_3 = input_image[idx3 + channel_stride - 1] / image_type_factor;
            }
            const uint_fast8_t mask0 = mask_input_0 > coverage_threshold ? 1 : 0;
            const uint_fast8_t mask1 = mask_input_1 > coverage_threshold ? 1 : 0;
            const uint_fast8_t mask2 = mask_input_2 > coverage_threshold ? 1 : 0;
            const uint_fast8_t mask3 = mask_input_3 > coverage_threshold ? 1 : 0;
            const uint_fast8_t mask_sum = mask0 + mask1 + mask2 + mask3;

            // loop through all channels (not just masked subset) to at least initialize with 0 (for mip export)
            for (uint_fast8_t c = 0; c < channel_stride; ++c)
            {
                // if nothing else, initialize with 0 
                if (mask_sum == 0 || !channels.has(c))
                {
                    output_image[output_idx * channel_stride + c] = 0.0f;
                    continue;
                }
                if (c == channel_stride - 1 && scale_alpha_unweighted)
                {
                    output_image[output_idx * channel_stride + c] = (mask_input_0 + mask_input_1 + mask_input_2 + mask_input_3) / 4.0f;
                    continue;
                }
                float color_sum;
                if (convert_srgb_to_linear)
                {
                    const float color0 = (mask0 == 0) ? 0.0f : to_linear(static_cast<float>(input_image[idx0 + c]) / image_type_factor);
                    const float color1 = (mask1 == 0) ? 0.0f : to_linear(static_cast<float>(input_image[idx1 + c]) / image_type_factor);
                    const float color2 = (mask2 == 0) ? 0.0f : to_linear(static_cast<float>(input_image[idx2 + c]) / image_type_factor);
                    const float color3 = (mask3 == 0) ? 0.0f : to_linear(static_cast<float>(input_image[idx3 + c]) / image_type_factor);
                    color_sum = (color0 + color1 + color2 + color3);
                }
                else
                {
                    const float color0 = (mask0 == 0) ? 0.0f : static_cast<float>(input_image[idx0 + c]);
                    const float color1 = (mask1 == 0) ? 0.0f : static_cast<float>(input_image[idx1 + c]);
                    const float color2 = (mask2 == 0) ? 0.0f : static_cast<float>(input_image[idx2 + c]);
                    const float color3 = (mask3 == 0) ? 0.0f : static_cast<float>(input_image[idx3 + c]);
                    color_sum = (color0 + color1 + color2 + color3) / image_type_factor;
                }
                output_image[output_idx * channel_stride + c] = color_sum / static_cast<float>(mask_sum); 
            }
            output_mask[output_idx] = mask_sum > 0;
            if (is_normal_map && mask_sum > 0)
            {
                NORMALIZE_CHANNELS
            }
            #ifdef _DEBUG
            if (mask_sum > 0)
                ++covered_pixels;
            #endif
        }
    }
    #ifdef _DEBUG
    const float coverage_percent = static_cast<float>(covered_pixels) / static_cast<float>((output_height_or_end_row - start_row) * output_width);
    std::printf("Convert and scale down weighted: done, start row %hu, end row %hu, output width %hu, channel stride %hhu, channel_mask %s, image conversion factor %f, mask conversion factor %f, coverage: %f, last channel is mask: %s, scale alpha unweighted: %s\n", start_row, output_height_or_end_row, output_width, channel_stride, channels.to_string().c_str(), image_type_factor, mask_type_factor, coverage_percent, (last_channel_is_mask ? "yes" : "no"), (scale_alpha_unweighted ? "yes" : "no"));
    #endif
}
INSTANTIATE_TYPES_2(FUNC)
#undef FUNC


#define FUNC(INPUT_T, INPUT_MASK_T) \
void libmipflooding::convert_and_scale_down_weighted_threaded( \
        const uint_fast16_t output_width, \
        const uint_fast16_t output_height, \
        const uint_fast8_t channel_stride, \
        const INPUT_T* input_image, \
        const INPUT_MASK_T* input_mask, \
        float* output_image, \
        uint8_t* output_mask, \
        const float coverage_threshold, \
        const bool convert_srgb_to_linear, \
        const bool is_normal_map, \
        const uint8_t channel_mask, \
        const bool scale_alpha_unweighted, \
        const uint_fast8_t max_threads)

template <typename InputT, typename InputMaskT>
FUNC(InputT, InputMaskT)
{
    run_threaded(convert_and_scale_down_weighted<InputT, InputMaskT>, max_threads, 
        output_width, output_height, channel_stride, input_image, input_mask, output_image, output_mask, coverage_threshold, convert_srgb_to_linear, is_normal_map, channel_mask, scale_alpha_unweighted);
}
INSTANTIATE_TYPES_2(FUNC)
#undef FUNC


void libmipflooding::scale_down_weighted(
        const uint_fast16_t output_width,
        const uint_fast16_t output_height_or_end_row,
        const uint_fast8_t channel_stride,
        const float* input_image,
        const uint8_t* input_mask,
        float* output_image,
        uint8_t* output_mask,
        const bool is_normal_map,
        const uint8_t channel_mask,
        const bool scale_alpha_unweighted,
        const uint_fast16_t start_row)
{
    const channel_set channels = channel_set(channel_mask, channel_stride);
    const uint_fast16_t input_width = output_width * 2;
    #ifdef _DEBUG
    std::printf("Scale down weighted, start row %hu, end row %hu, output width %hu, channel stride %hhu, channel_mask %s\n", start_row, output_height_or_end_row, output_width, channel_stride, channels.to_string().c_str());
    #endif    
    for (uint_fast32_t y = start_row; y < output_height_or_end_row; ++y)
    {
        for (uint_fast32_t x = 0; x < output_width; ++x)
        {
            uint_fast32_t idx0 =  2 * y      * input_width +  2 * x;
            uint_fast32_t idx1 =  2 * y      * input_width + (2 * x + 1);
            uint_fast32_t idx2 = (2 * y + 1) * input_width +  2 * x;
            uint_fast32_t idx3 = (2 * y + 1) * input_width + (2 * x + 1);

            const uint32_t output_idx = y * output_width + x;

            const uint8_t mask_sum = input_mask[idx0] + input_mask[idx1] + input_mask[idx2] + input_mask[idx3];

            idx0 *= channel_stride;
            idx1 *= channel_stride;
            idx2 *= channel_stride;
            idx3 *= channel_stride;

            // need to go through all channels to at least initialize with 0 for mip export
            for (uint_fast8_t c = 0; c < channel_stride; ++c)
            {
                // if nothing else, initialize with 0 
                if (mask_sum == 0 || !channels.has(c))
                {
                    output_image[output_idx * channel_stride + c] = 0.0f;
                    continue;
                }
                if (scale_alpha_unweighted && c == channel_stride - 1)
                {
                    output_image[output_idx * channel_stride + c] = (input_image[idx0 + c] +
                                                                     input_image[idx1 + c] +
                                                                     input_image[idx2 + c] +
                                                                     input_image[idx3 + c] ) / 4.0f;
                    continue;
                }
                output_image[output_idx * channel_stride + c] = (input_image[idx0 + c] +
                                                                 input_image[idx1 + c] +
                                                                 input_image[idx2 + c] +
                                                                 input_image[idx3 + c] )
                                                                / static_cast<float>(mask_sum);
            }
            output_mask[output_idx] = mask_sum > 0;
            if (is_normal_map && mask_sum > 0)
            {
                NORMALIZE_CHANNELS
            }
        }
    }
}

void libmipflooding::scale_down_weighted_threaded(
        const uint_fast16_t output_width,
        const uint_fast16_t output_height, 
        const uint_fast8_t channel_stride,
        const float* input_image,
        const uint8_t* input_mask,
        float* output_image,
        uint8_t* output_mask,
        const bool is_normal_map,
        const uint8_t channel_mask,
        const bool scale_alpha_unweighted,
        const uint_fast8_t max_threads)
{
    run_threaded(scale_down_weighted, max_threads, output_width, output_height, channel_stride, input_image, input_mask, output_image, output_mask, is_normal_map, channel_mask, scale_alpha_unweighted);
}

    
void libmipflooding::composite_up(
        const uint_fast16_t input_width,
        const uint_fast16_t input_height_or_end_row,
        const uint_fast8_t channel_stride,
        const float* input_image,
        float* output_image,
        const uint8_t* output_mask,
        const uint8_t channel_mask,
        const uint_fast16_t start_row)
{
    const channel_set channels = channel_set(channel_mask, channel_stride);
    const uint_fast16_t output_width = input_width * 2;
    #ifdef _DEBUG
    std::printf("Composite up, start row %hu, end row %hu, output width %hu, channel stride %hhu, channel_mask %s\n", start_row, input_height_or_end_row, output_width, channel_stride, channels.to_string().c_str());
    #endif    
    for (uint_fast16_t y = start_row; y < input_height_or_end_row; ++y)
    {
        for (uint_fast16_t x = 0; x < input_width; ++x)
        {
            uint_fast32_t t_idx0 =  2 * y      * output_width +  2 * x;
            uint_fast32_t t_idx1 =  2 * y      * output_width + (2 * x + 1);
            uint_fast32_t t_idx2 = (2 * y + 1) * output_width +  2 * x;
            uint_fast32_t t_idx3 = (2 * y + 1) * output_width + (2 * x + 1);

            const uint8_t mask0 = output_mask[t_idx0];
            const uint8_t mask1 = output_mask[t_idx1];
            const uint8_t mask2 = output_mask[t_idx2];
            const uint8_t mask3 = output_mask[t_idx3];

            const uint_fast32_t source_idx = (y * input_width + x) * channel_stride;

            t_idx0 *= channel_stride;
            t_idx1 *= channel_stride;
            t_idx2 *= channel_stride;
            t_idx3 *= channel_stride;
            
            for (const uint_fast8_t c : channels)
            {
                const float source_color = input_image[source_idx + c];

                if (mask0 == 0)
                    output_image[t_idx0 + c] = source_color;
                if (mask1 == 0)
                    output_image[t_idx1 + c] = source_color;
                if (mask2 == 0)
                    output_image[t_idx2 + c] = source_color;
                if (mask3 == 0)
                    output_image[t_idx3 + c] = source_color;
            }
        }
    }
}


void libmipflooding::composite_up_threaded(
        const uint_fast16_t input_width,
        const uint_fast16_t input_height,
        const uint_fast8_t channel_stride,
        const float* input_image,
        float* output_image,
        const uint8_t* output_mask,
        const uint8_t channel_mask,
        const uint_fast8_t max_threads)
{
     run_threaded(composite_up, max_threads, 
        input_width, input_height, channel_stride, input_image, output_image, output_mask, channel_mask);
}

#define FUNC(OUTPUT_T, MASK_T) \
void libmipflooding::final_composite_and_convert( \
        const uint_fast16_t input_width, \
        const uint_fast16_t input_height_or_end_row, \
        const uint_fast8_t channel_stride, \
        const float* input_image, \
        OUTPUT_T* output_image, \
        const MASK_T* mask, \
        const float coverage_threshold, \
        const bool convert_linear_to_srgb, \
        const uint8_t channel_mask, \
        const uint_fast16_t start_row)

template <typename OutputT, typename MaskT>
FUNC(OutputT, MaskT)
{
    const bool infer_mask = (mask == nullptr);
    const channel_set channels = channel_set(channel_mask, channel_stride);
    const uint_fast16_t output_width = input_width * 2;
    const float image_type_factor = std::is_floating_point<OutputT>::value ? 1.0f : static_cast<float>(std::numeric_limits<OutputT>::max());
    const float mask_type_factor = std::is_floating_point<MaskT>::value ? 1.0f : static_cast<float>(std::numeric_limits<MaskT>::max());
    #ifdef _DEBUG
    std::printf("Final composite and convert, start row %hu, end row %hu, width %hu, channel stride %hhu, channel_mask %s, image conversion factor %f, mask conversion factor %f\n", start_row, input_height_or_end_row, output_width, channel_stride, channels.to_string().c_str(), image_type_factor, mask_type_factor);
    #endif    
    for (uint_fast16_t y = start_row; y < input_height_or_end_row; ++y)
    {
        for (uint_fast16_t x = 0; x < input_width; ++x)
        {
            uint_fast32_t out_idx0 =  2 * y      * output_width +  2 * x;
            uint_fast32_t out_idx1 =  2 * y      * output_width + (2 * x + 1);
            uint_fast32_t out_idx2 = (2 * y + 1) * output_width +  2 * x;
            uint_fast32_t out_idx3 = (2 * y + 1) * output_width + (2 * x + 1);

            float mask_input_0 = 0, mask_input_1 = 0, mask_input_2 = 0, mask_input_3 = 0;
            if (!infer_mask)
            {
                mask_input_0 = mask[out_idx0] / mask_type_factor;
                mask_input_1 = mask[out_idx1] / mask_type_factor;
                mask_input_2 = mask[out_idx2] / mask_type_factor;
                mask_input_3 = mask[out_idx3] / mask_type_factor;
            }
            // all input accesses after this point are strided
            out_idx0 *= channel_stride;
            out_idx1 *= channel_stride;
            out_idx2 *= channel_stride;
            out_idx3 *= channel_stride;
            // extract mask from last channel, usually alpha (ignoring miniscule sRGB difference here)
            if (infer_mask)
            {
                mask_input_0 = output_image[out_idx0 + channel_stride - 1] / image_type_factor;                    
                mask_input_1 = output_image[out_idx1 + channel_stride - 1] / image_type_factor;                    
                mask_input_2 = output_image[out_idx2 + channel_stride - 1] / image_type_factor;                    
                mask_input_3 = output_image[out_idx3 + channel_stride - 1] / image_type_factor;                    
            }
            const uint_fast8_t mask0 = mask_input_0 > coverage_threshold ? 1 : 0;
            const uint_fast8_t mask1 = mask_input_1 > coverage_threshold ? 1 : 0;
            const uint_fast8_t mask2 = mask_input_2 > coverage_threshold ? 1 : 0;
            const uint_fast8_t mask3 = mask_input_3 > coverage_threshold ? 1 : 0;

            const uint_fast32_t input_idx = (y * input_width + x) * channel_stride;
            
            for (const uint_fast8_t c : channels)
            {
                OutputT input_color;
                if (convert_linear_to_srgb)
                    input_color = static_cast<OutputT>(to_sRGB(input_image[input_idx + c]) * image_type_factor + (std::is_floating_point<OutputT>::value ? 0 : 0.5f));
                else
                    input_color = static_cast<OutputT>(input_image[input_idx + c] * image_type_factor + (std::is_floating_point<OutputT>::value ? 0 : 0.5f));

                if (mask0 == 0)
                    output_image[out_idx0 + c] = input_color;
                if (mask1 == 0)
                    output_image[out_idx1 + c] = input_color;
                if (mask2 == 0)
                    output_image[out_idx2 + c] = input_color;
                if (mask3 == 0)
                    output_image[out_idx3 + c] = input_color;
            }
        }
    }
}
INSTANTIATE_TYPES_2(FUNC)
#undef FUNC


#define FUNC(OUTPUT_T, MASK_T) \
void libmipflooding::final_composite_and_convert_threaded( \
        const uint_fast16_t input_width, \
        const uint_fast16_t input_height, \
        const uint_fast8_t channel_stride, \
        const float* input_image, \
        OUTPUT_T* output_image, \
        const MASK_T* mask, \
        const float coverage_threshold, \
        const bool convert_linear_to_srgb, \
        const uint8_t channel_mask, \
        const uint_fast8_t max_threads)

template <typename OutputT, typename MaskT>
FUNC(OutputT, MaskT)
{
    run_threaded(final_composite_and_convert<OutputT, MaskT>, max_threads, 
       input_width, input_height, channel_stride, input_image, output_image, mask, coverage_threshold, convert_linear_to_srgb, channel_mask);
}
INSTANTIATE_TYPES_2(FUNC)
#undef FUNC


#pragma endregion subroutines

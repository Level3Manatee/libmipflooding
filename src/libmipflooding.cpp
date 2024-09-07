#include <vector>
#include <cstdint>
#include <thread>
#include <iostream>
#include <sstream>
#include "libmipflooding.h"
#include "libmipflooding_c.h"

//#define _DEBUG

/*******************************************
 * HELPER FUNCTIONS
 *******************************************/
#pragma region helper_functions

inline float to_linear(const float in_sRGB) {
    if (in_sRGB <= 0.04045f)
        return in_sRGB / 12.92f;
    else
        return pow((in_sRGB + 0.055f) / 1.055f, 2.4f);
}


inline float to_sRGB(const float in_linear) {
    if (in_linear <= 0.0031308f)
        return in_linear * 12.92f;
    else
        return 1.055f * pow(in_linear, 1.0f / 2.4f) - 0.055f;
}


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


inline uint8_t libmipflooding::get_mip_count(const uint_fast16_t width, const uint_fast16_t height)
{
    return static_cast<uint8_t>(std::log2(std::min(width, height)));
}

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
    } \


/// Helper class to allow iterating over a subset of channels
class channel_set
{
private:
    uint_fast8_t channel_count_;
    uint_fast8_t channels_[8] = {0,0,0,0,0,0,0,0};
    bool         channel_mask_[8] = {false,false,false,false,false,false,false,false};
public:
    class iterator
    {
    private:
        const channel_set* ptr_;
        uint_fast8_t current_index_;
    public:
        explicit iterator(const channel_set* ptr) : ptr_(ptr), current_index_(0){}
        explicit iterator(const channel_set* ptr, const uint_fast8_t index) : ptr_(ptr), current_index_(index){}
        iterator& operator++() { ++current_index_; return *this; }
        uint_fast8_t operator*() const { return ptr_->channels_[current_index_]; }
        bool operator!=(const iterator & other) const { return current_index_ != other.current_index_; }
    };
    explicit channel_set(const uint8_t channel_mask, const uint_fast8_t channel_stride) : channel_count_(0)
    {
        if (channel_mask == 0)
        {
            channel_count_ = channel_stride;
            for (uint_fast8_t i = 0; i < channel_stride; ++i)
            {
                channel_mask_[i] = true;
                channels_[i] = i;
            }
            return;
        }
        for (uint_fast8_t i = 0; i < channel_stride; ++i)
        {
            if((channel_mask & (1 << i)) != 0)
            {
                channel_mask_[i] = true;
                channels_[channel_count_] = i;
                ++channel_count_;
            }
        }
    }
    explicit channel_set(const bool* array, const uint_fast8_t channel_count) : channel_count_(0)
    {
        for (uint_fast8_t i = 0; i < channel_count; ++i)
        {
            if (array[i])
            {
                channel_mask_[i] = true;
                channels_[channel_count_] = i;
                ++channel_count_;
            }
        }
    }
    explicit channel_set(const uint_fast8_t channel_count) : channel_count_(channel_count)
    {
        for (uint_fast8_t i = 0; i < channel_count; ++i)
        {
            channel_mask_[i] = true;
            channels_[i] = i;
        }
    }
    iterator begin() const { return iterator(this); }
    iterator end() const { return iterator(this, channel_count_); }
    uint8_t has(const uint8_t channel) const { return channel < channel_count_ && channel_mask_[channel]; }
    std::string to_string() const
    {
        std::stringstream str;
        str << "[";
        for (int i = 0; i < channel_count_; ++i)
        {
            str << std::to_string(channels_[i]);
            if (i < channel_count_ - 1)
                str << ',';
        }
        str << "]";
        return str.str();
    }
};


uint8_t libmipflooding::channel_mask_from_array(const bool* array, const uint_fast8_t element_count)
{
    uint8_t mask = 0;
    for (uint_fast8_t i = 0; i < element_count; ++i)
    {
        if (array[i])
            mask |= 1 << i;
    }
    return mask;
}


void libmipflooding::convert_linear_to_srgb(
        const uint_fast16_t width,
        const uint_fast16_t height_or_end_row,
        const uint_fast8_t channel_stride,
        float* image_in_out,
        const uint8_t channel_mask,
        const uint_fast16_t start_row)
{
    const channel_set channels = channel_set(channel_mask, channel_stride);
    const uint_fast16_t output_width = width * 2;
#ifdef _DEBUG
    std::printf("Convert linear to sRGB, start row %i, end row %i, output width %i, channel stride %i, channel_mask %s\n", start_row, height_or_end_row, output_width, channel_stride, channels.to_string().c_str());
#endif    
    for (uint_fast16_t y = start_row; y < height_or_end_row; ++y)
    {
        for (uint_fast16_t x = 0; x < width; ++x)
        {
            const uint32_t idx = (y * width + x) * channel_stride;
            for (const uint8_t c : channels)
            {
                image_in_out[idx + c] = to_sRGB(image_in_out[idx + c]);
            }
        }
    }
}


void libmipflooding::convert_linear_to_srgb_threaded(
        const uint_fast16_t width,
        const uint_fast16_t height_or_end_row,
        const uint_fast8_t channel_stride,
        float* image_in_out,
        const uint8_t channel_mask,
        const uint8_t max_threads)
{
    run_threaded(convert_linear_to_srgb, max_threads, width, height_or_end_row, channel_stride, image_in_out, channel_mask);
}


void libmipflooding::free_mips_memory(const uint_fast8_t mip_count, float** mips_output, uint8_t** masks_output)
{
    for (int i = 0; i < mip_count; ++i)
    {
        delete[] mips_output[i];
        delete[] masks_output[i];
    }
}


#pragma endregion helper_functions


/*******************************************
 * SUBROUTINES
 *******************************************/
#pragma region subroutines

template <typename InputT, typename InputMaskT>
void libmipflooding::convert_and_scale_down_weighted(
        const uint_fast16_t output_width,
        const uint_fast16_t output_height_or_end_row,
        const uint_fast8_t channel_stride,
        const InputT* input_image,
        const InputMaskT* input_mask,
        float* output_image,
        uint8_t* output_mask,
        const float coverage_threshold,
        const bool convert_srgb_to_linear,
        const bool is_normal_map,
        const uint8_t channel_mask,
        const bool scale_alpha_unweighted,
        const uint_fast16_t start_row)
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
    std::printf("Convert and scale down weighted: done, start row %i, end row %i, output width %i, channel stride %i, channel_mask %s, image conversion factor %f, mask conversion factor %f, coverage: %f, last channel is mask: %s, scale alpha unweighted: %s\n", start_row, output_height_or_end_row, output_width, channel_stride, channels.to_string().c_str(), image_type_factor, mask_type_factor, coverage_percent, (last_channel_is_mask ? "yes" : "no"), (scale_alpha_unweighted ? "yes" : "no"));
    #endif
}



template <typename InputT, typename InputMaskT>
void libmipflooding::convert_and_scale_down_weighted_threaded(
        const uint_fast16_t output_width,
        const uint_fast16_t output_height,
        const uint_fast8_t channel_stride,
        const InputT* input_image,
        const InputMaskT* input_mask,
        float* output_image,
        uint8_t* output_mask,
        const float coverage_threshold,
        const bool convert_srgb_to_linear,
        const bool is_normal_map,
        const uint8_t channel_mask,
        const bool scale_alpha_unweighted,
        const uint_fast8_t max_threads)
{
    run_threaded(convert_and_scale_down_weighted<InputT, InputMaskT>, max_threads, 
        output_width, output_height, channel_stride, input_image, input_mask, output_image, output_mask, coverage_threshold, convert_srgb_to_linear, is_normal_map, channel_mask, scale_alpha_unweighted);
}


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
    std::printf("Scale down weighted, start row %i, end row %i, output width %i, channel stride %i, channel_mask %s\n", start_row, output_height_or_end_row, output_width, channel_stride, channels.to_string().c_str());
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
    std::printf("Composite up, start row %i, end row %i, output width %i, channel stride %i, channel_mask %s\n", start_row, input_height_or_end_row, output_width, channel_stride, channels.to_string().c_str());
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


template <typename OutputT, typename MaskT>
void libmipflooding::final_composite_and_convert(
        const uint_fast16_t input_width,
        const uint_fast16_t input_height_or_end_row,
        const uint_fast8_t channel_stride,
        const float* input_image,
        OutputT* output_image,
        const MaskT* mask,
        const float coverage_threshold,
        const bool convert_linear_to_srgb,
        const uint8_t channel_mask,
        const uint_fast16_t start_row)
{
    const bool infer_mask = (mask == nullptr);
    const channel_set channels = channel_set(channel_mask, channel_stride);
    const uint_fast16_t output_width = input_width * 2;
    const float image_type_factor = std::is_floating_point<OutputT>::value ? 1.0f : static_cast<float>(std::numeric_limits<OutputT>::max());
    const float mask_type_factor = std::is_floating_point<MaskT>::value ? 1.0f : static_cast<float>(std::numeric_limits<MaskT>::max());
    #ifdef _DEBUG
    std::printf("Final composite and convert, start row %i, end row %i, width %i, channel stride %i, channel_mask %s, image conversion factor %f, mask conversion factor %f\n", start_row, input_height_or_end_row, output_width, channel_stride, channels.to_string().c_str(), image_type_factor, mask_type_factor);
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


template <typename OutputT, typename MaskT>
void libmipflooding::final_composite_and_convert_threaded(
        const uint_fast16_t input_width,
        const uint_fast16_t input_height,
        const uint_fast8_t channel_stride,
        const float* input_image,
        OutputT* output_image,
        const MaskT* mask,
        const float coverage_threshold,
        const bool convert_linear_to_srgb,
        const uint8_t channel_mask,
        const uint_fast8_t max_threads)
{
    run_threaded(final_composite_and_convert<OutputT, MaskT>, max_threads, 
       input_width, input_height, channel_stride, input_image, output_image, mask, coverage_threshold, convert_linear_to_srgb, channel_mask);
}


#pragma endregion subroutines


/*******************************************
 * CORE FUNCTIONS
 *******************************************/
#pragma region core_functions

template <typename ImageT, typename MaskT>
bool libmipflooding::generate_mips(
        ImageT* image_in_out,
        const uint_fast16_t image_width,
        const uint_fast16_t image_height,
        const uint8_t channel_stride,
        const MaskT* image_mask,
        float** mips_output,
        uint8_t** masks_output,
        const float coverage_threshold,
        const bool convert_srgb,
        const bool is_normal_map,
        const uint8_t channel_mask,
        const bool scale_alpha_unweighted,
        const uint_fast8_t max_threads)
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


template <typename ImageT, typename MaskT>
bool libmipflooding::flood_image(
        ImageT* image_in_out,
        const uint_fast16_t image_width,
        const uint_fast16_t image_height,
        const uint_fast8_t channel_stride,
        const MaskT* image_mask,
        const float coverage_threshold,
        const bool convert_srgb,
        const bool is_normal_map,
        const uint8_t channel_mask,
        const bool scale_alpha_unweighted,
        const uint_fast8_t max_threads)
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

#pragma endregion core_functions


/*******************************************
 * C EXPORTS
 *******************************************/
#pragma region c_exports

/* Macros
 * to handle calling main functions with all permutations of image and mask types
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

extern "C" {
    
    uint8_t libmipflooding_c::channel_mask_from_array(const bool* array, const uint8_t element_count)
    {
        return libmipflooding::channel_mask_from_array(array, element_count);
    }
    
    
    void libmipflooding_c::convert_linear_to_srgb(
            const uint_fast16_t width,
            const uint_fast16_t height_or_end_row,
            const uint_fast8_t channel_stride,
            float* image_in_out,
            const uint8_t channel_mask,
            const uint16_t start_row)
    {
        libmipflooding::convert_linear_to_srgb(width, height_or_end_row, channel_stride, image_in_out, channel_mask, start_row);
    }
    
    
    void libmipflooding_c::convert_linear_to_srgb_threaded(
            const uint_fast16_t width,
            const uint_fast16_t height_or_end_row,
            const uint_fast8_t channel_stride,
            float* image_in_out,
            const uint8_t channel_mask,
            const uint8_t max_threads)
    {
        libmipflooding::convert_linear_to_srgb_threaded(width, height_or_end_row, channel_stride, image_in_out, channel_mask, max_threads);
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
#pragma once
#include <string>

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
        bool operator!=(const iterator& other) const
        { return current_index_ != other.current_index_; }
    };

    explicit channel_set(const uint8_t channel_mask, const uint_fast8_t channel_stride);

    explicit channel_set(const bool* array, const uint_fast8_t channel_count);

    explicit channel_set(const uint_fast8_t channel_count);

    iterator begin() const { return iterator(this); }

    iterator end() const { return iterator(this, channel_count_); }

    uint8_t has(const uint8_t channel) const { return channel < channel_count_ && channel_mask_[channel]; }

#ifdef _DEBUG
    std::string to_string() const;
#endif
};

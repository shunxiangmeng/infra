/************************************************************************
 * Copyright(c) 2024  technology
 * 
 * File        :  RateStatistics.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-07-13 23:26:18
 * Description :  None
 * Note        : 
 ************************************************************************/
#include <algorithm>
#include <limits>
#include <memory>
#include "include/RateStatistics.h"
#include "include/Logger.h"

namespace infra {

RateStatistics::Bucket::Bucket(int64_t timestamp)
    : sum(0), num_samples(0), timestamp(timestamp) {}

RateStatistics::RateStatistics(int64_t window_size_ms, float scale)
    : accumulated_count_(0),
      first_timestamp_(-1),
      num_samples_(0),
      scale_(scale),
      max_window_size_ms_(window_size_ms),
      current_window_size_ms_(max_window_size_ms_) {}

RateStatistics::RateStatistics(const RateStatistics& other)
    : buckets_(other.buckets_),
      accumulated_count_(other.accumulated_count_),
      first_timestamp_(other.first_timestamp_),
      overflow_(other.overflow_),
      num_samples_(other.num_samples_),
      scale_(other.scale_),
      max_window_size_ms_(other.max_window_size_ms_),
      current_window_size_ms_(other.current_window_size_ms_) {}

RateStatistics::RateStatistics(RateStatistics&& other) = default;

RateStatistics::~RateStatistics() {}

void RateStatistics::Reset() {
    accumulated_count_ = 0;
    overflow_ = false;
    num_samples_ = 0;
    first_timestamp_ = -1;
    current_window_size_ms_ = max_window_size_ms_;
    buckets_.clear();
}

void RateStatistics::Update(int64_t count, int64_t now_ms) {
    LOG_CHECK_GE(count, 0);

    EraseOld(now_ms);
    if (first_timestamp_ == -1 || num_samples_ == 0) {
        first_timestamp_ = now_ms;
    }

    if (buckets_.empty() || now_ms != buckets_.back().timestamp) {
        if (!buckets_.empty() && now_ms < buckets_.back().timestamp) {
            warnf("Timestamp %lld is before the last added timestamp in the rate window: %lld, aligning to that.\n", now_ms, buckets_.back().timestamp);
            now_ms = buckets_.back().timestamp;
        }
        buckets_.emplace_back(now_ms);
    }
    Bucket& last_bucket = buckets_.back();
    last_bucket.sum += count;
    ++last_bucket.num_samples;

    if (std::numeric_limits<int64_t>::max() - accumulated_count_ > count) {
        accumulated_count_ += count;
    } else {
        overflow_ = true;
    }
    ++num_samples_;
}

infra::optional<int64_t> RateStatistics::Rate(int64_t now_ms) const {
    const_cast<RateStatistics*>(this)->EraseOld(now_ms);
    infra::optional<int64_t> rate;
    int active_window_size = 0;
    if (first_timestamp_ != -1) {
        if (first_timestamp_ <= now_ms - current_window_size_ms_) {
            // Count window as full even if no data points currently in view, if the
            // data stream started before the window.
            active_window_size = int(current_window_size_ms_);
        } else {
            // Size of a single bucket is 1ms, so even if now_ms == first_timestmap_
            // the window size should be 1.
            active_window_size = int(now_ms - first_timestamp_ + 1);
        }
    }

    // If window is a single bucket or there is only one sample in a data set that
    // has not grown to the full window size, or if the accumulator has
    // overflowed, treat this as rate unavailable.
    if (num_samples_ == 0 || active_window_size <= 1 || (num_samples_ <= 1 && active_window_size < current_window_size_ms_) || overflow_) {
        return rate;
    }

    float scale = static_cast<float>(scale_) / active_window_size;
    float result = accumulated_count_ * scale + 0.5f;                            //bps

    if (result > static_cast<float>(std::numeric_limits<int64_t>::max())) {
        return rate;
    }
    rate = (int64_t)result;
    return rate;
}

void RateStatistics::EraseOld(int64_t now_ms) {
    // New oldest time that is included in data set.
    const int64_t new_oldest_time = now_ms - current_window_size_ms_ + 1;

    // Loop over buckets and remove too old data points.
    while (!buckets_.empty() && buckets_.front().timestamp < new_oldest_time) {
        const Bucket& oldest_bucket = buckets_.front();
        LOG_CHECK_GE(accumulated_count_, oldest_bucket.sum);
        LOG_CHECK_GE(num_samples_, oldest_bucket.num_samples);
        accumulated_count_ -= oldest_bucket.sum;
        num_samples_ -= oldest_bucket.num_samples;
        buckets_.pop_front();
    }
}

bool RateStatistics::SetWindowSize(int64_t window_size_ms, int64_t now_ms) {
    if (window_size_ms <= 0 || window_size_ms > max_window_size_ms_) {
        return false;
    }
    if (first_timestamp_ != -1) {
        // If the window changes (e.g. decreases - removing data point, then
        // increases again) we need to update the first timestamp mark as
        // otherwise it indicates the window coveres a region of zeros, suddenly
        // under-estimating the rate.
        first_timestamp_ = std::max(first_timestamp_, now_ms - window_size_ms + 1);
    }
    current_window_size_ms_ = window_size_ms;
    EraseOld(now_ms);
    return true;
}
}
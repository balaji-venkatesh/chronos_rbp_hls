#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace utils {

// TODO(mcj) define a global LENGTH = 2
static inline float_t logSum(const std::array<float_t,2> logs) {
    constexpr float_t NINF = -std::numeric_limits<float_t>::infinity();
    float_t maxLog = NINF;
    for (float_t log : logs) {
        maxLog = std::max(maxLog, log);
    }
    if (maxLog == NINF) return NINF;

    float_t sumExp = 0.0;
    for (float_t log : logs) {
        sumExp += std::exp(log - maxLog);
    }
    return maxLog + std::log(sumExp);
}


static inline float_t distance(std::array<float_t,2> log1,
                              std::array<float_t,2> log2) {
    float_t ans = 0.0;
    for (uint32_t i = 0; i < 2; i++) {
        ans += std::abs(std::exp(log1[i]) - std::exp(log2[i]));
    }
    return ans;
}


static inline float_t distance_vl(std::array<float_t,2> val1,
                                 std::array<float_t,2> log2) {
    float_t ans = 0.0;
    for (uint32_t i = 0; i < 2; i++) {
        ans += std::abs(val1[i] - std::exp(log2[i]));
    }
    return ans;
}



} // namespace utils

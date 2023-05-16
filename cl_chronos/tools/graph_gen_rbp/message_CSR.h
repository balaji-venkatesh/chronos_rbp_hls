/**
 * Created by vaksenov on 23.07.2019.
 * Ported to C++ by Mark Jeffrey 2021.04.26
 */
#pragma once

#include <array>
#include <cmath>

struct Message {
    uint32_t i;
    uint32_t j;

    std::array<float_t,2> logMu;
    std::array<float_t,2> lookAhead;

    std::array<std::array<float_t,2>,2> logsIn;
};

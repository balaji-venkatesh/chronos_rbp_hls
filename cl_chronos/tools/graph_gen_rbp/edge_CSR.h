/**
 * Created by vaksenov on 25.07.2019.
 * Ported to C++ by Mark Jeffrey 2021.04.26
 */
#pragma once

#include <array>
#include <cstdint>

class Edge {
  public:
    using array2d_t = std::array<std::array<float_t,2>,2>;

    array2d_t logPotentials;

    void setPotential(array2d_t potentials);

    float_t getLogPotential(bool forward, uint32_t vi, uint32_t vj) const;
  
};

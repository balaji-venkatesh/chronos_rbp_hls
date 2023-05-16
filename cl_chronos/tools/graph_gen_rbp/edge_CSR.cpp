#include <cmath>

#include "edge_CSR.h"

void Edge::setPotential(array2d_t potentials) {
    for (uint32_t i = 0; i < 2; i++) {
        for (uint32_t j = 0; j < 2; j++) {
            logPotentials[i][j] = std::log(potentials[i][j]);
        }
    }
}

float_t Edge::getLogPotential(bool forward, uint32_t vi, uint32_t vj) const {
    if (forward) {
        return logPotentials[vi][vj];
    } else {
        return logPotentials[vj][vi];
    }
}

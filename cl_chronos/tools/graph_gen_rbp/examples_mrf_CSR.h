/**
 * Created by vaksenov on 24.07.2019.
 */
#pragma once

#include <cstdint>

class MRF_CSR;

namespace examples_mrf_CSR {

MRF_CSR* isingMRF(uint32_t n, uint32_t m, uint32_t C, uint32_t seed);

MRF_CSR* pottsMRF(uint32_t n, uint32_t C, uint32_t seed);

MRF_CSR* randomTree(uint32_t n, uint32_t C, uint32_t seed);

MRF_CSR* deterministicTree(uint32_t n);

} // namespace examples_mrf

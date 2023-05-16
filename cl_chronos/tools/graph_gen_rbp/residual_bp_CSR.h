#pragma once

#include <vector>
#include <array>

class MRF_CSR;

namespace residual_bp {

void solve(MRF_CSR* mrf, float_t sensitivity,
           std::vector<std::array<float_t,2> >* answer);

} // namespace residual_bp

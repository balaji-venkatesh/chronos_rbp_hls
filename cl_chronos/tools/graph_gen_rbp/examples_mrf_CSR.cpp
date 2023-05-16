#include <array>
#include <cmath>
#include <random>

#include "examples_mrf_CSR.h"
#include "mrf_CSR.h"

MRF_CSR* examples_mrf_CSR::isingMRF(
        uint32_t n, uint32_t m, uint32_t C, uint32_t seed) {
    std::default_random_engine e(seed);
    std::uniform_real_distribution<float_t> randomHalfCSpread(-0.5 * C, 0.5 * C);

    MRF_CSR* mrf = new MRF_CSR(n * m, 2 * n * m - m - n);

    for (uint32_t i = 0; i < n * m; i++) {
        std::array<float_t,2> potential;
        float_t beta = randomHalfCSpread(e);
        for (uint32_t j = 0; j < 2; j++) {
            int v = 2 * j - 1;
            potential[j] = std::exp(beta * v);
        }
        mrf->setNodePotential(i, potential);
    }

    uint32_t dx[] = {1, 0};
    uint32_t dy[] = {0, 1};

    for (uint32_t x = 0; x < n; x++) {
        for (uint32_t y = 0; y < m; y++) {
            uint32_t i = x * m + y;
            for (int k = 0; k < 2; k++) {
                if (x + dx[k] < n && y + dy[k] < m) {
                    uint32_t j = (x + dx[k]) * m + (y + dy[k]);
                    float_t alpha = randomHalfCSpread(e);
                    std::array<std::array<float_t,2>,2> potential;
                    for (int a = 0; a < 2; a++) {
                        for (int b = 0; b < 2; b++) {
                            // N.B. these must be signed!
                            int vi = 2 * a - 1;
                            int vj = 2 * b - 1;
                            potential[a][b] = std::exp(alpha * vi * vj);
                        }
                    }
                    mrf->addEdge(i, j, potential);
                    printf("adding edge (%d, %d)\n", i, j);
                }
            }
        }
    }
    mrf->endAddEdge();
    printf("\n");
    return mrf;
}

MRF_CSR* examples_mrf_CSR::pottsMRF(
        uint32_t n, uint32_t C, uint32_t seed) {
    std::default_random_engine e(seed);
    std::uniform_real_distribution<float_t> randomHalfCSpread(-0.5 * C, 0.5 * C);

    MRF_CSR* mrf = new MRF_CSR(n * n, 2 * n * n - n - n);

    for (uint32_t i = 0; i < n * n; i++) {
        std::array<float_t,2> potential;
        potential[0] = 1;
    }

    uint32_t dx[] = {1, 0};
    uint32_t dy[] = {0, 1};

    for (uint32_t x = 0; x < n; x++){
        for (uint32_t y = 0; y < n; y++){
            uint32_t i = x * n + y;
            for (int k = 0; k < 2; k++){
                if (x + dx[k] < n && y + dy[k] < n){
                    uint32_t j = (x + dx[k]) * n + (y + dy[k]);
                    float_t alpha = randomHalfCSpread(e);
                    std::array<std::array<float_t,2>,2> potential;
                    for (int vali = 0; vali < 2; vali ++){
                        for (int valj = 0; valj < 2; valj++){
                            if (vali == valj){
                                potential[vali][valj] = std::exp(alpha);
                            } else {
                                potential[vali][valj] = 1;
                            }
                        }
                    }
                    mrf->addEdge(i, j, potential);
                    printf("adding edge (%d, %d)\n", i, j);
                }
            }
        }
    }
    mrf->endAddEdge();
    printf("\n");
    return mrf;
}

MRF_CSR* examples_mrf_CSR::randomTree(uint32_t n, uint32_t C, uint32_t seed){
    std::default_random_engine e(seed);
    std::uniform_real_distribution<float_t> randomZeroToC(0.0, C);
    MRF_CSR* mrf = new MRF_CSR(n, n - 1);

    for (uint32_t i = 0; i  < n; i++){
        std::array<float_t, 2> potential;
        for (uint32_t j = 0; j < 2; j++){
            potential[j] = randomZeroToC(e);
        }
        mrf->setNodePotential(i, potential);
    }
    
    for (uint32_t i = 1; i < n; i++){
        std::uniform_int_distribution<uint32_t> randomToI(0, i-1);
        uint32_t p = randomToI(e);
        std::array<std::array<float_t, 2>, 2> potential;
        for (int a = 0; a < 2; a++){
            for (int b = 0; b < 2; b++){
                potential[a][b] = randomZeroToC(e);
            }
        }
        mrf->addEdge(i, p, potential);
        printf("adding edge (%d, %d)\n", i, p);
    }
    mrf->endAddEdge();
    printf("\n");
    return mrf;
}

MRF_CSR* examples_mrf_CSR::deterministicTree(uint32_t n){
    MRF_CSR* mrf = new MRF_CSR(n, n - 1);
    mrf->setNodePotential(0, {0.1, 0.9});
    for (uint32_t i = 1; i < n; i++){
        mrf->setNodePotential(i, {0.5, 0.5});
    }
    std::array<std::array<float_t, 2>, 2> tmp;
    for (int a = 0; a < 2; a++){
        for (int b = 0; b < 2; b++){
            tmp[a][b] = (a==b);
        }
    }

    for (uint32_t i = 2; i <=n; i++){
        mrf->addEdge(i - 1, i / 2 - 1, tmp);
        printf("adding edge (%d, %d)\n", i - 1, i / 2 - 1);
    }
    mrf->endAddEdge();
    printf("\n");
    return mrf;
}
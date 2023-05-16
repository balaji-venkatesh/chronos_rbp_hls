#include <cassert>
#include <iostream>

#include <cmath>
#include <string>
#include <cstring>
#include <fstream>
#include <vector>
#include <array>
#include "examples_mrf_CSR.h"
#include "mrf_CSR.h"
#include "residual_bp_CSR.h"
#include "node_CSR.h"

#include "edge_CSR.h"
#include "message_CSR.h"

#define MAGIC_OP 0xdead


using Results = std::vector<std::array<float_t,2>>;
MRF_CSR* mrf;
MRF_CSR* mrf_sol;
uint32_t numV;
uint32_t numE;
float_t sensitivity = 1e-5;

void WriteOutput(FILE* fp) {
	// all offsets are in units of uint32_t. i.e 16 per cache line

	uint32_t SIZE_EDGE_INDICES = (numV + 1 + 15)/16 * 16;
    uint32_t SIZE_EDGE_DEST = (numE + 15)/16 * 16;
    uint32_t SIZE_REVERSE_EDGE_INDICES = (numV + 1 + 15)/16 * 16;
    uint32_t SIZE_REVERSE_EDGE_DEST = (numE + 15)/16 * 16;
    uint32_t SIZE_REVERSE_EDGE_ID = (numE + 15)/16 * 16;
    uint32_t SIZE_MESSAGE_NODES = (2 * numE * 2 + 15)/16 * 16;
    uint32_t SIZE_NODE_POTENTIALS = (numV * 2 + 15)/16 * 16;
    uint32_t SIZE_EDGE_POTENTIALS = (numE * 4 + 15)/16 * 16;

    uint32_t SIZE_MESSAGES = (2 * numE * 2 + 15)/16 * 16;
    uint32_t SIZE_MESSAGE_PRIORITIES = (2 * numE + 15)/16 * 16;
    uint32_t SIZE_NODE_LOGPRODUCTINS = (numV * 2 + 15)/16 * 16;

    uint32_t BASE_EDGE_INDICES = 16;
    uint32_t BASE_EDGE_DEST = BASE_EDGE_INDICES + SIZE_EDGE_INDICES;
    uint32_t BASE_REVERSE_EDGE_INDICES = BASE_EDGE_DEST + SIZE_EDGE_DEST;
    uint32_t BASE_REVERSE_EDGE_DEST = BASE_REVERSE_EDGE_INDICES + SIZE_REVERSE_EDGE_INDICES;
    uint32_t BASE_REVERSE_EDGE_ID = BASE_REVERSE_EDGE_DEST + SIZE_REVERSE_EDGE_DEST;
    uint32_t BASE_MESSAGE_NODES = BASE_REVERSE_EDGE_ID + SIZE_REVERSE_EDGE_ID;
    uint32_t BASE_NODE_POTENTIALS = BASE_MESSAGE_NODES + SIZE_MESSAGE_NODES;
    uint32_t BASE_EDGE_POTENTIALS = BASE_NODE_POTENTIALS + SIZE_NODE_POTENTIALS;

    uint32_t BASE_MESSAGES = BASE_EDGE_POTENTIALS + SIZE_EDGE_POTENTIALS;
    uint32_t BASE_MESSAGE_PRIORITIES = BASE_MESSAGES + SIZE_MESSAGES;
    uint32_t BASE_NODE_LOGPRODUCTINS = BASE_MESSAGE_PRIORITIES + SIZE_MESSAGE_PRIORITIES;

	uint32_t BASE_END = BASE_NODE_LOGPRODUCTINS + SIZE_NODE_LOGPRODUCTINS;

	uint32_t* data = (uint32_t*) calloc(BASE_END, sizeof(uint32_t));

	data[0] = MAGIC_OP;
	data[1] = numV;
	data[2] = numE;
	data[3] = BASE_EDGE_INDICES;
	data[4] = BASE_EDGE_DEST;
	data[5] = BASE_REVERSE_EDGE_INDICES;
	data[6] = BASE_REVERSE_EDGE_DEST;
	data[7] = BASE_REVERSE_EDGE_ID;
	data[8] = BASE_MESSAGE_NODES;
    data[9] = BASE_NODE_POTENTIALS;
    data[10] = BASE_EDGE_POTENTIALS;

    data[11] = BASE_MESSAGES;
    data[12] = BASE_MESSAGE_PRIORITIES;
    data[13] = BASE_NODE_LOGPRODUCTINS;

	data[14] = *((uint32_t *) &sensitivity);
	data[15] = BASE_END;

    printf("header %d: 0x%4x\n", 0, data[0]);
	for (uint32_t i = 1; i < 13; i++) {
		printf("header %d: %d\n", i, data[i]);
	}
    printf("header %d: %f\n", 14, *((float_t *) &data[14]));
    printf("header %d: %d\n", 15, data[15]);

    for (uint32_t i = 0; i < numV + 1; i++) {
        data[BASE_EDGE_INDICES + i] = mrf->edge_indices[i];
    }

    for (uint32_t i = 0; i < numE; i++) {
        data[BASE_EDGE_DEST + i] = mrf->edge_dest[i];
    }

    for (uint32_t i = 0; i < numV + 1; i++) {
        data[BASE_REVERSE_EDGE_INDICES + i] = mrf->reverse_edge_indices[i];
        assert((BASE_REVERSE_EDGE_INDICES + i) < BASE_END);
    }

    for (uint32_t i = 0; i < numE; i++) {
        data[BASE_REVERSE_EDGE_DEST + i] = mrf->reverse_edge_dest[i];
        assert((BASE_REVERSE_EDGE_DEST + i) < BASE_END);
    }

    for (uint32_t i = 0; i < numE; i++) {
        data[BASE_REVERSE_EDGE_ID + i] = mrf->reverse_edge_id[i];
        assert((BASE_REVERSE_EDGE_ID + i) < BASE_END);
    }

    for (uint32_t i = 0; i < 2 * numE; i++) {
        data[BASE_MESSAGE_NODES + i * 2] = (mrf->messages[i]).i;
        data[BASE_MESSAGE_NODES + i * 2 + 1] = (mrf->messages[i]).j;
        assert((BASE_MESSAGE_NODES + i * 2 + 1) < BASE_END);
    }

    for (uint32_t i = 0; i < numV; i++) {
        data[BASE_NODE_POTENTIALS + i * 2] = *(uint32_t *) &((mrf->nodes[i]).logNodePotentials[0]);
        data[BASE_NODE_POTENTIALS + i * 2 + 1] = *(uint32_t *) &((mrf->nodes[i]).logNodePotentials[1]);
        assert((BASE_NODE_POTENTIALS + i * 2 + 1) < BASE_END);
    }

    for (uint32_t i = 0; i < numE; i++) {
        data[BASE_EDGE_POTENTIALS + i * 4] = *(uint32_t *) &((mrf->edges[i]).logPotentials[0][0]);
        data[BASE_EDGE_POTENTIALS + i * 4 + 1] = *(uint32_t *) &((mrf->edges[i]).logPotentials[0][1]);
        data[BASE_EDGE_POTENTIALS + i * 4 + 2] = *(uint32_t *) &((mrf->edges[i]).logPotentials[1][0]);
        data[BASE_EDGE_POTENTIALS + i * 4 + 3] = *(uint32_t *) &((mrf->edges[i]).logPotentials[1][1]);
        assert((BASE_EDGE_POTENTIALS + i * 4 + 3) < BASE_END);
    }

    for (uint32_t i = 0; i < 2 * numE; i++) {
        // message values
        data[BASE_MESSAGES + i * 2]  = *(uint32_t *) &((mrf->messages[i]).logMu[0]);
        data[BASE_MESSAGES + i * 2 + 1]  = *(uint32_t *) &((mrf->messages[i]).logMu[1]);
        assert((BASE_MESSAGES + i * 2 + 1) < BASE_END);
    }

    for (uint32_t i = 0; i < 2 * numE; i++) {
        // message priorities
        data[BASE_MESSAGE_PRIORITIES + i]  = 0xffffffff;
        assert((BASE_MESSAGE_PRIORITIES + i) < BASE_END);
    }

    for (uint32_t i = 0; i < numV; i++) {
        // node log product ins
        data[BASE_NODE_LOGPRODUCTINS + i * 2]  = *(uint32_t *) &((mrf->nodes[i]).logProductIn[0]);
        data[BASE_NODE_LOGPRODUCTINS + i * 2 + 1]  = *(uint32_t *) &((mrf->nodes[i]).logProductIn[1]);
        assert((BASE_NODE_LOGPRODUCTINS + i * 2 + 1) < BASE_END);
    }

    // for (uint32_t i = 0; i < BASE_END; i++) {
    //     printf("file %d: %d\n", i, data[i]);
    // }

	printf("Writing file \n");
	fwrite(data, 4, BASE_END, fp);
	fclose(fp);

	free(data);

}

int main(int argc, const char** argv) {
    char out_file[50];
    if (argc < 4) {
        std::cerr << "Usage: "
                  << argv[0]
                  << " <algorithm>"
                  << " <mrf>"
                  << " <size>"
                  << " [<threads>]"
                  << std::endl;
        return -1;
    }

    std::string algorithm(argv[1]);
    std::string mrfName(argv[2]);
    uint32_t size = atol(argv[3]);
    assert(size > 0);
    assert(argc == 4);

    if (mrfName == "ising") {
        mrf = examples_mrf_CSR::isingMRF(size, size, 2, 1);
        mrf_sol = examples_mrf_CSR::isingMRF(size, size, 2, 1);
    } else if (mrfName == "potts") {
        mrf = examples_mrf_CSR::pottsMRF(size, 5, 1);
        mrf_sol = examples_mrf_CSR::pottsMRF(size, 5, 1);
    } else if (mrfName == "tree") {
        mrf = examples_mrf_CSR::randomTree(size, 5, 1);
        mrf_sol = examples_mrf_CSR::randomTree(size, 5, 1);
    } else if (mrfName == "deterministic_tree") {
        mrf = examples_mrf_CSR::deterministicTree(size);
        mrf_sol = examples_mrf_CSR::deterministicTree(size);
    } else {
        std::cerr << "Unrecognized MRF: " << mrfName << std::endl;
        return 1;
    }

    numV = mrf->num_nodes;
    numE = mrf->num_edges;

    printf("num_edges = %d\n", numE);

    printf("edge_indices: ");
    for (uint32_t i = 0; i < numV + 1; i++) {
        printf("%d ", mrf->edge_indices[i]);
    }
    printf("\n");

    printf("edge_dest: ");
    for (uint32_t i = 0; i < numE; i++) {
        printf("%d ", mrf->edge_dest[i]);
    }
    printf("\n");

    printf("reverse_edge_indices: ");
    for (uint32_t i = 0; i < numV + 1; i++) {
        printf("%d ", mrf->reverse_edge_indices[i]);
    }
    printf("\n");

    printf("reverse_edge_dest: ");
    for (uint32_t i = 0; i < numE; i++) {
        printf("%d ", mrf->reverse_edge_dest[i]);
    }
    printf("\n");

    printf("reverse_edge_id: ");
    for (uint32_t i = 0; i < numE; i++) {
        printf("%d ", mrf->reverse_edge_id[i]);
    }
    printf("\n");

    for (uint32_t i = 0; i < 2 * numE; i++) {
        printf("Message %d: (%d, %d) = (%f, %f)\n", i, mrf->messages[i].i, mrf->messages[i].j, mrf->messages[i].logMu[0], mrf->messages[i].logMu[1]);
    }
    printf("\n");


    assert(mrf);
    assert(mrf_sol);
    std::cout << "The " << mrfName << " model has been set up" << std::endl;

    // Initialize lookaheads
    for (uint32_t i = 0; i < 2 * numE; i++) {
        (mrf->messages[i]).lookAhead = mrf->getFutureMessageVal(i);
    }

    Results res;

    if (algorithm == "residual") {
        residual_bp::solve(mrf_sol, sensitivity, &res);
    } else {
        std::cerr << "Unrecognized algorithm: " << algorithm << std::endl;
        return 1;
    }

    for (uint32_t i = 0; i < 2 * numE; i++) {
        printf("Converged message %d: (%d, %d) = (%f, %f)\n", i, mrf_sol->messages[i].i, mrf_sol->messages[i].j, mrf_sol->messages[i].logMu[0], mrf_sol->messages[i].logMu[1]);
    }
    printf("\n");

    std::sprintf(out_file, "%s_%d.rbp", mrfName.c_str(), size);
    FILE* fp;
	fp = fopen(out_file, "wb");
	printf("Writing file %s %p\n", out_file, fp);
    WriteOutput(fp);

    
    assert(!res.empty());

    if (mrfName == "deterministic_tree") {
        for (uint32_t i = 0; i < size; i++) {
            if (std::abs (res[i][0] - 0.1) > 0.001) {
                std::cerr << "Something is wrong with vertex "
                          << i << std::endl;
                return 1;
            }
        }
        std::cout << "Everything is fine\n";
    }

    std::string arg3 = std::to_string(size);
    std::string filename("output-" + mrfName + "-" + arg3);
    uint32_t len = res.size();
    uint32_t wid = res[0].size();
    if (algorithm == "residual") {
        std::ofstream ostrm(filename);
        for (uint32_t i = 0; i < len; i++) {
            for (uint32_t j = 0; j < wid; j++) {
                ostrm << res[i][j] << " ";
            }
            ostrm << "\n";
        }
    } else {
        std::ifstream istrm(filename);
        if (! istrm.good()) {
            std::cerr <<  "Correctness-checking file does not exist "
                      << filename << std::endl;
            return 1;
        }

        Results jury(res.size());

        for (uint32_t i = 0; i < len; i++) {
            for (uint32_t j = 0; j < wid; j++) {
                float_t d1;
                istrm >> d1;
                jury[i][j] = d1;
            }
        }

        float_t accuracy = 0;
        float_t accuracyMax = 0;
        for (uint32_t i = 0; i < len; i++) {
            float_t L1 = 0;
            for (uint32_t j = 0; j < wid; j++) {
                L1 += std::abs(jury[i][j] - res[i][j]);
            }
            accuracy += L1;
            accuracyMax = std::max(accuracyMax, L1);
        }
        std::cout << "Accuracy:" << std::to_string(accuracy / res.size()) << std::endl;
        std::cout << "AccuracyMax:" << std::to_string(accuracyMax) << std::endl;
    }
    std::cout << "The first results are "
              << res[0][0]
              << " and "
              << res[0][1]
              << std::endl;

    // delete mrf;
    // delete mrf_sol;

    return 0;
}



#include "mrf_CSR.h"
#include <assert.h>

MRF_CSR::MRF_CSR(uint32_t num_nodes, uint32_t num_edges)
    : num_nodes(num_nodes)
    , num_edges(num_edges)
{
    //allocate memory
    nodes = new Node[num_nodes];
    edge_indices = (uint32_t*) calloc(num_nodes + 1, sizeof(uint32_t));
    edge_dest = (uint32_t*) calloc(num_edges, sizeof(uint32_t));
    edges = new Edge[num_edges];
    messages = new Message[2*num_edges];

    reverse_edge_indices = (uint32_t*) calloc(num_nodes + 1, sizeof(uint32_t));
    reverse_edge_dest = (uint32_t*) calloc(num_edges, sizeof(uint32_t));
    reverse_edge_id = (uint32_t*) calloc(num_edges, sizeof(uint32_t));

    //init id values for nodes
    uint32_t i;

    //init edges and messages
    for (i = 0; i < num_edges; i++) {
        //init logMu for messages
        std::fill((messages[2*i].logMu).begin(), (messages[2*i].logMu).end(), std::log(1.0 / 2));    
        std::fill((messages[2*i + 1].logMu).begin(), (messages[2*i + 1].logMu).end(), std::log(1.0 / 2));     
    }

    // init edge indices to 0
    for (i = 0; i <= num_nodes; i++) {
        edge_indices[i] = 0;
        reverse_edge_indices[i] = 0;
        nodes[i].logProductIn = {};
    }
}

MRF_CSR::~MRF_CSR()
{
    //free memory
    delete [] nodes;
    free(edge_indices);
    free(edge_dest);
    delete [] edges;
    delete [] messages;

    free(reverse_edge_indices);
    free(reverse_edge_dest);
    free(reverse_edge_id);

    // <TODO> (leo): figure out delete/free errors (replicate with "residual ising 2")
}

void MRF_CSR::endAddEdge() {
    std::vector<std::array<edge_id,3>> edge_buffer;
    for (node_id n = 0; n < num_nodes; n++) {
        uint32_t edge_idx_begin = edge_indices[n];
        uint32_t edge_idx_end = edge_indices[n + 1];
        for (edge_id e = edge_idx_begin; e < edge_idx_end; e++) {
            std::array<uint32_t,3> edge;
            edge[0] = edge_dest[e];
            edge[1] = n;
            edge[2] = e;
            edge_buffer.push_back(edge);
        }
    }
    std::sort(edge_buffer.begin(), edge_buffer.end());

    for (uint32_t i = 0; i < edge_buffer.size(); i++) {
        node_id n = edge_buffer[i][0];
        reverse_edge_dest[i] = edge_buffer[i][1];
        reverse_edge_id[i] = edge_buffer[i][2];
        reverse_edge_indices[n + 1] = i + 1;
    }

    // update logProductIn
    for (node_id n = 0; n < num_nodes; n++) {
        IteratorMessagesTo in_messages = getMessagesTo(n);
        while(in_messages.hasNext()) {
            message_id m = in_messages.getNext();
            for (uint32_t i = 0; i < 2; i++) {
                nodes[n].logProductIn[i] += messages[m].logMu[i];
            }
        }
    }
}

void MRF_CSR::addEdge(node_id i, node_id j, Edge::array2d_t phi) {
    edge_id e;
    if (edge_indices[i + 1] == 0) {
        e = edge_indices[i];
    } else {
        e = edge_indices[i + 1];
    }

    edge_dest[e] = j;
    edges[e].setPotential(phi);
    edge_indices[i + 1] = e + 1;

    messages[2*e].i = i;
    messages[2*e].j = j;
    messages[2*e + 1].i = j;
    messages[2*e + 1].j = i;
}

void MRF_CSR::getNodeProbabilities(std::vector<std::array<float_t,2> >* answer) const {
    answer->resize(num_nodes, {0,0});
    for (node_id n = 0; n < num_nodes; n++) {
        std::array<float_t,2>& a = (*answer)[n];
        for (uint32_t vali = 0; vali < 2; vali++) {
            a[vali] = (nodes[n]).logNodePotentials[vali] + (nodes[n]).logProductIn[vali];
        }
        float_t sum = utils::logSum(a);
        for (uint32_t vali = 0; vali < 2; vali++) {
            a[vali] -= sum;
            a[vali] = std::exp(a[vali]);
        }
    }
}

std::array<float_t,2> MRF_CSR::getFutureMessageVal(message_id m_id){
    Message &m = messages[m_id];
    Edge &e = edges[m_id/2];

    uint32_t i = m.i;
    Node &n = nodes[i];

    uint32_t r_m_id = getReverseMessage(m_id);

    //find reverse message
    Message &r_m = messages[r_m_id];

    bool forward;
    if (m_id % 2 == 0) {
        forward = true;
    } else {
        forward = false;
    }

    std::array<float_t,2> result;
    for (uint32_t valj = 0; valj < 2; valj++) {
        std::array<float_t,2> logsIn;
        for (uint32_t vali = 0; vali < 2; vali++) {
            logsIn[vali] = e.getLogPotential(forward, vali, valj)
                    + n.logNodePotentials[vali]
                    + (n.logProductIn[vali] - r_m.logMu[vali]);
        }
        result[valj] = utils::logSum(logsIn);
    }
    float_t logTotalSum = utils::logSum(result);

    // normalization
    for (uint32_t valj = 0; valj < 2; valj++) {
        result[valj] -= logTotalSum;
    }

    return result;
}

std::array<float_t,2> MRF_CSR::updateLookAhead(message_id m_id){
    Message &m = messages[m_id];
    Edge &e = edges[m_id/2];

    uint32_t i = m.i;
    Node &n = nodes[i];

    uint32_t r_m_id = getReverseMessage(m_id);

    //find reverse message
    Message &r_m = messages[r_m_id];

    bool forward;
    if (m_id % 2 == 0) {
        forward = true;
    } else {
        forward = false;
    }

    std::array<float_t,2> result;
    for (uint32_t valj = 0; valj < 2; valj++) {
        for (uint32_t vali = 0; vali < 2; vali++) {
            m.logsIn[valj][vali] = e.getLogPotential(forward, vali, valj)
                    + n.logNodePotentials[vali]
                    + (n.logProductIn[vali] - r_m.logMu[vali]);
        }
        result[valj] = utils::logSum(m.logsIn[valj]);
    }
    float_t logTotalSum = utils::logSum(result);

    // normalization
    for (uint32_t valj = 0; valj < 2; valj++) {
        result[valj] -= logTotalSum;
    }

    // update lookAhead
    m.lookAhead[0] = result[0];
    m.lookAhead[1] = result[1];

    return result;
}
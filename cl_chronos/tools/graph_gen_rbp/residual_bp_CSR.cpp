/**
 * Created by vaksenov on 24.07.2019.
 * Ported to C++ by Mark Jeffrey 2021.04.29
 */

#include <array>
#include <boost/heap/fibonacci_heap.hpp>
#include <cstdlib>
#include <iostream>
#include <tuple>
#include <vector>

#include "message_CSR.h"
#include "mrf_CSR.h"
#include "residual_bp_CSR.h"

namespace residual_bp {

// This algorithm schedules its updates of messages using max heap. A message's
// priority is the amount of change the update would incur. This residual
// algorithm therefore performs "lookahead", computing the change before it
// happens.
// TODO: check how the non-lookahead version performs, along with the sequential
// relaxed version. Basically re-read the supplemental paper and check all
// 1-thread run times in Table 6 onward
//
// There are a couple of interesting challenges here when looking through the
// Swarm lens:
// (1) The algorithm opts to change a Message's priority, rather than schedule
//     new actions for the same Message (or not scheduling further work).
//     Wikipedia's discussion of heaps refers to this as "decreasing" or
//     "increasing" keys:
//     https://en.wikipedia.org/wiki/Heap_(data_structure)#Operations
//     If we use std::priority_queue without an update function, what is the
//     right approach to handle this sort of scheduling?
// (2) A message's priority is not necessarily monotonically decreasing. When a
//     message creates new work for a neighbor that is higher priority than its
//     own, we can mock this using a new Fractal domain. Alternatively, since
//     the scheduling is flexible, maybe we can just give it equal timestamp to
//     its parent, mocking a sense of urgency.
//     But what about when a Message's priority is *changed* to be lower than
//     its current priority? When the priority drops to zero, it's fair to say
//     we could simply not creating a new task for that message. But if the
//     priority is changed to a lower but nonzero value, how do we handle this
//     absent a priority queue with "decrease key"? Maybe we add a "current
//     priority" value to each Message, and just skip the update if the priority
//     from the queue is higher than the "current priority"?

// We use a Boost heap below since it implements an "update" function, whereas
// std::priority_queue does not. We leverage the lexicographical comparison of
// std::tuple for ordering, but we need to specify the use of std::less.
// Inexplicably, this is how we make a max heap with the Boost fibonacci_heap
// rather than std::greater. I don't get it...
using PQElement = std::tuple<float_t, uint32_t>;
using PQ = boost::heap::fibonacci_heap<
            PQElement,
            boost::heap::compare<std::less<PQElement> >
            >;
// TODO Use a d_ary heap since it has a reserve() method?
// <boost/heap/d_ary_heap.hpp>


// [mcj] Since the Java ResidualBP class is basically a singleton with little
// abstraction, let's not bother with OOO for it.
static float_t sensitivity;
static MRF_CSR* mrf;
static std::vector<PQ::handle_type> pqHandles;


static inline float_t priority(message_id m) {
    return utils::distance(mrf->getMessageVal(m), mrf->getFutureMessageVal(m));
}

void solve(MRF_CSR* mrf, float_t sensitivity,
           std::vector<std::array<float_t,2> >* answer) {
    std::cout << "Running the sequential residual BP algorithm" << std::endl;

    residual_bp::sensitivity = sensitivity;
    residual_bp::mrf = mrf;


    uint32_t num_messages = mrf->getNumMessages();
    pqHandles.reserve(num_messages);

    PQ pq;
    for (message_id m = 0; m < num_messages; m++) {
        auto handle = pq.push(std::make_tuple(priority(m), m));
        pqHandles.push_back(handle);
    }

    int it = 0;
    while (!pq.empty()) {
        float_t prio;
        message_id m;
        std::tie(prio, m) = pq.top();
        if (prio <= sensitivity) break;

        mrf->updateMessage(m, mrf->getFutureMessageVal(m));
        pq.update(pqHandles[m], std::make_tuple(0.0, m));

        node_id dest = mrf->getDest(m);
        IteratorMessagesFrom affected_messages = mrf->getMessagesFrom(dest);
        while (affected_messages.hasNext()) {
            m = affected_messages.getNext();
            pq.update(pqHandles[m], std::make_tuple(priority(m), m));
        }
        it++;
    }

    std::cout << "Updates " << it << std::endl;

    mrf->getNodeProbabilities(answer);
}

} // namespace residual_bp

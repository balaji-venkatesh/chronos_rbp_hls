#include <cstdint>
#include <stdio.h>
#include <iostream>

typedef uint32_t node_id;
typedef uint32_t message_id;
typedef uint32_t edge_id;

class Iterator {
    public:
        virtual bool hasNext() = 0;
        virtual message_id getNext() = 0;
};

class IteratorMessagesFrom : public Iterator {
    private:
        node_id n;
        edge_id *reverse_edge_id;
        uint32_t CSR_position;
        uint32_t CSC_position;
        uint32_t CSR_end;
        uint32_t CSC_end;
        Message *messages;

    public:
        IteratorMessagesFrom(node_id n, uint32_t *edge_indices, uint32_t *reverse_edge_indices, edge_id *reverse_edge_id, Message *messages)
            : n(n)
            , reverse_edge_id(reverse_edge_id)
            , messages(messages)
        {
            CSR_position = edge_indices[n];
            CSR_end = edge_indices[n + 1];
            CSC_position = reverse_edge_indices[n];
            CSC_end = reverse_edge_indices[n + 1];
        }

        bool hasNext() {
            if ((CSR_position < CSR_end) || (CSC_position < CSC_end)) {
                return true;
            } else {
                return false;
            }
        }

        message_id getNext() {
            if (CSR_position < CSR_end) {
                message_id next = CSR_position * 2;
                CSR_position++;
                // if (CSR_position < CSR_end) {
                //     __builtin_prefetch(&messages[next + 2]);
                // }
                return next;
            } else if (CSC_position < CSC_end) {
                message_id next = reverse_edge_id[CSC_position] * 2 + 1;
                CSC_position++;
                // if (CSC_position < CSC_end) {
                //     message_id next_next = reverse_edge_id[CSC_position] * 2 + 1;
                //     __builtin_prefetch(&messages[next_next]);
                // }
                return next;
            } else {
                std::cout << "getNext() invalid call, please use hasNext() to check if getNext() can be called\n";
                return 0;
            }
        }
};

class IteratorMessagesTo : public Iterator {
    private:
        node_id n;
        edge_id *reverse_edge_id;
        uint32_t CSR_position;
        uint32_t CSC_position;
        uint32_t CSR_end;
        uint32_t CSC_end;
        Message *messages;

    public:
        IteratorMessagesTo(node_id n, uint32_t *edge_indices, uint32_t *reverse_edge_indices, edge_id *reverse_edge_id, Message *messages)
            : n(n)
            , reverse_edge_id(reverse_edge_id)
            , messages(messages)
        {
            CSR_position = edge_indices[n];
            CSR_end = edge_indices[n + 1];
            CSC_position = reverse_edge_indices[n];
            CSC_end = reverse_edge_indices[n + 1];
        }

        bool hasNext() {
            if ((CSR_position < CSR_end) || (CSC_position < CSC_end)) {
                return true;
            } else {
                return false;
            }
        }

        message_id getNext() {
            if (CSR_position < CSR_end) {
                message_id next = CSR_position * 2 + 1;
                CSR_position++;
                // if (CSR_position < CSR_end) {
                //     __builtin_prefetch(&messages[next + 2]);
                // }
                return next;
            } else if (CSC_position < CSC_end) {
                message_id next = reverse_edge_id[CSC_position] * 2;
                CSC_position++;
                // if (CSC_position < CSC_end) {
                //     message_id next_next = reverse_edge_id[CSC_position] * 2;
                //     __builtin_prefetch(&messages[next_next]);
                // }
                return next;
            } else {
                std::cout << "getNext() invalid call, please use hasNext() to check if getNext() can be called\n";
                return 0;
            }
        }
};
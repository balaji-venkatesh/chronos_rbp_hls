# Amazon FPGA Hardware Development Kit
#
# Copyright 2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
# Licensed under the Amazon Software License (the "License"). You may not use
# this file except in compliance with the License. A copy of the License is
# located at
#
#    http://aws.amazon.com/asl/
#
# or in the "license" file accompanying this file. This file is distributed on
# an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, express or
# implied. See the License for the specific language governing permissions and
# limitations under the License.

#VPATH = src:include:$(HDK_DIR)/common/software/src:$(HDK_DIR)/common/software/include

CC = g++
CFLAGS = -std=c++11 -O3 -Wall -g

LDLIBS = -lrt -lpthread

SRC = graph_gen_rbp.cpp examples_mrf_CSR.cpp edge_CSR.cpp mrf_CSR.cpp residual_bp_CSR.cpp
OBJ = $(SRC:.c=.o)
BIN = graph_gen_rbp examples_mrf_CSR edge_CSR mrf_CSR residual_bp_CSR

all: $(BIN) 

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f *.o $(BIN) *.rbp output-*


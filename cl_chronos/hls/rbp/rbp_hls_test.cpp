/** $lic$
 * Copyright (C) 2014-2019 by Massachusetts Institute of Technology
 *
 * This file is part of the Chronos FPGA Acceleration Framework.
 *
 * Chronos is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2.
 *
 * If you use this framework in your research, we request that you reference
 * the Chronos paper ("Chronos: Efficient Speculative Parallelism for
 * Accelerators", Abeydeera and Sanchez, ASPLOS-25, March 2020), and that
 * you send us a citation of your work.
 *
 * Chronos is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*******************************************************************************
Vendor: Xilinx
Associated Filename: array_io_test.c
Purpose: Vivado HLS tutorial example
Device: All
Revision History: March 1, 2013 - initial release

*******************************************************************************
Copyright 2008 - 2013 Xilinx, Inc. All rights reserved.

This file contains confidential and proprietary information of Xilinx, Inc. and
is protected under U.S. and international copyright and other intellectual
property laws.

DISCLAIMER
This disclaimer is not a license and does not grant any rights to the materials
distributed herewith. Except as otherwise provided in a valid license issued to
you by Xilinx, and to the maximum extent permitted by applicable law:
(1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL FAULTS, AND XILINX
HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY,
INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-INFRINGEMENT, OR
FITNESS FOR ANY PARTICULAR PURPOSE; and (2) Xilinx shall not be liable (whether
in contract or tort, including negligence, or under any other theory of
liability) for any loss or damage of any kind or nature related to, arising under
or in connection with these materials, including for any direct, or any indirect,
special, incidental, or consequential loss or damage (including loss of data,
profits, goodwill, or any type of loss or damage suffered as a result of any
action brought by a third party) even if such damage or loss was reasonably
foreseeable or Xilinx had been advised of the possibility of the same.

CRITICAL APPLICATIONS
Xilinx products are not designed or intended to be fail-safe, or for use in any
application requiring fail-safe performance, such as life-support or safety
devices or systems, Class III medical devices, nuclear facilities, applications
related to the deployment of airbags, or any other applications that could lead
to death, personal injury, or severe property or environmental damage
(individually and collectively, "Critical Applications"). Customer asresultes the
sole risk and liability of any use of Xilinx products in Critical Applications,
subject only to applicable laws and regulations governing limitations on product
liability.

THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE AT
ALL TIMES.

*******************************************************************************/
#include "rbp_hls.h"

#include "queue"
#include <stdio.h>
#include <stdlib.h>

struct compare_task {
	bool operator() (const task_t &a, const task_t &b) const {
		return a.ts > b.ts;
	}
};

int main () {

	// Create input data
  task_t task_in = {0,0,0,0};
  hls::stream<undo_log_t> undo_log_entry;


  std::priority_queue<task_t, std::vector<task_t>, compare_task > pq;

  ap_uint<32> mem[16384] = {0};
  FILE* fp = fopen("input_rbp", "rb");
  printf("File %p\n", fp);
  if (fp == NULL) {
     printf("Error opening file");
  }

  fread(&mem, sizeof(ap_uint<32>), 16384, fp);

  // Push in initial tasks
  ap_uint<32> nume = mem[2];
  ap_uint<32> base_end = mem[15];
  ap_uint<32> base_messages = mem[11];
  printf("%d\n", (unsigned int) nume);

  for (int i = 0; i < nume * 2; i+=2) {
	  args_t in_args(i+1,0,0,0);
	  //in_args.unpacked.arg0 = i + 1;
	  task_t initial_task = {0,i,0,in_args.packed};
	  printf("\t Enqueue: (%u, %u), args: (%u, %u, %u, %u)\n", (unsigned int) (initial_task.ts), (unsigned int) (initial_task.object),
			  (unsigned int)(in_args.unpacked.arg0), (unsigned int)(in_args.unpacked.arg1), (unsigned int)(in_args.unpacked.arg2), (unsigned int)(in_args.unpacked.arg3));
	  pq.push(initial_task);
  }

  for (int i = 1; i < nume * 2; i+=2) {
	  args_t in_args(i-1,0,0,0);
	  //in_args.unpacked.arg0 = i - 1;
	  task_t initial_task = {0,i,0,in_args.packed};
	  printf("\t Enqueue: (%u, %u), args: (%u, %u, %u, %u)\n", (unsigned int) (initial_task.ts), (unsigned int) (initial_task.object),
	  			  (unsigned int)(in_args.unpacked.arg0), (unsigned int)(in_args.unpacked.arg1), (unsigned int)(in_args.unpacked.arg2), (unsigned int)(in_args.unpacked.arg3));
	  pq.push(initial_task);
  }

  while(!pq.empty()) {
	  task_t task_in = pq.top();
	  pq.pop();
	  args_t in_args(0,0,0,0);
	  in_args.packed = task_in.args;
	  printf("\t Dequeue: (%u, %u), args: (%u, %u, %u, %u)\n", (unsigned int) (task_in.ts), (unsigned int) (task_in.object),
	  	  			  (unsigned int)(in_args.unpacked.arg0), (unsigned int)(in_args.unpacked.arg1), (unsigned int)(in_args.unpacked.arg2), (unsigned int)(in_args.unpacked.arg3));

	  hls::stream<task_t> task_out;
	  rbp_hls(task_in, &task_out, mem, &undo_log_entry);

	  task_t out;
	  while(!task_out.empty()) {
		  out = task_out.read();
		  pq.push(out);
		  args_t out_args(0,0,0,0);
		  out_args.packed = out.args;
		  printf("\t Enqueue: (%u, %u), args: (%u, %u, %u, %u)\n", (unsigned int) (out.ts), (unsigned int) (out.object),
		  	  			  (unsigned int)(out_args.unpacked.arg0), (unsigned int)(out_args.unpacked.arg1), (unsigned int)(out_args.unpacked.arg2), (unsigned int)(out_args.unpacked.arg3));
	  }
  }

  for (int i = 0; i < nume * 2; i++) {
	  float logmu[2];
	  union IntFloat temp_logmu[2];
	  temp_logmu[0].intval = mem[base_messages + i * 2];
	  temp_logmu[1].intval = mem[base_messages + i * 2 + 1];
	  logmu[0] = temp_logmu[0].floatval;
	  logmu[1] = temp_logmu[1].floatval;
	  printf("Converged message %d: [%f, %f]\n", i, logmu[0], logmu[1]);
  }

	// Return 0 if the test passes
  return 0;
}

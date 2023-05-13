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
Associated Filename: array_io.h
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
#ifndef RBP_H_
#define RBP_H_

#define READ_REVERSE_MESSAGE_TASK 0
#define CALC_LOOKAHEAD_TASK 1
#define CALC_PRIORITY_TASK 2
#define WRITE_PRIORITY_TASK 3
#define UPDATE_MESSAGE_TASK 4
#define UPDATE_MESSAGE_VAL_TASK 5
#define UPDATE_NODE_LOGPRODUCTIN_TASK 6
 
#include <stdio.h>
#include "hls_stream.h"
#include "hls_math.h"
#include "ap_int.h"

typedef struct {
	ap_uint<32> ts;
	ap_uint<32> object;
	ap_uint<4> ttype;
	ap_uint<128> args;
} task_t;

typedef struct {
	ap_uint<32> addr;
	ap_uint<32> data;
} undo_log_t;

typedef union IntFloat {
	unsigned int intval;
	float floatval;
};

typedef union Args {
	struct { ap_uint<32> arg0; ap_uint<32> arg1;
		ap_uint<32> arg2; ap_uint<32> arg3; } unpacked;
	ap_uint<128> packed;
};

typedef ap_uint<32> addr_t;

void rbp_core (task_t task_in, hls::stream<task_t>* task_out, ap_uint<32>* l1, hls::stream<undo_log_t>* undo_log_entry);

static inline float logSum(float log1, float log2) {
   float max = log1 > log2 ? log1 : log2;
   float min = log1 < log2 ? log1 : log2;
   float ans = max + hls::log(1.0 + hls::exp(min - max));
   return ans;
}

static inline float distance(float log00, float log01, float log10, float log11) {
   float ans = 0.0;
   // split each expf into a task use direct cast to int value as id
   ans += hls::abs(hls::exp(log00) - hls::exp(log10));
   ans += hls::abs(hls::exp(log01) - hls::exp(log11));
   return ans;
}

#define UINT32_MAX		    (4294967295U)
#define SCALING_FACTOR 		(1U << 31)

static inline ap_uint<32> timestamp(float dist) {
   float scaled = dist * SCALING_FACTOR;
   ap_uint<32> ts = UINT32_MAX - 8 - hls::round(scaled);
   return ts;
}
#endif

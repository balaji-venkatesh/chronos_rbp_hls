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
Associated Filename: array_io.c
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


void rbp_hls (task_t task_in, hls::stream<task_t>* task_out, ap_uint<32>* l1, hls::stream<undo_log_t>* undo_log_entry) {
#pragma HLS INTERFACE axis port=undo_log_entry
#pragma HLS INTERFACE m_axi depth=1000 port=l1
#pragma HLS INTERFACE axis port=task_out

	int i;

	static ap_uint<1> initialized = 0;

	static float sensitivity;
	static ap_uint<32> numv;
	static ap_uint<32> nume;

	// Read-Only
	static ap_uint<32> base_edge_indices;
	static ap_uint<32> base_edge_dest;
	static ap_uint<32> base_reverse_edge_indices;
	static ap_uint<32> base_reverse_edge_dest;
	static ap_uint<32> base_reverse_edge_id;
	static ap_uint<32> base_message_nodes;
	static ap_uint<32> base_node_potentials;
	static ap_uint<32> base_edge_potentials;

	// Read-Write
	static ap_uint<32> base_messages;
	static ap_uint<32> base_message_priorities;
	static ap_uint<32> base_node_logproductins;
	

	if (!initialized) {
		union IntFloat temp_intfp;
		initialized = 1;
		numv = l1[1];
		nume = l1[2];

		// Read-Only
		base_edge_indices = l1[3];
		base_edge_dest = l1[4];
		base_reverse_edge_indices = l1[5];
		base_reverse_edge_dest = l1[6];
		base_reverse_edge_id = l1[7];
		base_message_nodes = l1[8];
		base_node_potentials = l1[9];
		base_edge_potentials = l1[10];

		// Read-Write
		base_messages = l1[11];
		base_message_priorities = l1[12];
		base_node_logproductins = l1[13];

		// Sensitivity
		temp_intfp.intval = l1[14];
		sensitivity = temp_intfp.floatval;
	}

	if (task_in.ttype == READ_REVERSE_MESSAGE_TASK) {
		// Read reverse message
		ap_uint<32> reverse_mid = task_in.object;
		ap_uint<32> logmu[2];
		logmu[0] = l1[base_messages + (2 * reverse_mid)];
		logmu[1] = l1[base_messages + (2 * reverse_mid) + 1];

		// Find message id
		args_t in_args(0,0,0,0);
		in_args.packed = task_in.args;
		ap_uint<32> mid = in_args.unpacked.arg0;

		// Find source node id
		ap_uint<32> source_nid = l1[base_message_nodes + (2 * mid)];

		// Enqueue CALC_LOOKAHEAD_TASK
		args_t out_args(logmu[0], logmu[1], mid, 0);
		// out_args.unpacked.arg0 = logmu[0];
		// out_args.unpacked.arg1 = logmu[1];
		// out_args.unpacked.arg2 = mid;

		task_t task_out_temp = {task_in.ts + 1, source_nid + 4 * nume, CALC_LOOKAHEAD_TASK, out_args.packed};
		//task_out_temp.ts = task_in.ts + 1;
		//task_out_temp.object = source_nid + 4 * nume;
		//task_out_temp.ttype = CALC_LOOKAHEAD_TASK;
		//task_out_temp.args = out_args.packed;
		//task_out_temp.no_write = 1;

		task_out->write(task_out_temp);

	} else if (task_in.ttype == CALC_LOOKAHEAD_TASK) {
		// Read node logproductin
		ap_uint<32> nid = task_in.object - 4 * nume;
		float logproductin[2];
		union IntFloat temp_node_logproductin[2];
		// printf("idx = %d\n", (int) (base_node_logproductins + (2 * nid)));
		temp_node_logproductin[0].intval = l1[base_node_logproductins + (2 * nid)];
		temp_node_logproductin[1].intval = l1[base_node_logproductins + (2 * nid) + 1];
		logproductin[0] = temp_node_logproductin[0].floatval;
		logproductin[1] = temp_node_logproductin[1].floatval;
		//printf("nid: %d, logproductin: [%f, %f]\n", (int) nid, logproductin[0], logproductin[1]);

		// Read edge potentials
		args_t in_args(0,0,0,0);
		in_args.packed = task_in.args;
		ap_uint<32> mid = in_args.unpacked.arg2;

		ap_uint<32> eid = mid/2;
		float edge_potentials[2][2];
		union IntFloat temp_edge_potentials[2][2];
		// printf("idx = %d\n", (int) (base_edge_potentials + (4 * eid)));
		temp_edge_potentials[0][0].intval = l1[(base_edge_potentials + (4 * eid))];
		temp_edge_potentials[0][1].intval = l1[base_edge_potentials + (4 * eid) + 1];
		temp_edge_potentials[1][0].intval = l1[base_edge_potentials + (4 * eid) + 2];
		temp_edge_potentials[1][1].intval = l1[base_edge_potentials + (4 * eid) + 3];
		edge_potentials[0][0] = temp_edge_potentials[0][0].floatval;
		edge_potentials[0][1] = temp_edge_potentials[0][1].floatval;
		edge_potentials[1][0] = temp_edge_potentials[1][0].floatval;
		edge_potentials[1][1] = temp_edge_potentials[1][1].floatval;
		//printf("eid: %d, edge_potentials: [%f, %f, %f, %f]\n", (int) eid, edge_potentials[0][0], edge_potentials[0][1], edge_potentials[1][0], edge_potentials[1][1]);

		// Read node potentials
		float node_potentials[2];
		union IntFloat temp_node_potentials[2];
		temp_node_potentials[0].intval = l1[base_node_potentials + (2 * nid)];
		temp_node_potentials[1].intval = l1[base_node_potentials + (2 * nid) + 1];
		node_potentials[0] = temp_node_potentials[0].floatval;
		node_potentials[1] = temp_node_potentials[1].floatval;
		//printf("nid: %d, node_potentials: [%f, %f]\n", (int) nid, node_potentials[0], node_potentials[1]);

		// Read reverse message logmu
		float reverse_logmu[2];
		union IntFloat temp_reverse_logmu[2];
		temp_reverse_logmu[0].intval = in_args.unpacked.arg0;
		temp_reverse_logmu[1].intval = in_args.unpacked.arg1;
		reverse_logmu[0] = temp_reverse_logmu[0].floatval;
		reverse_logmu[1] = temp_reverse_logmu[1].floatval;
		// printf("mid: %d, reverse_logmu: [%f, %f]\n", (int) mid, reverse_logmu[0], reverse_logmu[1]);

		// Calculate lookahead
		float lookahead[2];
		if (mid % 2 == 0) {
			for (ap_uint<4> valj = 0; valj < 2; valj++) {
				#pragma HLS UNROLL
				float logsin[2];
				for (ap_uint<4> vali = 0; vali < 2; vali++) {
					#pragma HLS UNROLL
					logsin[vali] = edge_potentials[vali][valj]
							+ node_potentials[vali]
							+ (logproductin[vali] - reverse_logmu[vali]);
				}
				lookahead[valj] = logSum(logsin[0], logsin[1]);
			}
		} else {
			for (ap_uint<4> valj = 0; valj < 2; valj++) {
				#pragma HLS UNROLL
				float logsin[2];
				for (ap_uint<4> vali = 0; vali < 2; vali++) {
					#pragma HLS UNROLL
					logsin[vali] = edge_potentials[valj][vali]
							+ node_potentials[vali]
							+ (logproductin[vali] - reverse_logmu[vali]);
				}
				lookahead[valj] = logSum(logsin[0], logsin[1]);
			}
		}
		float logtotalsum = logSum(lookahead[0], lookahead[1]);

		// normalization
		for (ap_uint<4> valj = 0; valj < 2; valj++) {
			#pragma HLS UNROLL
			lookahead[valj] -= logtotalsum;
		}

		// printf("mid: %d, lookahead: [%f, %f]\n", (int) mid, lookahead[0], lookahead[1]);

		// Enqueue CALC_PRIORITY_TASK
		union IntFloat temp_lookahead[2];
		temp_lookahead[0].floatval = lookahead[0];
		temp_lookahead[1].floatval = lookahead[1];

		args_t out_args(temp_lookahead[0].intval, temp_lookahead[1].intval, 0, 0);
		//out_args.unpacked.arg0 = temp_lookahead[0].intval;
		//out_args.unpacked.arg1 = temp_lookahead[1].intval;

		task_t task_out_temp;
		task_out_temp.ts = task_in.ts + 1;
		task_out_temp.object = mid;
		task_out_temp.ttype = CALC_PRIORITY_TASK;
		task_out_temp.args = out_args.packed;
		//task_out_temp.no_write = 1;

		task_out->write(task_out_temp);

	} else if (task_in.ttype == CALC_PRIORITY_TASK) {
		// Read message logmu
		ap_uint<32> mid = task_in.object;
		float logmu[2];
		union IntFloat temp_logmu[2];
		temp_logmu[0].intval = l1[base_messages + (2 * mid)];
		temp_logmu[1].intval = l1[base_messages + (2 * mid) + 1];
		logmu[0] = temp_logmu[0].floatval;
		logmu[1] = temp_logmu[1].floatval;
		// printf("mid: %d, logmu: [%f, %f]\n", (int) mid, logmu[0], logmu[1]);

		// Read reverse logmu
		args_t in_args(0,0,0,0);
		in_args.packed = task_in.args;
		float lookahead[2];
		union IntFloat temp_lookahead[2];
		temp_lookahead[0].intval = in_args.unpacked.arg0;
		temp_lookahead[1].intval = in_args.unpacked.arg1;
		lookahead[0] = temp_lookahead[0].floatval;
		lookahead[1] = temp_lookahead[1].floatval;
		// printf("mid: %d, lookahead: [%f, %f]\n", (int) mid, lookahead[0], lookahead[1]);

		// Calculate residual
		float residual = distance(logmu[0], logmu[1], lookahead[0], lookahead[1]);

		if (residual > sensitivity) {
			// Calculate priority
			ap_uint<32> update_ts = timestamp(residual);

			// printf("mid: %d, residual: %f, update_ts: %d\n", (int) mid, residual, (int) update_ts);

			// Enqueue WRITE_PRIORITY_TASK
			args_t out_args(in_args.unpacked.arg0,in_args.unpacked.arg1,update_ts,0);
			//out_args.unpacked.arg0 = in_args.unpacked.arg0;
			//out_args.unpacked.arg1 = in_args.unpacked.arg1;
			//out_args.unpacked.arg2 = update_ts;

			task_t task_out_temp;
			task_out_temp.ts = task_in.ts + 1;
			task_out_temp.object = mid + 2 * nume;
			task_out_temp.ttype = WRITE_PRIORITY_TASK;
			task_out_temp.args = out_args.packed;
			//task_out_temp.no_write = 0;

			task_out->write(task_out_temp);
		}

	} else if (task_in.ttype == WRITE_PRIORITY_TASK) {
		// Read priority
		args_t in_args(0,0,0,0);
		in_args.packed = task_in.args;
		ap_uint<32> update_ts = in_args.unpacked.arg2;

		// Write priority
		ap_uint<32> pid = task_in.object - 2 * nume;
		ap_uint<32> old_ts = l1[base_message_priorities + pid];

		undo_log_t ulog;
		ulog.addr = (base_message_priorities + pid) << 2;
		ulog.data = old_ts;
		undo_log_entry->write(ulog);

		// Enqueue UPDATE_MESSAGE_TASK
		args_t out_args(in_args.unpacked.arg0,in_args.unpacked.arg1,0,0);
		//out_args.unpacked.arg0 = in_args.unpacked.arg0;
		//out_args.unpacked.arg1 = in_args.unpacked.arg1;

		task_t task_out_temp;
		if (update_ts > task_in.ts) {
			task_out_temp.ts = update_ts;
			l1[base_message_priorities + pid] = update_ts;
		} else {
			task_out_temp.ts = task_in.ts + 1;
			l1[base_message_priorities + pid] = task_in.ts + 1;
		}
		task_out_temp.object = task_in.object;
		task_out_temp.ttype = UPDATE_MESSAGE_TASK;
		task_out_temp.args = out_args.packed;
		//task_out_temp.no_write = 1;

		task_out->write(task_out_temp);

	} else if (task_in.ttype == UPDATE_MESSAGE_TASK) {
		// Read latest priority and compare with enqueued priority
		ap_uint<32> enq_ts = task_in.ts;
		ap_uint<32> pid = task_in.object - 2 * nume;
		ap_uint<32> latest_ts = l1[base_message_priorities + pid];

		if (enq_ts == latest_ts) {
			ap_uint<32> mid = pid;

			// Read input args
			args_t in_args(0,0,0,0);
			in_args.packed = task_in.args;

			// Enqueue UPDATE_MESSAGE_VAL_TASK
			args_t out_args(in_args.unpacked.arg0,in_args.unpacked.arg1,0,0);
			//out_args.unpacked.arg0 = in_args.unpacked.arg0;
			//out_args.unpacked.arg1 = in_args.unpacked.arg1;

			task_t task_out_temp;
			task_out_temp.ts = enq_ts + 1;
			task_out_temp.object = mid;
			task_out_temp.ttype = UPDATE_MESSAGE_VAL_TASK;
			task_out_temp.args = out_args.packed;
			//task_out_temp.no_write = 0;

			task_out->write(task_out_temp);
		}

	} else if (task_in.ttype == UPDATE_MESSAGE_VAL_TASK) {
		// Read lookaheads
		args_t in_args(0,0,0,0);
		in_args.packed = task_in.args;
		float lookahead[2];
		union IntFloat temp_lookahead[2];
		temp_lookahead[0].intval = in_args.unpacked.arg0;
		temp_lookahead[1].intval = in_args.unpacked.arg1;
		lookahead[0] = temp_lookahead[0].floatval;
		lookahead[1] = temp_lookahead[1].floatval;

		// Read logmu
		ap_uint<32> mid = task_in.object;
		float logmu[2];
		union IntFloat temp_logmu[2];
		temp_logmu[0].intval = l1[base_messages + (2 * mid)];
		temp_logmu[1].intval = l1[base_messages + (2 * mid) + 1];
		logmu[0] = temp_logmu[0].floatval;
		logmu[1] = temp_logmu[1].floatval;

		// Write logmu
		undo_log_t ulog;
		ulog.addr = (base_messages + (2 * mid)) << 2;
		ulog.data = temp_logmu[0].intval;
		undo_log_entry->write(ulog);

		ulog.addr = ((base_messages + (2 * mid)) + 1) << 2;
		ulog.data = temp_logmu[1].intval;
		undo_log_entry->write(ulog);

		l1[base_messages + (2 * mid)] = in_args.unpacked.arg0;
		l1[base_messages + (2 * mid) + 1] = in_args.unpacked.arg1;

		// Calculate difference between old and new logmu
		float diff[2];
		diff[0] = lookahead[0] - logmu[0];
		diff[1] = lookahead[1] - logmu[1];
		union IntFloat temp_diff[2];
		temp_diff[0].floatval = diff[0];
		temp_diff[1].floatval = diff[1];

		// Find destination node id
		ap_uint<32> nid = l1[base_message_nodes + (2 * mid) + 1];

		// Enqueue UPDATE_NODE_LOGPRODUCTIN_TASK
		args_t out_args(temp_diff[0].intval,temp_diff[1].intval,mid,0);
		//out_args.unpacked.arg0 = temp_diff[0].intval;
		//out_args.unpacked.arg1 = temp_diff[1].intval;
		//out_args.unpacked.arg2 = mid;

		task_t task_out_temp;
		task_out_temp.ts = task_in.ts + 1;
		task_out_temp.object = nid + 4 * nume;
		task_out_temp.ttype = UPDATE_NODE_LOGPRODUCTIN_TASK;
		task_out_temp.args = out_args.packed;
		//task_out_temp.no_write = 0;

		task_out->write(task_out_temp);
					
	} else if (task_in.ttype == UPDATE_NODE_LOGPRODUCTIN_TASK) {
		// Read diff
		args_t in_args(0,0,0,0);
		in_args.packed = task_in.args;
		float diff[2];
		union IntFloat temp_diff[2];
		temp_diff[0].intval = in_args.unpacked.arg0;
		temp_diff[1].intval = in_args.unpacked.arg1;
		diff[0] = temp_diff[0].floatval;
		diff[1] = temp_diff[1].floatval;

		// Read node logproductin
		ap_uint<32> nid = task_in.object - 4 * nume;
		float logproductin[2];
		union IntFloat temp_node_logproductin[2];
		temp_node_logproductin[0].intval = l1[base_node_logproductins + (2 * nid)];
		temp_node_logproductin[1].intval = l1[base_node_logproductins + (2 * nid) + 1];
		logproductin[0] = temp_node_logproductin[0].floatval;
		logproductin[1] = temp_node_logproductin[1].floatval;

		// Update node logproductin
		float new_logproductin[2];
		new_logproductin[0] = logproductin[0] + diff[0];
		new_logproductin[1] = logproductin[1] + diff[1];
		union IntFloat temp_new_logproductin[2];
		temp_new_logproductin[0].floatval = new_logproductin[0];
		temp_new_logproductin[1].floatval = new_logproductin[1];

		undo_log_t ulog;
		ulog.addr = (base_node_logproductins + (2 * nid)) << 2;
		ulog.data = temp_node_logproductin[0].intval;
		undo_log_entry->write(ulog);

		ulog.addr = ((base_node_logproductins + (2 * nid)) + 1) << 2;
		ulog.data = temp_node_logproductin[1].intval;
		undo_log_entry->write(ulog);

		l1[base_node_logproductins + (2 * nid)] = temp_new_logproductin[0].intval;
		l1[base_node_logproductins + (2 * nid) + 1] = temp_new_logproductin[1].intval;

		// For each affected node, enqueue READ_REVERSE_MESSAGE_TASK
		ap_uint<32> mid = in_args.unpacked.arg2;

		ap_uint<32> CSR_position = l1[base_edge_indices + nid];
		ap_uint<32> CSR_end = l1[base_edge_indices + nid + 1];
		ap_uint<32> CSC_position = l1[base_reverse_edge_indices + nid];
		ap_uint<32> CSC_end = l1[base_reverse_edge_indices + nid + 1];
		ap_uint<32> affected_mid = 0;

		ap_uint<32> reverse_mid;

		if (mid % 2 == 0) {
			reverse_mid = mid + 1;
		} else {
			reverse_mid = mid - 1;
		}

		while ((CSR_position < CSR_end) || (CSC_position < CSC_end)) {
			if (CSR_position < CSR_end) {
				affected_mid = CSR_position * 2;
				CSR_position++;
			} else {
				affected_mid = l1[base_reverse_edge_id + CSC_position] * 2 + 1;
				CSC_position++;
			}
			if (affected_mid != reverse_mid) {
				// Find reverse message id of affected message
				ap_uint<32> reverse_affected_mid;
				if (affected_mid % 2 == 0) {
					reverse_affected_mid = affected_mid + 1;
				} else {
					reverse_affected_mid = affected_mid - 1;
				}

				// Enqueue READ_REVERSE_MESSAGE_TASK
				args_t out_args(affected_mid,0,0,0);
				//out_args.unpacked.arg0 = affected_mid;

				task_t task_out_temp;
				task_out_temp.ts = task_in.ts + 1;
				task_out_temp.object = reverse_affected_mid;
				task_out_temp.ttype = READ_REVERSE_MESSAGE_TASK;
				task_out_temp.args = out_args.packed;
				//task_out_temp.no_write = 1;

				task_out->write(task_out_temp);
			}
		}

	}
}












# /*******************************************************************************
#  Copyright (C) 2024 Marius Meyer
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
# *******************************************************************************/

#include <cclo_benchmark.h>

//hls-synthesizable function performing
//an elementwise increment on fp32 data in src 
//followed by a put to another rank, then
//writing result in dst
void ccol_benchmark(
    int benchmark_type,
    int repetitions,
    //parameters pertaining to CCLO config
    ap_uint<32> src_addr,
    ap_uint<32> comm_adr, 
    ap_uint<32> dpcfg_adr,
    //streams to and from CCLO
    STREAM<command_word> &cmd_to_cclo,
    STREAM<command_word> &sts_from_cclo,
    STREAM<stream_word> &data_to_cclo,
    STREAM<stream_word> &data_from_cclo
){
#pragma HLS INTERFACE s_axilite port=benchmark_type
#pragma HLS INTERFACE s_axilite port=repetitions
#pragma HLS INTERFACE s_axilite port=comm_adr
#pragma HLS INTERFACE s_axilite port=dpcfg_adr
#pragma HLS INTERFACE axis port=cmd_to_cclo
#pragma HLS INTERFACE axis port=sts_from_cclo
#pragma HLS INTERFACE axis port=data_to_cclo
#pragma HLS INTERFACE axis port=data_from_cclo
#pragma HLS INTERFACE s_axilite port=return
    //set up interfaces
    accl_hls::ACCLCommand accl(cmd_to_cclo, sts_from_cclo, comm_adr, dpcfg_adr, 0, 3);
    accl_hls::ACCLData data(data_to_cclo, data_from_cclo);
    //select benchmark type depending on input parameter
    // invalid accl command: will result in NOP and return OK
    int accl_op = 100;
    int sflags = 3;
    switch (benchmark_type) {
        case BENCHMARK_TYPE_STREAM: 
            accl_op = ACCL_COPY;
            sflags = 3;
            break;
        case BENCHMARK_TYPE_MEM: 
            accl_op = ACCL_COPY; 
            sflags = 2;
            break;   
        default: break; 
    }

    for (ap_uint<32> i= 0; i < repetitions; i++) {
        // Push data into the data stream if required
        if (benchmark_type == BENCHMARK_TYPE_STREAM) {
            ap_uint<512> tmpword;
            for(int i=0; i<16; i++){
                float inc = i;
                tmpword((i+1)*32-1, i*32) = *reinterpret_cast<ap_uint<32>*>(&inc);
            }
            //send the vector to cclo
            data.push(tmpword, 0);
        }
        // Schedule the command
        {
            #pragma HLS protocol fixed
            accl.start_call(
                accl_op, 16, 0, 0, 0, 0, 
                dpcfg_adr, 0, sflags, 
                src_addr, 0, 0
            );
            accl.finalize_call();
        }
        // Read back data from the stream if required
        if (benchmark_type != BENCHMARK_TYPE_NOP) {
            ap_uint<512> d = data.pull().data;
        }
    }
}
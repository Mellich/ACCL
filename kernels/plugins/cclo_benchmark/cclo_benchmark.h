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

#include <accl_hls.h>

static int BENCHMARK_TYPE_NOP = 0;
static int BENCHMARK_TYPE_STREAM = 1;
static int BENCHMARK_TYPE_MEM = 2;

void cclo_benchmark(
    int benchmark_type,
    int repetitions,
    //parameters pertaining to CCLO config
    ap_uint<32> comm_adr, 
    ap_uint<32> dpcfg_adr,
    //streams to and from CCLO
    STREAM<command_word> &cmd_to_cclo,
    STREAM<command_word> &sts_from_cclo,
    STREAM<stream_word> &data_to_cclo,
    STREAM<stream_word> &data_from_cclo
);
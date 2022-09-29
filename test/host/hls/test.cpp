/*******************************************************************************
#  Copyright (C) 2022 Xilinx, Inc
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
#
*******************************************************************************/

#include "accl.hpp"
#include <cstdlib>
#include <functional>
#include <mpi.h>
#include <random>
#include <sstream>
#include <tclap/CmdLine.h>
#include <vector>
#include "vadd_put.h"
#include "cclo_bfm.h"
#include <xrt/xrt_device.h>
#include <iostream>

#include "Simulation.h"

using namespace ACCL;

int rank, size;

struct options_t {
    int start_port;
    unsigned int rxbuf_size;
    unsigned int count;
    unsigned int dest;
    unsigned int nruns;
    unsigned int device_index;
    bool udp;
};

void loopback_test(int count, hlslib::Stream<stream_word>& to_cclo, hlslib::Stream<stream_word>& from_cclo) {
    for (int i=0; i < count/16; i++) {
        auto in = from_cclo.read();
        to_cclo.write(in);
    }
}

void run_test_stream_id(ACCL::ACCL& accl, options_t options) {

    //run test here:
    //initialize a CCLO BFM and streams as needed
    hlslib::Stream<command_word> callreq, callack;
    hlslib::Stream<stream_word> data_cclo2krnl("cclo2krnl"), data_krnl2cclo("krnl2cclo");
    std::vector<unsigned int> dest = {9};
    CCLO_BFM cclo(options.start_port, rank, size, dest, callreq, callack, data_cclo2krnl, data_krnl2cclo);
    cclo.run();
    std::cout << "CCLO BFM started" << std::endl;
    MPI_Barrier(MPI_COMM_WORLD);

    //allocate float arrays for the HLS function to use
    auto src_buffer = accl.create_buffer<int>(options.count, ACCL::dataType::int32, 0);
    auto dst_buffer = accl.create_buffer<int>(options.count, ACCL::dataType::int32, 0);
    for(int i=0; i<options.count; i++){
        src_buffer->buffer()[i] = rank;
        dst_buffer->buffer()[i] = 0;
    }

    int init_rank = ((rank - 1 + size) % size);
    int loopback_rank = ((rank + 1 ) % size);

    std::cout << "Send to " << loopback_rank << std::endl;
    accl.send(*src_buffer, options.count, 
            loopback_rank, 9);
    HLSLIB_DATAFLOW_INIT()
    //run the hls function, using the global communicator
    std::cout << "Loopback " << std::endl;
    HLSLIB_DATAFLOW_FUNCTION(loopback_test, options.count, data_krnl2cclo, data_cclo2krnl);
    std::cout << "Recv from " << init_rank << std::endl;
    accl.recv(*src_buffer, options.count, 
            init_rank, 9, ACCL::GLOBAL_COMM, false, ACCL::streamFlags::RES_STREAM);
    HLSLIB_DATAFLOW_FINALIZE()
    std::cout << "Send to " << init_rank << std::endl;
    accl.send(*src_buffer, options.count, 
            init_rank, 9, ACCL::GLOBAL_COMM, false, ACCL::streamFlags::OP0_STREAM);
    std::cout << "Recv from " << loopback_rank << std::endl;
    accl.recv(*dst_buffer, options.count, 
            loopback_rank, 9);
    //check HLS function outputs
    unsigned int err_count = 0;
    for(int i=0; i<options.count; i++){
        err_count += (dst_buffer->buffer()[i] != rank);
    }
    std::cout << "Test finished with " << err_count << " errors" << std::endl;
    //clean up
    cclo.stop();
}

void run_test(ACCL::ACCL& accl, options_t options) {

    //run test here:
    //initialize a CCLO BFM and streams as needed
    hlslib::Stream<command_word> callreq, callack;
    hlslib::Stream<stream_word> data_cclo2krnl, data_krnl2cclo;
    std::vector<unsigned int> dest = {9};
    CCLO_BFM cclo(options.start_port, rank, size, dest, callreq, callack, data_cclo2krnl, data_krnl2cclo);
    cclo.run();
    std::cout << "CCLO BFM started" << std::endl;
    MPI_Barrier(MPI_COMM_WORLD);

    //allocate float arrays for the HLS function to use
    float src[options.count], dst[options.count];
    for(int i=0; i<options.count; i++){
        src[i] = 0;
    }
    //run the hls function, using the global communicator
    vadd_put(   src, dst, options.count, 
                (rank+1)%size,
                accl.get_communicator_addr(), 
                accl.get_arithmetic_config_addr({dataType::float32, dataType::float32}), 
                callreq, callack, 
                data_krnl2cclo, data_cclo2krnl);
    //check HLS function outputs
    unsigned int err_count = 0;
    for(int i=0; i<options.count; i++){
        err_count += (dst[i] != 1);
    }
    std::cout << "Test finished with " << err_count << " errors" << std::endl;
    //clean up
    cclo.stop();
}


options_t parse_options(int argc, char *argv[]) {
    TCLAP::CmdLine cmd("Test ACCL C++ driver");
    TCLAP::ValueArg<unsigned int> nruns_arg("n", "nruns",
                                            "How many times to run each test",
                                            false, 1, "positive integer");
    cmd.add(nruns_arg);
    TCLAP::ValueArg<uint16_t> start_port_arg(
        "s", "start-port", "Start of range of ports usable for sim", false, 5500,
        "positive integer");
    cmd.add(start_port_arg);
    TCLAP::ValueArg<uint32_t> count_arg("c", "count", "How many bytes per buffer",
                                        false, 16, "positive integer");
    cmd.add(count_arg);
    TCLAP::ValueArg<uint32_t> bufsize_arg("b", "rxbuf-size",
                                            "How many KB per RX buffer", false, 1,
                                            "positive integer");
    cmd.add(bufsize_arg);
    TCLAP::SwitchArg udp_arg("u", "udp", "Use UDP backend", cmd, false);

    try {
        cmd.parse(argc, argv);
    } catch (std::exception &e) {
        if (rank == 0) {
        std::cout << "Error: " << e.what() << std::endl;
        }

        MPI_Finalize();
        exit(1);
    }

    options_t opts;
    opts.start_port = start_port_arg.getValue();
    opts.count = count_arg.getValue();
    opts.rxbuf_size = bufsize_arg.getValue() * 1024; // convert to bytes
    opts.nruns = nruns_arg.getValue();
    opts.udp = udp_arg.getValue();
    return opts;
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    options_t options = parse_options(argc, argv);

    std::ostringstream stream;
    stream << "rank " << rank << " size " << size << std::endl;
    std::cout << stream.str();

    std::vector<rank_t> ranks = {};
    for (int i = 0; i < size; ++i) {
        rank_t new_rank = {"127.0.0.1", options.start_port + i, i, options.rxbuf_size};
        ranks.emplace_back(new_rank);
    }

    std::unique_ptr<ACCL::ACCL> accl;
    accl = std::make_unique<ACCL::ACCL>(ranks, rank, options.start_port,
                                            options.udp ? networkProtocol::UDP : networkProtocol::TCP, 16,
                                            options.rxbuf_size);

    accl->set_timeout(1e8);
    std::cout << "Host-side CCLO initialization finished" << std::endl;
    MPI_Barrier(MPI_COMM_WORLD);

    run_test(*accl, options);
    MPI_Barrier(MPI_COMM_WORLD);

    run_test_stream_id(*accl, options);

    MPI_Finalize();
    return 0;
}

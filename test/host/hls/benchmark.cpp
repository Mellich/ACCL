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

#include <accl.hpp>
#include <accl_network_utils.hpp>
#include <cstdlib>
#include <functional>
#include <mpi.h>
#include <random>
#include <sstream>
#include <tclap/CmdLine.h>
#include <vector>
#include <xrt/xrt_device.h>
#include <iostream>
#include <chrono>

#include "cclo_benchmark.h"
#include "cclo_bfm.h"

using namespace ACCL;
using namespace accl_network_utils;

int rank, size;

struct options_t {
    int start_port;
    unsigned int rxbuf_size;
    unsigned int segment_size;
    unsigned int count;
    unsigned int dest;
    unsigned int nruns;
    unsigned int device_index;
    bool udp;
    bool hardware;
    bool rsfec;
    std::string xclbin;
    std::string config_file;
};

void test_benchmark(ACCL::ACCL &accl, xrt::device &device, options_t options, int operation) {
    //run test here:

    //allocate float arrays for the HLS function to use
    float src[16*options.count];
    for(int i=0; i<options.count*16; i++){
        src[i] = 1.0*(options.count*rank+i);
    }
    double time;
    //need to use XRT API because vadd kernel might use different HBM banks than ACCL
    auto src_bo = accl.create_buffer<float>(src, 16*options.count,ACCL::dataType::float32);
    src_bo->sync_to_device();

    if (options.hardware) {
        auto bm_ip = xrt::kernel(device, device.get_xclbin_uuid(), "cclo_benchmark:{bench}",
                        xrt::kernel::cu_access_mode::exclusive);
        auto t1 = std::chrono::high_resolution_clock::now();
        auto run = bm_ip(operation, options.nruns, options.count, src_bo->physical_address(), accl.get_communicator_addr(),
                    accl.get_arithmetic_config_addr({dataType::float32, dataType::float32}));
        run.wait(10000);
        auto t2 = std::chrono::high_resolution_clock::now();
        time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();

    } else {
        //initialize a CCLO BFM and streams as needed
        hlslib::Stream<command_word> callreq("cmd"), callack("ack");
        hlslib::Stream<stream_word> data_cclo2krnl("c2k"), data_krnl2cclo("k2c");
        std::vector<unsigned int> dest = {0};
        CCLO_BFM cclo(options.start_port, rank, size, dest, callreq, callack, data_cclo2krnl, data_krnl2cclo);
        cclo.run();
        std::cout << "CCLO BFM started" << std::endl;
        MPI_Barrier(MPI_COMM_WORLD);

        auto t1 = std::chrono::high_resolution_clock::now();
        //run the hls function, using the global communicator
        cclo_benchmark(operation, options.nruns, options.count, src_bo->physical_address(),
                    accl.get_communicator_addr(),
                    accl.get_arithmetic_config_addr({dataType::float32, dataType::float32}),
                    callreq, callack,
                    data_krnl2cclo, data_cclo2krnl);
        auto t2 = std::chrono::high_resolution_clock::now();
        time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        //stop the BFM
        cclo.stop();
    }

    std::cout << "Measured time [µs]: " << time << std::endl;
    std::cout << "Measured time per ACCL call [µs]: " << (time / options.nruns) << std::endl;

    // //check HLS function outputs
    // unsigned int err_count = 0;
    // for(int i=0; i<options.count; i++){
    //     float expected = 1.0*(options.count*((rank+size-1)%size)+i) + 1;
    //     if(dst[i] != expected){
    //         err_count++;
    //         std::cout << "Mismatch at [" << i << "]: got " << dst[i] << " vs expected " << expected << std::endl;
    //     }
    // }

    // std::cout << "Test finished with " << err_count << " errors" << std::endl;
    MPI_Barrier(MPI_COMM_WORLD);
}

options_t parse_options(int argc, char *argv[]) {
    TCLAP::CmdLine cmd("Test ACCL C++ driver");
    TCLAP::ValueArg<unsigned int> nruns_arg("n", "nruns",
                                            "How many times to run each test",
                                            false, 1, "positive integer");
    cmd.add(nruns_arg);
    TCLAP::ValueArg<uint16_t> start_port_arg(
        "p", "start-port", "Start of range of ports usable for sim", false, 5500,
        "positive integer");
    cmd.add(start_port_arg);
    TCLAP::ValueArg<uint32_t> count_arg("s", "count", "How many bytes per buffer",
                                        false, 16, "positive integer");
    cmd.add(count_arg);
    TCLAP::ValueArg<uint32_t> bufsize_arg("b", "rxbuf-size",
                                            "How many KB per RX buffer", false, 1,
                                            "positive integer");
    cmd.add(bufsize_arg);
    TCLAP::SwitchArg udp_arg("u", "udp", "Use UDP backend", cmd, false);
    TCLAP::SwitchArg hardware_arg("f", "hardware", "enable hardware mode", cmd, false);
    TCLAP::ValueArg<std::string> xclbin_arg(
        "x", "xclbin", "xclbin of accl driver if hardware mode is used", false,
        "accl.xclbin", "file");
    cmd.add(xclbin_arg);
    TCLAP::ValueArg<uint16_t> device_index_arg(
        "i", "device-index", "device index of FPGA if hardware mode is used",
        false, 0, "positive integer");
    cmd.add(device_index_arg);
    TCLAP::ValueArg<std::string> config_arg(
        "c", "config", "Config file containing IP mapping",
        false, "", "JSON file");
    TCLAP::SwitchArg rsfec_arg("", "rsfec", "Enables RS-FEC in CMAC.", cmd,
                               false);
    cmd.add(config_arg);

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
    opts.segment_size = opts.rxbuf_size;
    opts.nruns = nruns_arg.getValue();
    opts.udp = udp_arg.getValue();
    opts.hardware = hardware_arg.getValue();
    opts.xclbin = xclbin_arg.getValue();
    opts.device_index = device_index_arg.getValue();
    opts.config_file = config_arg.getValue();
    opts.rsfec = rsfec_arg.getValue();
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

    acclDesign design = acclDesign::AXIS3x;
    std::vector<rank_t> ranks;
    if (options.config_file == "") {
        ranks = generate_ranks(true, rank, size, options.start_port,
                               options.rxbuf_size);
    } else {
        ranks = generate_ranks(options.config_file, rank, options.start_port,
                               options.rxbuf_size);
    }

    xrt::device device{};
    if (options.hardware) {
        device = xrt::device(options.device_index);
    }

    std::unique_ptr<ACCL::ACCL> accl = initialize_accl(
        ranks, rank, !options.hardware, design, device, options.xclbin, 16,
        options.rxbuf_size, options.segment_size, options.rsfec);

    accl->set_timeout(1e6);

    MPI_Barrier(MPI_COMM_WORLD);
    test_benchmark(*accl, device, options, BENCHMARK_TYPE_NOP);
    test_benchmark(*accl, device, options, BENCHMARK_TYPE_MEM);
    test_benchmark(*accl, device, options, BENCHMARK_TYPE_STREAM);

    MPI_Finalize();
    return 0;
}

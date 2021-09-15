/*******************************************************************************
#  Copyright (C) 2021 Xilinx, Inc
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

#include "timing.hpp"
#include "accl-device.hpp"

#include <mpi.h>
#include <vector>

void check_usage(int argc, char *argv[]) {
  if (argc < 4) {
    std::cerr << "Usage: " << argv[0]
              << " <bitstream> <device_idx> <bank_id>" << std::endl;
    exit(-1);
  }
}

int main(int argc, char *argv[]) {

  MPI_Init(&argc, &argv);

  check_usage(argc, argv);

  const std::string bitstream_f = argv[1];
  const auto device_idx = atoi(argv[2]);
  const auto nbufs = atoi(argv[3]);
  const auto mode = atoi(argv[4]);

  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  std::cout << "Rank " << rank << std::endl;
  std::cout << "Bitstream " << bitstream_f << std::endl;
  std::cout << "Buffer count " << nbufs << std::endl;
//  std::cout << "Mode " << mode << std::endl;

  // Setup
  Timer t_construct, t_bitstream, t_read_reg, t_write_reg, t_execute_kernel,
      t_preprxbuffers, t_dump_rx_buffers, t_config_comm, t_dump_comm;
  accl_operation_t op = nop;

  const int buffer_size = nbufs * 1024;

  std::cerr << "Construct ACCL" << std::endl;
  t_construct.start();
  ACCL f(nbufs, buffer_size, device_idx, rank, size, DUAL);
  t_construct.end();

  std::cerr << "Load bitstream" << std::endl;
  t_bitstream.start();
  f.load_bitstream(bitstream_f);
  t_bitstream.end();

  std::cerr << "Prep RX buffers" << std::endl;
  t_preprxbuffers.start();
  f.prep_rx_buffers();
  t_preprxbuffers.end();
  
  std::cerr << "Dump RX buffers" << std::endl;
  t_dump_rx_buffers.start();
  f.dump_rx_buffers();
  t_dump_rx_buffers.end();
  
  std::cerr << "Config communicator" << std::endl;
  t_config_comm.start();
  f.config_comm();
  t_config_comm.end();
  
  std::cerr << "Dump communicator" << std::endl;
  t_dump_comm.start();
  f.get_comm().dump_communicator();
  t_dump_comm.end();


  std::cerr << "NOP" << std::endl;
  t_execute_kernel.start();
  f.nop_op();
  std::cout << "Return code: " << f.get_retcode() << std::endl;
  t_execute_kernel.end();
  
  std::cout << "t_construct: " << t_construct.elapsed() << " usecs"
            << std::endl;
  std::cout << "t_bitstream: " << t_bitstream.elapsed() << " usecs"
            << std::endl;
  std::cout << "t_config_comm: " << t_config_comm.elapsed() << " usecs"
            << std::endl;
  std::cout << "t_preprxbuffers: " << t_preprxbuffers.elapsed() << " usecs"
            << std::endl;
  std::cout << "t_dump_rx_buffers: " << t_dump_rx_buffers.elapsed() << " usecs"
            << std::endl;
  std::cout << "t_execute_kernel: " << t_execute_kernel.elapsed() << " usecs"
            << std::endl;
  std::cout << "HWID: " << hex << f.get_hwid() << std::dec << std::endl;
  MPI_Finalize();
  return 0;
}

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
#
*******************************************************************************/

#include "accl-device.hpp"


/*
def test_self_sendrecv():
    print("========================================")
    print("Self Send/Recv ")
    print("========================================")
    for j in range(args.naccel):
        src_rank = j
        dst_rank = j
        tag      = 5+10*j
        senddata = np.random.randint(100, size=tx_buf[src_rank].shape)
        tx_buf[src_rank][:]=senddata
        cclo_inst[src_rank].send(0, tx_buf[src_rank], dst_rank, tag)

        cclo_inst[dst_rank].recv(0, rx_buf[dst_rank], src_rank, tag)

        recvdata = rx_buf[dst_rank]
        if (recvdata == senddata).all():
            print("Send/Recv {} -> {} succeeded".format(src_rank, dst_rank))
        else:
            diff = (recvdata != senddata)
            firstdiff = np.argmax(diff)
            ndiffs = diff.sum()
            print("Send/Recv {} -> {} failed, {} bytes different starting at {}".format(src_rank, dst_rank, ndiffs, firstdiff))
            print(f"Senddata: {senddata} != {recvdata} Recvdata ")
            cclo_inst[dst_rank].dump_communicator()
            cclo_inst[src_rank].dump_communicator()
            cclo_inst[dst_rank].dump_rx_buffers_spares()
            import pdb; pdb.set_trace()
*/

#include <vector>
#include <iostream>
#include <algorithm>
#include "accl-device.hpp"
#include "timing.hpp"
using namespace std;


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
	int errors =0;
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


  const int buffer_size = nbufs * 1024;
  const auto SIZE = buffer_size/sizeof(int);
  ACCL f(nbufs, buffer_size, device_idx, rank, size, DUAL);
  f.load_bitstream(bitstream_f);
  f.prep_rx_buffers();
  
  f.config_comm();
	f.dump_rx_buffers();
	const auto naccel =  size;
	for(int j=0; j<naccel; j++) {
		auto src_rank = j;
		auto dst_rank = j;
		auto tag = 5+10*j;

		// Allocate sendddata
		vector<int> senddata(SIZE);
		vector<int> recvdata(SIZE);	
		fill(senddata.begin(), senddata.end(), 0x5ca1ab1e);
		f.copy_to_buffer(senddata, j);
		f.send(0, f.buffer_at(j), j, tag);
		f.recv(0, f.buffer_at(j), j, tag);

		if(senddata!=recvdata) {
			errors++;
		}
		for(int i=0; i< 10; i++) {
			cout << hex << senddata.data()[i] << " " << recvdata.data()[i] << endl;
		}
	}
	cout << "Errors: " << errors << endl;
	f.dump_rx_buffers();
	f.get_comm().dump_communicator();
	MPI_Finalize();
	return errors;
}

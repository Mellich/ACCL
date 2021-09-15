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

int main() {
	cout << "========================================" << endl;
	cout << "Self Send/Recv" << endl;
	cout << "========================================" << endl;
	for(int j=0; j<naccel; j++) {
		auto src_rank = j;
		auto dst_rank = j;
		auto tag = 5+10*j;
	}
	return 0;
}

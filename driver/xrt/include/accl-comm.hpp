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

#pragma once

#include <iostream>
#include <map>

#include "experimental/xrt_aie.h"
#include "experimental/xrt_device.h"
#include "experimental/xrt_kernel.h"
#include <arpa/inet.h>
#include <mpi.h>
#include <cassert>
#include "accl-consts.hpp"
#include "accl-util.hpp"

using namespace std;

class communicator {

	private:
  struct comm_data {
	int32_t local_rank;
	int32_t addr;
	int32_t ranks;
	int32_t session_addr;
	vector<int32_t> inbound_seq_number_addr;	
	vector<int32_t> outbound_seq_number_addr;	
	};
    struct sockaddr_in sa;
	char str[INET_ADDRSTRLEN];
  string base_ipaddr = "197.11.27.";
  int start_ip = 1;
  int _world_size;
  int _local_rank;
  int _rank;
  bool _vnx;
  map<int, string> rank_to_ip;
  int32_t _comm_addr;
  int _comm_count = 0;
  xrt::kernel _krnl[1];
  vector<comm_data> _cd;

public:
  communicator() {}

  communicator(const int world_size, const int local_rank, const int rank, const int32_t comm_addr, xrt::kernel krnl, 
               bool vnx = false)
      : _world_size(world_size), _local_rank(local_rank), _rank(rank), _comm_addr(comm_addr), _vnx(vnx) {
	_krnl[0] = krnl;		
    uint32_t addr = _comm_addr;
	assert(_comm_addr !=0);  
  // communicator = {"local_rank": local_rank, "addr": comm_address, "ranks": ranks, "inbound_seq_number_addr":[0 for _ in ranks], "outbound_seq_number_addr":[0 for _ in ranks], "session_addr":[0 for _ in ranks]}
	comm_data cd = {.local_rank = _local_rank, .addr = _comm_addr, .ranks = _world_size};
	
	mmio_write(_krnl[0], addr, _world_size);
    addr += 4;
    mmio_write(_krnl[0], addr, _local_rank);
    for (int i = 0; i < _world_size; i++) {
      string ip = base_ipaddr + to_string(i + start_ip);
      rank_to_ip.insert(pair<int, string>(_rank, ip));
      addr += 4;
      mmio_write(_krnl[0], addr, ip_encode(ip));
      addr += 4;
      if (_vnx) {
        mmio_write(_krnl[0], addr, i);
      } else {
        mmio_write(_krnl[0], addr, port_from_rank(i));
      }
		addr += 4;
        mmio_write(_krnl[0], addr, 0);
        cd.inbound_seq_number_addr.push_back(addr);
		addr += 4;
		mmio_write(_krnl[0], addr, 0);
        cd.outbound_seq_number_addr.push_back(addr);
  		addr += 4;
		mmio_write(_krnl[0], addr, 0xFFFFFFFF);
  		cd.session_addr = addr;	
	}
	_cd.push_back(cd); 
}

  int32_t port_from_rank(int rank) {
    return 16; // XXX What is port?
  }

  unsigned long ip_encode(string ip) { 
	inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr));
	return sa.sin_addr.s_addr; 
  }
  
  string ip_decode(auto ip) { 
	inet_ntop(AF_INET, &(sa.sin_addr), str, INET_ADDRSTRLEN);	
	return string(str);
  }

  string ip_from_rank(int rank) { return rank_to_ip[rank]; }

	void dump_communicator() {
		uint32_t _addr;
		if(_cd.size()==0) {
			_addr = _comm_addr;
		} else {
			_addr = _comm_addr; //_cd.back().addr - EXCHANGE_MEM_OFFSET_ADDRESS;
		}
		int nr_ranks =  mmio_read(_krnl[0], _addr);
		_addr +=4;
		int local_ranks = mmio_read(_krnl[0], _addr);
		cout << "Communicator: local_rank: " << _local_rank << " number of ranks: " << _world_size << endl;
		for(int i =0; i<nr_ranks; i++) {
			_addr+=4;
			int32_t ip_addr_rank = mmio_read(_krnl[0], _addr);
			_addr+=4;
			auto port  = mmio_read(_krnl[0], _addr);
			_addr+=4;
			auto inbound_seq_number  = mmio_read(_krnl[0], _addr);
			_addr+=4;
			auto outbound_seq_number  = mmio_read(_krnl[0], _addr);
			_addr+=4;
			auto session  = mmio_read(_krnl[0], _addr);	
			cout << i << " " << ip_decode(ip_addr_rank) << " " << port << " " << inbound_seq_number << " " << outbound_seq_number << endl;
		}
	}


/*
    def dump_communicator(self):
        if len(self.communicators) == 0:
            addr    = self.communicators_addr
        else:
            addr    = self.communicators[-1]["addr"] - EXCHANGE_MEM_OFFSET_ADDRESS
        nr_ranks    = self.exchange_mem.read(addr)
        addr +=4
        local_rank  = self.exchange_mem.read(addr)
        print(f"Communicator. local_rank: {local_rank} \t number of ranks: {nr_ranks}.")
        for i in range(nr_ranks):
            addr +=4

*/

};

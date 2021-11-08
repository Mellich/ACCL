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

#include "accl-comm.hpp"
#include "accl-consts.hpp"
#include "accl-util.hpp"

#include "experimental/xrt_aie.h"
#include "experimental/xrt_device.h"
#include "experimental/xrt_kernel.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <uuid/uuid.h>
#include <vector>
#include <sstream>      // std::stringstream


const bool compatible_size(size_t nbytes, accl_reduce_func type) {
  if (type == fp || type == i32) {
    return (nbytes % 4) == 0 ? true : false;
  } else if (type == dp || type == i64) {
    return (nbytes % 8) == 0 ? true : false;
  }
  return false;
}

enum network_protocol_t { TCP, UDP, ROCE };

enum mode { DUAL = 2, QUAD = 4 };

class ACCL {

private:
  std::vector<std::vector<int8_t>> _rx_host_bufs;
  int _segment_size;
  int _nbufs;
  int32_t _rx_buffers_adr;
  int _rx_buffer_size;
  std::vector<xrt::bo> _rx_buffer_spares;
  xrt::bo _utility_spare; 
  xrt::device _device;
  xrt::kernel _krnl[1];
  communicator _comm;
  const int32_t _base_addr = HOST_CTRL_ADDRESS_RANGE;
  int32_t _comm_addr = 0;
  enum mode _mode;
  int _rank;
  int _size;
  string _local_rank_string;
  int _local_rank;

public:
  ACCL(unsigned int idx = 1) { get_xilinx_device(idx); }

  ACCL(int nbufs=16, int buffersize=1024, unsigned int idx=1, int rank=1, int size=1, enum mode m=DUAL)
      : _nbufs(nbufs), _rx_host_bufs(nbufs), _rx_buffer_spares(nbufs),
        _rx_buffer_size(buffersize), _rank(rank), _size(size), _mode(m) {
    	get_xilinx_device(idx);
    	_local_rank_string = string(getenv("OMPI_COMM_WORLD_LOCAL_RANK"));
    	_local_rank = stoi(_local_rank_string);
 }

  template <typename... Args> auto execute_kernel(Args... args) {
    auto run = _krnl[0](args...);
	run.wait();
	return run;
  }
 
 ~ACCL() {
    std::cout << "Removing CCLO object at " << std::hex << get_mmio_addr() << dec
              << std::endl;
    execute_kernel(config, 1, 0, 0, reset_periph, 0, 0, 0, 0, DUMMY_ADDR, DUMMY_ADDR, DUMMY_ADDR);
  }

  /*
   *      Get the first or idx Xilinx device
   */
  void get_xilinx_device(unsigned int idx=0) {
    _device = xrt::device(idx);
    // Do some checks to see if we're on the U280, error otherwise
    //	std::string name = _device.get_info<xrt::info::device::name>();
    //	if(name.find("u280") == std::string::npos) {
    //		std::cerr << "Device selected is not a U280." << std::endl;
    //		exit(-1);
    //	}
  }

  void config_comm() { _comm = {_size, _local_rank, _rank, _comm_addr, _krnl[0], false}; }
//int world_size, int local_rank, int rank, uint64_t comm_addr, xrt::kernel krnl, bool vnx = false
  void load_bitstream(const std::string xclbin) {
    auto uuid = _device.load_xclbin(xclbin.c_str());	
	MPI_Barrier(MPI_COMM_WORLD);
	_krnl[0] = xrt::kernel(
        _device, uuid,
        "ccl_offload:ccl_offload_"+string{_local_rank_string},
        xrt::kernel::cu_access_mode::exclusive);
	MPI_Barrier(MPI_COMM_WORLD);
	write_reg(_krnl[0], 0x18, 0xdeadbeef);
	auto run  = _krnl[0](0xdeadbeef,0xdeadbeef,0xdeadbeef,0xdeadbeef,0xdeadbeef,0xdeadbeef,0xdeadbeef,0xdeadbeef,0xdeadbeef,0xdeadbeef,0xdeadbeef,0xdeadbeef);
    run.wait();
	cout << "RAN KERNEL" << endl;			
    
  }

  template <typename T>
  void copy_to_buffer(vector<T> input, int j) {
      auto hostmap = _rx_buffer_spares[j].map<int32_t *>();
	  for(int i=0; i< input.size(); i++) {
		hostmap[i] = input.data()[i];
	  }
	 _rx_buffer_spares[j].sync(XCL_BO_SYNC_BO_TO_DEVICE);
  }

  communicator get_comm() {
		return _comm;
	}

  xrt::bo buffer_at(const int idx) {
		if(idx>_nbufs) {
			return NULL;
		}
		return _rx_buffer_spares[idx];
  }

	void dump_exchange_memory() {
        std::cout << "Exchange mem: "<< std::endl;
        const int num_word_per_line=4;
        for(int i =0; i< 116; i+=4){
			cout << read_reg(_krnl[0], i)  <<endl;
		}
	}


  void dump_rx_buffers() {
    uint32_t addr = 0; //_base_addr;
    std::cout << "Dumping nbufs: " <<  mmio_read(_krnl[0], addr) << std::endl;
	for (int i = 0; i < _nbufs; i++) {
      uint32_t res;
      addr += 4;
      const auto addrl = mmio_read(_krnl[0], addr);

      addr += 4;
      const auto addrh = mmio_read(_krnl[0], addr);

      addr += 4;
      const auto maxsize = mmio_read(_krnl[0], addr);

      addr += 4;
      const auto dmatag = mmio_read(_krnl[0], addr);

      addr += 4;
      const auto rstatus = mmio_read(_krnl[0], addr);

      addr += 4;
      const auto rxtag = mmio_read(_krnl[0], addr);

      addr += 4;
      const auto rxlen = mmio_read(_krnl[0], addr);

      addr += 4;
      const auto rxsrc = mmio_read(_krnl[0], addr);
      
	  addr += 4;
      const auto seq = mmio_read(_krnl[0], addr);
    
		string status;
		if(rstatus == 0) {
                status =  "NOT USED";
		}
            else if (rstatus == 1) {
                status = "ENQUEUED";
			} else if (rstatus == 2) {
                status = "RESERVED";
			} else {
                status = "UNKNOWN";
			}
    string content = "content";
	std::cout << "SPARE RX BUFFER " << i <<":\t ADDR: "<< hex << addrh << " " << addrl <<" \t STATUS: "<< dec << status << " \t OCCUPACY: "<< rxlen/maxsize << " \t DMA TAG: " << hex << dmatag << dec<< " \t  MPI TAG:"<< rxtag <<" \t SEQ: "<< seq <<" \t SRC: "<< rxsrc<<" \t content "<<content << endl;
	}
  }

void set_dma_transaction_size_param(const auto value=0) {
        if(value % 8 != 0) {
            cerr << "ACCL: dma transaction must be divisible by 8 to use reduce collectives" << endl;

	} else if(value > _rx_buffer_size) {
            cerr << "ACCL: transaction size should be less or equal to configured buffer size!" << endl;
            return;
	}
     execute_kernel(config, value, 0, 0, set_dma_transaction_size, TAG_ANY, 0, 0, 0, DUMMY_ADDR, DUMMY_ADDR, DUMMY_ADDR);


        _segment_size = value;
        cout << "time taken to start and stop timer " <<  mmio_read(_krnl[0], 0x0FF4) << endl;
}

const int32_t get_mmio_addr() {
    return mmio_read(_krnl[0], _base_addr);
}


void set_max_dma_transaction_param(const auto value=0) {
        if(value > 20) {
            cerr  << "ACCL: transaction size should be less or equal to configured buffer size!" << endl;
            return;
	}
    execute_kernel(config, value, 0, 0, set_max_dma_transactions, TAG_ANY, 0, 0, 0, DUMMY_ADDR, DUMMY_ADDR, DUMMY_ADDR);
}

  void prep_rx_buffers() {
    const auto SIZE = _rx_buffer_size / sizeof(uint32_t);
    uint32_t addr = 0; 
    mmio_write(_krnl[0], addr, _nbufs);
    for (int i = 0; i < _nbufs; i++) {
      // Alloc and fill buffer
      const auto bank_grp_idx = i; //_krnl->group_id(i);
      auto bo = xrt::bo(_device, _rx_buffer_size, bank_grp_idx);
      auto hostmap = bo.map<int32_t *>();
      std::fill(hostmap, hostmap + SIZE, 0);
      bo.sync(XCL_BO_SYNC_BO_TO_DEVICE);
      _rx_buffer_spares.insert(_rx_buffer_spares.cbegin() + i, bo);

      // Write meta data
      addr += 4;
      mmio_write(_krnl[0], addr, bo.address() & 0xffffffff);
      addr += 4;
      mmio_write(_krnl[0], addr, (bo.address() >> 32) & 0xffffffff);

      addr += 4;
      mmio_write(_krnl[0], addr, _rx_buffer_size);

      for (int i = 3; i < 9; i++) {
        addr += 4;
        mmio_write(_krnl[0], addr, 0);
      }
    }
      _comm_addr = addr + 4;
      // Start irq-driven RX buffer scheduler and (de)packetizer
      //execute_kernel(config, 0xdeadbeef); //, 0, 0, enable_irq, TAG_ANY, 0, 0, 0, DUMMY_ADDR, DUMMY_ADDR, DUMMY_ADDR);
	  dump_exchange_memory();
	  cout << "Enable IRQ: " << get_retcode() << endl;
      execute_kernel(config, 1, 0, 0, enable_pkt, TAG_ANY, 0, 0, 0, DUMMY_ADDR, DUMMY_ADDR, DUMMY_ADDR);
	  cout << "Enable PKT: " << get_retcode() << endl;
      cout << "time taken to enqueue buffers "<< mmio_read(_krnl[0], 0x0FF4) << endl;
 
	set_dma_transaction_size_param(_rx_buffer_size);
	set_max_dma_transaction_param(10);

	MPI_Barrier(MPI_COMM_WORLD);
  }



  const int32_t get_retcode() { return mmio_read(_krnl[0], 0xFFC); }

  const int32_t get_hwid() { return mmio_read(_krnl[0], 0xFF8); }

  xrt::run nop_op(const bool run_async = false) { //, waitfor=[]) {
     return execute_kernel(nop, 1, 0, 0, 0, TAG_ANY, 0, 0, 0, DUMMY_ADDR, DUMMY_ADDR, DUMMY_ADDR);
  }

  auto enable_profiling(bool run_async=false) {//, waitfor=[]) {
        auto handle = _krnl[0](config, start_profiling);
        if(!run_async) {
            handle.wait();
		}
	return handle;		
	}

	auto disable_profiling(bool run_async=false) {//, waitfor=[]):
        auto handle = _krnl[0](config, end_profiling);
        if(!run_async) {
            handle.wait();
		}
		return handle;		
	}


	auto send(auto comm_id, auto srcbuf, auto dst, auto tag=TAG_ANY, bool from_fpga=false) {
		if(srcbuf.size()==0) {
			cerr << "Attempt to send 0 size buffer"<<endl;
		}
		if(from_fpga==false) {
			srcbuf.sync(XCL_BO_SYNC_BO_TO_DEVICE);
		}
		auto handle = execute_kernel(sendop, _rx_buffer_size/sizeof(int32_t), comm_id, dst, 0, tag, 0, 0, 0, srcbuf.address(), DUMMY_ADDR, DUMMY_ADDR);
		return handle;
	}

	auto recv(auto comm_id, auto dstbuf, auto src, auto tag=TAG_ANY, bool to_fpga=false, bool run_async=false) {
		if(to_fpga==false && run_async) {
			cerr << "ACCL: async run returns data on FPGA, user must sync_from_device() after waiting" << endl;
		}
		if(dstbuf.size()==0) {
			cerr << "Attempt to recv to 0 size buffer"<<endl;
		}
		auto handle = execute_kernel(recvop, _rx_buffer_size/sizeof(int32_t), comm_id, src, 0, tag, 0,0,0, dstbuf.address(), DUMMY_ADDR, DUMMY_ADDR, DUMMY_ADDR);
		if(run_async==false) {
			return handle;
		} else {
			handle.wait();
		}
		if(to_fpga==false) {
			dstbuf.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
		}
		return handle;
	}
	/*
    @self_check_return_value
    def send(self, comm_id, srcbuf, dst, tag=TAG_ANY, from_fpga=False, run_async=False, waitfor=[]):
        if srcbuf.nbytes == 0:
            warnings.warn("zero size buffer")
            return
        if not from_fpga:
            srcbuf.sync_to_device()
        handle = self.start(scenario=CCLOp.send, len=srcbuf.nbytes, comm=self.communicators[comm_id]["addr"], root_src_dst=dst, tag=tag, addr_0=srcbuf, waitfor=waitfor)
        if run_async:
            return handle 
        else:
            handle.wait()
    
    @self_check_return_value
    def recv(self, comm_id, dstbuf, src, tag=TAG_ANY, to_fpga=False, run_async=False, waitfor=[]):
        if not to_fpga and run_async:
            warnings.warn("ACCL: async run returns data on FPGA, user must sync_from_device() after waiting")
        if dstbuf.nbytes == 0:
            warnings.warn("zero size buffer")
            return
        handle = self.start(scenario=CCLOp.recv, len=dstbuf.nbytes, comm=self.communicators[comm_id]["addr"], root_src_dst=src, tag=tag, addr_0=dstbuf, waitfor=waitfor)
        if run_async:
            return handle
        else:
            handle.wait()
        if not to_fpga:
            dstbuf.sync_from_device()
	*/

};

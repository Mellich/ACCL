#include "xlnx-comm.hpp"
#include "xlnx-consts.hpp"

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

using namespace std;

int main() {
    auto _device = xrt::device(0);
	auto _rx_buffer_size = 4e6;
    auto uuid = _device.load_xclbin("ccl_offload.xclbin");
    auto MAX_LENGTH = _rx_buffer_size;
    auto krnl = xrt::kernel(
        _device, uuid,
        "ccl_offload:{ccl_offload_0}", //+string{local_rank_string},
        xrt::kernel::cu_access_mode::exclusive);
      
    int *host_ptr;      
    posix_memalign((void**)&host_ptr,4096,MAX_LENGTH*sizeof(int));


      // Sample example filling the allocated host memory
      for(int i=0; i<MAX_LENGTH; i++) {
      host_ptr[i] = i;  // whatever
      }
 
      auto mybuf = xrt::bo (_device, host_ptr, MAX_LENGTH*sizeof(int), 0);
      mybuf.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    
    
    
      cout << "RX BUFFER SIZE: " << _rx_buffer_size << endl;
      cout << "SIZE: " << _rx_buffer_size << endl;
}

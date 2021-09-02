#pragma once

#include "experimental/xrt_aie.h"
#include "experimental/xrt_device.h"
#include "experimental/xrt_kernel.h"
#include "accl-consts.hpp"
  
void write_reg(xrt::kernel _krnl, int32_t addr, uint32_t data) {
    _krnl.write_register(addr, data);
}

const int32_t read_reg(xrt::kernel _krnl, int32_t addr) { return _krnl.read_register(addr); }

const int32_t mmio_read(xrt::kernel _krnl, const int32_t addr) {
	read_reg(_krnl, EXCHANGE_MEM_OFFSET_ADDRESS+addr);
}
  
void mmio_write(xrt::kernel _krnl, const int32_t addr, const int32_t data) {
	write_reg(_krnl, EXCHANGE_MEM_OFFSET_ADDRESS+addr, data);
}



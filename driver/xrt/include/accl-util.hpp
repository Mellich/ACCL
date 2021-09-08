#pragma once

#include "experimental/xrt_aie.h"
#include "experimental/xrt_device.h"
#include "experimental/xrt_kernel.h"
#include "accl-consts.hpp"
  
void write_reg(xrt::kernel _krnl, const int32_t addr, const int32_t data) {
    _krnl.write_register(addr, data);
}

const auto read_reg(xrt::kernel _krnl, const auto addr) { return _krnl.read_register(addr); }

const auto mmio_read(xrt::kernel _krnl, const auto addr) {
	return read_reg(_krnl, EXCHANGE_MEM_OFFSET_ADDRESS+addr);
}
  
void mmio_write(xrt::kernel _krnl, const int32_t addr, const int32_t data) {
	write_reg(_krnl, EXCHANGE_MEM_OFFSET_ADDRESS+addr, data);
}



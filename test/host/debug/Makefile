# /*******************************************************************************
#  Copyright (C) 2024 Marius Meyer
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

# Path to to the microblaze used by ACCL in the design
MICROBLAZE_PATH = level0_i/ulp/ccl_offload_0/ccl_offload_bd_i/cclo/control/microblaze_0

# Path to input bitstream
INPUT_XCLBIN ?= ccl_offload.xclbin

# Path to output bitstream. Will be created.
OUTPUT_XCLBIN ?= ccl_offload_updated.xclbin

# Path to vivado project prj.xpr of the input bitstream 
# Usually in the build folder of the bitstream under link/vivado/vpl/prj/prj.xpr
VIVADO_PROJECT ?= prj.xpr

# Firmware binary that is used to update bitstream
NEW_ELF ?= ccl_offload_control.elf

.PHONY: clean all

all: $(OUTPUT_XCLBIN)

bitstream.bit: $(INPUT_XCLBIN)
	xclbinutil --force --dump-section BITSTREAM:RAW:bitstream.bit --input $(INPUT_XCLBIN)

#use Vivado to extract descriptor.mmi file from the project
descriptor.mmi: extract_mmi.tcl
	vivado $(VIVADO_PROJECT) -mode batch -source extract_mmi.tcl -tclargs descriptor.mmi

#update the new executable in the bitstream
bitstream_updated.bit: descriptor.mmi bitstream.bit $(NEW_ELF)
	updatemem -force --meminfo descriptor.mmi --data $(NEW_ELF) --bit bitstream.bit --proc $(MICROBLAZE_PATH) --out bitstream_updated.bit

#write bitstream to xclbin
$(OUTPUT_XCLBIN): bitstream_updated.bit $(INPUT_XCLBIN)
	xclbinutil --force --replace-section BITSTREAM:RAW:bitstream_updated.bit --input $(INPUT_XCLBIN) --o $(OUTPUT_XCLBIN)

#remove temp files
clean:
	rm -rf *.bit
	rm -rf *.mmi
	rm -rf *.log
	rm -rf *.jou
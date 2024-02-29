# /*******************************************************************************
#  Copyright (C) 2021 Xilinx, Inc
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

#Arguments: <old xclbin> <vivado project> <new elf> <new xclbin>
#
#	<old xclbin>: 		Path to the xclbin that is used as basis and contains 
#						an ACCL instance with microblaze
#	<vivado project>: 	Path to the Vivado project (.xpr) of the base bitstream               
#	<new elf>:			New ELF file with firmware for the CCLO
#	<new xclbin>:		Path of the newly created bitstream
#
# Depending on the desing of the used ACCL bitstream, the path to the microblaze
# in the design may have to be adapted. This can be done by setting the
# variable MICROBALZE_PATH in this script.
#
# Changes (Marius Meyer):
#	- Update path to microblaze in updatemem
#	- Use input xclbin as basis for new bitstream
#	- Add description of input parameters
#   - use GNU make for building to reduce build time for repeated builds

make INPUT_XCLBIN=$1 VIVADO_PROJECT=$2 NEW_ELF=$3 OUTPUT_XCLBIN=$4
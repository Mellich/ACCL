#!/bin/bash
#SBATCH -J accl_udp
#SBATCH -p normal
#SBATCH -q fpgasynthesis
#SBATCH -t 1-00:00:00
#SBATCH --cpus-per-task=8
#SBATCH --mem 64g
#SBATCH -o synth_benchmark_%j.out
#SBATCH -e synth_benchmark_%j.out
#SBATCH --mail-type FAIL,END
#SBATCH --mail-user marius.meyer@uni-paderborn.de

module reset
module load fpga xilinx/xrt/2.14 devel/CMake/3.23.1-GCCcore-11.3.0.lua toolchain/gompi/2022b.lua
module load lib/JsonCpp/1.9.4-GCCcore-11.2.0.lua
module load lib/zmqpp/4.2.0-GCCcore-11.2.0.lua
module load devel/Boost/1.81.0-GCC-12.2.0.lua
module load lib/TCLAP/1.2.5-GCCcore-11.2.0.lua
module load lang/Python/3.9.6-GCCcore-11.2.0.lua

export LM_LICENSE_FILE="27000@kiso.uni-paderborn.de"

DATE=$(date +%Y-%m-%d)
SDIR=${DATE}-accl-benchmark

git clone -b utbest-dev-benchmark git@github.com:Mellich/ACCL.git  $SDIR
cd $SDIR
git submodule update --init --recursive
cd test/hardware

make all PLATFORM=xilinx_u280_gen3x16_xdma_1_202211_1 MODE=benchmark
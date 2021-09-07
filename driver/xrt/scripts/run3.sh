#!/bin/bash

mpirun -np 3 ./src/accl_demo ./ccl_offload.xclbin 0 16 0 

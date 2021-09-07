# /*******************************************************************************
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
# *******************************************************************************/

project(xlnx-accl)
cmake_minimum_required(VERSION 3.5)

enable_testing()

set(CMAKE_CXX_STANDARD 14)

# MPI
find_package(MPI)

link_directories($ENV{XILINX_XRT}/lib)
link_libraries(xilinxopencl)
link_libraries(xrt_coreutil)
link_libraries(xrt_core)
include_directories(${MPI_CXX_INCLUDE_PATH})
link_libraries(${MPI_CXX_LIBRARIES} uuid)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${MPI_CXX_LINK_FLAGS})
include_directories($ENV{XILINX_XRT}/include)
include_directories(include)
add_subdirectory(src)
add_subdirectory(test)

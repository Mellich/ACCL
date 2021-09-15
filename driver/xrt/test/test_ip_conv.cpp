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

#include "accl-comm.hpp"

int main() {
	communicator cd;
	string ip = "192.168.2.25";

	auto ip_stored = cd.ip_encode(ip);
	auto ip_restored = cd.ip_decode(ip_stored);

	cout << "IP: " << ip << " " << ip_stored << " " << ip_restored << endl;	

	int error = 0;
	if(ip!=ip_restored) {
		error++;
	}

	return error;
}

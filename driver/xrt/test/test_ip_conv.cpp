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

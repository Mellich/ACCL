#include "accl-device.hpp"
#include "timing.hpp"

int main (int argc, char *argv[]) {

  MPI_Init(&argc, &argv);

  const std::string bitstream_f = argv[1];
  const auto device_idx = atoi(argv[2]);
  const auto nbufs = atoi(argv[3]);
  const auto mode = atoi(argv[4]);

  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  std::cout << "Rank " << rank << std::endl;
  std::cout << "Bitstream " << bitstream_f << std::endl;
  std::cout << "Buffer count " << nbufs << std::endl;
//  std::cout << "Mode " << mode << std::endl;

  // Setup
  Timer t_construct, t_bitstream, t_read_reg, t_write_reg, t_execute_kernel,
      t_preprxbuffers, t_dump_rx_buffers, t_config_comm, t_dump_comm;
  accl_operation_t op = nop;

  const int buffer_size = nbufs * 1024;

  std::cerr << "Construct ACCL" << std::endl;
  t_construct.start();
  ACCL f(nbufs, buffer_size, device_idx, rank, size, DUAL);
  t_construct.end();

  std::cerr << "Load bitstream" << std::endl;
  t_bitstream.start();
  f.load_bitstream(bitstream_f);
  t_bitstream.end();

  int error = 0;
  if (f.get_hwid()!= 0xcafebabe) {
		error++;
  }
	MPI_Finalize();
	return error;
}

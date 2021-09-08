add_executable(test_sendrecv test_sendrecv.cpp)
add_executable(test_regio test_regio.cpp)


add_test(NAME test_sendrecv COMMAND test_sendrecv)
add_test(NAME test_regio COMMAND test_regio.sh)

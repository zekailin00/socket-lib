#include <stdlib.h>
#include <vector>
#include <queue>
#include <iomanip>
#include <iostream>
#include <stdint.h>
#include <socketlib.h>
#include <unistd.h>

struct test_args {
  uint32_t addr;
  uint32_t size;
};

int main() {
  char tosend_arr[4] = {(char) 0xdeu, (char) 0xadu, (char) 0xbeu, (char) 0xefu};
  std::vector<char> tosend;
  tosend.push_back((char)0xde);
  tosend.push_back((char)0xde);
  tosend.push_back((char)0xde);
  tosend.push_back((char)0xde);
  struct test_args tosend_args;
  tosend_args.addr = 0x8badf00dU;
  tosend_args.size = 0x12345678U;

  // init_server(6969);
  init_server_file("test.socket");
  std::vector<char> args_buf;
  args_buf = serialize_args(tosend_args);
  std::vector<char> buf;
  while (true) {
    sleep(1);
    socket_send(0, 69, args_buf, tosend);
    socket_send(0, 420, empty_vec, empty_vec);
  }
  return 0;
}

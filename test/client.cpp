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

void printVectorInHex(const std::vector<char>& buf) {
    for (const char& c : buf) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c) << " ";
    }
    std::cout << std::dec << std::endl;
}

int main() {
  init_client(6969);
  std::vector<char> buf;
  while (true) {
    sleep(1);
    if (socket_receive(69, false, buf)) {
      struct test_args args;
      deserialize_args(buf, args);
      std::cout << "addr=0x" << std::hex << args.addr << ", size=0x" << args.size << std::endl;
      printVectorInHex(buf);
    }
    if (socket_receive(420, false, buf)) {
      std::cout << "received blank call" << std::endl;
    }
  }
  return 0;
}

#include <stdlib.h>
#include <vector>
#include <queue>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <iostream>
#include "socketlib.h"
#include <stdbool.h>
#include <stdint.h>
#include <cstring>
#include <algorithm>

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

uint64_t socket_cycles = 0;


#ifndef RISCV_COMPILATION

#define CYCLES_START() 
#define CYCLES_END() 

#else

inline static uint64_t sock_read_cycles() {
  uint64_t cycles = 0;
  asm volatile ("rdcycle %0" : "=r" (cycles));
  return cycles;
}

#define CYCLES_START() \
  uint64_t ent_cycles = sock_read_cycles();

#define CYCLES_END() \
  socket_cycles = socket_cycles + (sock_read_cycles() - ent_cycles); \
  printf("[SocketLib] current cycle count: %llu at %d\n", socket_cycles, __LINE__);

#endif

int server_socket, new_socket;

void init_server(const uint32_t port) {
  CYCLES_START()
  struct sockaddr_in server_addr;
  struct sockaddr_storage server_storage;
  socklen_t addr_size;
  server_socket = socket(PF_INET, SOCK_STREAM, 0);

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = INADDR_ANY;
  memset(server_addr.sin_zero, '\0', sizeof server_addr.sin_zero);
  bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr));

  // Listen for incoming connections
  if (listen(server_socket, 5) == 0) {
    std::cout << "Listening on port " << port << std::endl;
    fcntl(new_socket, F_SETFL, O_NONBLOCK);
  } else {
    std::cerr << "Error in listening." << std::endl;
    exit(1);
  }

  addr_size = sizeof server_storage;
  fcntl(server_socket, F_SETFL, O_NONBLOCK);

  while ((new_socket = accept(server_socket, (struct sockaddr *) &server_storage, &addr_size)) == -1) {}
  std::cout << "Connection accepted" << std::endl;
  // TODO: re-accept once client dropped
  CYCLES_END()
}

void init_server_file(const char *socket_path) {
  CYCLES_START()
  struct sockaddr_un addr;
  server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
  printf("pass1\n");


  if (server_socket == -1) {
      perror("socket");
      exit(EXIT_FAILURE);
  }
  unlink(socket_path);
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
  printf("pass2\n");


  if (bind(server_socket, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
      perror("bind");
      close(server_socket);
      exit(EXIT_FAILURE);
  }

  printf("pass3\n");

  if (listen(server_socket, 5) == -1) {
      perror("listen");
      close(server_socket);
      exit(EXIT_FAILURE);
  }
  std::cout << "Listening on file " << socket_path << std::endl;
  // fcntl(server_socket, F_SETFL, O_NONBLOCK);

  while ((new_socket = accept(server_socket, NULL, NULL)) == -1) {
      printf("pass4\n");
      perror("accept");
      close(server_socket);
      exit(EXIT_FAILURE);
  }
  printf("pass5\n");
  uint8_t endpoint_req;
  // recv(new_socket, &endpoint_req, 1, 0);
  fcntl(new_socket, F_SETFL, O_NONBLOCK);
  std::cout << "Connection accepted" << std::endl;
  CYCLES_END()
}

void init_client(const uint32_t port) {
  init_client(port, NOSERV);
}

void init_client(const uint32_t port, const endpoint_id_t endpoint_id) {
  CYCLES_START()
  struct sockaddr_in server_addr;
  new_socket = socket(PF_INET, SOCK_STREAM, 0);
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

  if (connect(new_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) == 0) {
    std::cout << "Connected to port " << port << std::endl;
    if ((endpoint_id != NOSERV) && (send(new_socket, &endpoint_id, sizeof(endpoint_id_t), 0) < 0)) {
      close(new_socket);
      std::cerr << "Failed to register endpoint ID." << std::endl;
      CYCLES_END()
      exit(1);
    }
    // fcntl(new_socket, F_SETFL, O_NONBLOCK);
  } else {
    std::cerr << "Failed to connect." << std::endl;
    CYCLES_END()
    exit(1);
  }
  CYCLES_END()
}

void init_client_file(const char *socket_path) {
  init_client_file(socket_path, NOSERV);
}

void init_client_file(const char *socket_path, const endpoint_id_t endpoint_id) {
  CYCLES_START()
  struct sockaddr_un addr;
  new_socket = socket(AF_UNIX, SOCK_STREAM, 0);
  if (new_socket == -1) {
    std::cerr << "Failed to connect." << std::endl;
    exit(1);
  }
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
  if (connect(new_socket, (struct sockaddr *) &addr, sizeof(addr)) == 0) {
    std::cout << "Connected to file " << socket_path << std::endl;
    if ((endpoint_id != NOSERV) && (send(new_socket, &endpoint_id, sizeof(endpoint_id_t), 0) < 0)) {
      close(new_socket);
      std::cerr << "Failed to register endpoint ID." << std::endl;
      exit(1);
    }
    // fcntl(new_socket, F_SETFL, O_NONBLOCK);
  } else {
    std::cerr << "Failed to connect." << std::endl;
    exit(1);
  }
  CYCLES_END()
}

std::deque<message_packet_t> received_packets;
void fetch_packets() {
  message_header_t header;
  while (recv(new_socket, &header, sizeof(header), MSG_PEEK | MSG_DONTWAIT) > 0) { // more data to come
#ifdef SOCKETLIB_VERBOSE
    std::cout << "DEBUG: new data is arriving" << std::endl;
#endif
    // fcntl(new_socket, F_SETFL, 0);
    ssize_t header_bytes_received = recv(new_socket, &header, sizeof(message_header_t), 0);
    if (header_bytes_received != sizeof(message_header_t)) {
      // TODO: maybe put this in a loop to ensure we receive the header
      std::cerr << "Error receiving header: " << errno << std::endl;
    }
    char *message_data = (char *) malloc(header.size - sizeof(message_header_t));
    if (message_data == nullptr) {
      std::cerr << "Error allocating memory for message data" << std::endl;
      exit(-1);
    }
#ifdef SOCKETLIB_VERBOSE
    std::cout << "DEBUG: start receiving message with size " << header.size << std::endl;
#endif
    size_t bytes_received = 0;
    while (bytes_received < header.size - sizeof(message_header_t)) {
        size_t chunk_size = MIN(1024ul, header.size - sizeof(message_header_t) - bytes_received);
        ssize_t chunk_bytes_received = recv(new_socket, message_data + bytes_received, chunk_size, 0);
        if (chunk_bytes_received == -1) {
          std::cerr << "Error receiving message data: " << errno << " " << strerror(errno) << std::endl;
          exit(-1);
        }
        bytes_received += chunk_bytes_received;
    }
    // fcntl(new_socket, F_SETFL, O_NONBLOCK);
#ifdef SOCKETLIB_VERBOSE
    std::cout << "DEBUG: added packet with func id " << header.func_id << std::endl;
#endif
    message_packet_t new_packet;
    new_packet.header = header;
    new_packet.payload = message_data;
    received_packets.push_back(new_packet);
  }
}

int socket_receive(const func_id_t func_id, const bool blocking, std::vector<char> &dest_buf) {
  CYCLES_START()
  // check for qualifying messages
  // if not found, fetch from socket
  // if blocking, fetch & check in a loop
  bool found = false;
  endpoint_id_t src_id = 0;
  dest_buf.clear();
  if (blocking) {
#ifdef SOCKETLIB_VERBOSE
    std::cout << "DEBUG: trying to find packet with id " << func_id << std::endl;
#endif
    do {
      fetch_packets();
      for (auto it = received_packets.begin(); it != received_packets.end(); ++it) {
#ifdef SOCKETLIB_VERBOSE
      if (blocking) std::cout << "DEBUG: examining packet with id " << it->header.func_id << std::endl;
#endif
        if (it->header.func_id == func_id) {
           found = true;
           src_id = it->header.src_id;
           dest_buf.insert(dest_buf.end(), it->payload, it->payload + it->header.size - sizeof(message_header_t));
           free(it->payload);
           received_packets.erase(it);
           break;
        }
      }
    } while (blocking && !found);
  } else {
    if (received_packets.size() == 0) fetch_packets();
    if (received_packets.size() > 0) {
      auto it = received_packets.begin();
      if (it->header.func_id == func_id) {
         src_id = it->header.src_id;
         dest_buf.insert(dest_buf.end(), it->payload, it->payload + it->header.size - sizeof(message_header_t));
         free(it->payload);
         received_packets.pop_front();
      }
    }
  }
  CYCLES_END()
  return (int) src_id;
}

int socket_send(const endpoint_id_t endpoint_id, const func_id_t func_id, const std::vector<char> &args, const std::vector<char> &payload) {
  CYCLES_START()
  message_header_t header;
  header.size = sizeof(message_header_t) + args.size() + payload.size();
  header.src_id = NOSERV; // will be overriden by server
  header.dst_id = endpoint_id;
  header.func_id = func_id;

  // fcntl(new_socket, F_SETFL, 0);
  if ((size_t) send(new_socket, &header, sizeof(header), 0) != sizeof(header)) {
    std::cout << "DEBUG: failed to send header" << std::endl;
    CYCLES_END()
    return -1;
  }

  if ((size_t) send(new_socket, args.data(), args.size(), 0) != args.size()) {
    std::cout << "DEBUG: failed to send args" << std::endl;
    CYCLES_END()
    return -1;
  }

  for (size_t i = 0; i < payload.size();) {
    size_t chunk_size = MIN(1024ul, payload.size() - i);
    const char* data_ptr = payload.data() + i;
    ssize_t bytes_sent = send(new_socket, data_ptr, chunk_size, 0);
#ifdef SOCKETLIB_VERBOSE
    std::cout << "DEBUG: sent " << bytes_sent << " bytes" << std::endl;
#endif
    i += bytes_sent;
    if (bytes_sent == -1) {
      std::cout << "DEBUG: failed to send everything" << std::endl;
      CYCLES_END()
      return -1;
    }
  }
  // fcntl(new_socket, F_SETFL, O_NONBLOCK);
  CYCLES_END()
  return 0;
}
#include "server.h"

#include <chrono>
#include <iostream>

namespace rowen {
namespace server {

int EchoServer::onReceived(const ConnectedClient* client)
{
  std::unique_lock<std::mutex> ulock(message_sync_mutex_);

  uint8_t buffer[PACKET_BUFFER_SIZE];
  memset(buffer, '\0', sizeof(buffer));

  int read_size = read(client->client_sock, buffer, sizeof(buffer));

  if (read_size <= 0) {
    return read_size;
  }

  // 데이터 처리부 (Echo server)
  auto res = sendMessage(client->client_sock, buffer, read_size);
  if (res == read_size) {
    printf("echo : %s\n", buffer);
  }

  return read_size;
}

}  // namespace server
}  // namespace rowen
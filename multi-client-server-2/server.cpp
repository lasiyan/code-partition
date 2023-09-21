#include "server.h"

#include <string.h>

#include <chrono>
#include <iostream>

#ifdef SAFE_DELETE
  #undef SAFE_DELETE
#endif

#ifdef SAFE_DELETE_ARRAY
  #undef SAFE_DELETE_ARRAY
#endif

#define SAFE_DELETE(x) \
  if (x != nullptr) {  \
    delete x;          \
    x = nullptr;       \
  }

#define SAFE_DELETE_ARRAY(x) \
  if (x != nullptr) {        \
    delete[] x;              \
    x = nullptr;             \
  }

namespace rowen {
namespace server {

int MyServer::onReceived(ConnectedClient* client)
{
  std::unique_lock<std::mutex> ulock(message_sync_mutex_);

  // variables
  uint8_t* body   = nullptr;
  PacketHeader*  header = nullptr;

  int read_size = recvMessage(client, header, body);

  // If, Recv Invalid Message (null exception)
  if (read_size <= 0 | header == nullptr || body == nullptr) {
    SAFE_DELETE(header);
    SAFE_DELETE_ARRAY(body);
    return read_size;
  }

  // 데이터 처리
  switch (header->command) {
    case CMD_ECHO: {
      auto res = sendMessage(client->client_sock, body, header->datasize);
      if (res == header->datasize) {
        printf("echo : %s\n", body);
      }
      break;
    }
    case CMD_SET_MESSAGE:
      snprintf(client->client_message, sizeof(client->client_message), "%s", body);
      break;
    case CMD_GET_MESSAGE: {
      auto res = sendMessage(client->client_sock,
                             reinterpret_cast<uint8_t*>(client->client_message),
                             strlen(client->client_message));
      if (res == strlen(client->client_message)) {
        printf("response : %s\n", body);
      }
      break;
    }
    default:
      break;
  }

  SAFE_DELETE(header);
  SAFE_DELETE_ARRAY(body);

  return read_size;
}

#if 0
int MyServer::recvMessage(int socket, PacketHeader*& header, uint8_t*& buffer)
{
  bool recv_buffer_over = false;
  int  data_size        = 0;
  int  total_recv_size  = 0;

  uint8_t read_buffer[Server::PACKET_BUFFER_SIZE];
  bzero(read_buffer, sizeof(read_buffer));

  auto ret = read(socket, read_buffer, sizeof(read_buffer));

  // wrong message, so drop
  if (ret < sizeof(PacketHeader))
    return ret;

  // Parse Header & Data
  header = new PacketHeader;
  memcpy(header, read_buffer, sizeof(PacketHeader));

  buffer = new uint8_t[header->datasize + 1];  // 1 : eos
  memset(buffer, '\0', header->datasize + 1);  // add null
  memcpy(buffer, read_buffer + sizeof(PacketHeader), header->datasize);

  return ret;
}
#else
int MyServer::recvMessage(ConnectedClient* sClient, PacketHeader*& header, uint8_t*& buffer)
{
  constexpr int MAX_RETRY = 3;

  bool recv_buffer_over = false;  // PACKET_BUFFER_SIZE보다 큰 값을 수신한 경우

  int recv_error_count = 0;  // 수신 에러 카운트
  int total_recv_size  = 0;  // 수신된 총 데이터 크기
  int total_data_size  = 0;  // 수신된 총 데이터 크기 (헤더 포함)
  int remain_data_size = 0;  // 수신 받아야 할 잔여 데이터 크기

  uint8_t read_buffer[Server::PACKET_BUFFER_SIZE];
  memset(read_buffer, '\0', sizeof(read_buffer));

  int ret = 0;

  char title[64];
  snprintf(title, sizeof(title), "%s", sClient->client_ip);

  while ((ret = read(sClient->client_sock, read_buffer, sizeof(read_buffer))) != 0) {
    try {
      // ret가 0 이하인 경우( 에러가 발생한 경우 )
      if (ret <= 0) {
        recv_error_count++;
        if (ret == 0)  // 연결 종료
          throw false;
        else if (recv_error_count >= MAX_RETRY) {
          printf("%s : recv message error %d over times\n", title, MAX_RETRY);
          throw true;
        }
        else
          continue;
      }

      // 에러 카운트 초기화
      recv_error_count = 0;

      // 총 수신 크기 저장
      total_recv_size += ret;

      // 추가 수신(패킷 버퍼 오버플로) 상태가 아닌데, 헤더보다 작은 경우 예외처리
      if (recv_buffer_over == false && ret < sizeof(PacketHeader)) {
        printf("%s : recv message less than %lu bytes\n", title, sizeof(PacketHeader));
        break;
      }

      // 추가 수신(패킷 버퍼 오버플로) 상태가 아닌 경우
      if (recv_buffer_over == false) {
        header = new PacketHeader;
        memcpy(header, read_buffer, sizeof(PacketHeader));
        int curr_data_size = ret - sizeof(PacketHeader);
        total_data_size += curr_data_size;
        remain_data_size = header->datasize - curr_data_size;

        // 커맨드가 유효한지 확인
        if (header->command < CMD_ECHO || header->command > CMD_GET_MESSAGE) {
          printf("%s : invalid command code: %d\n", title, header->command);
          break;
        }

        buffer = new unsigned char[header->datasize + 1];  // 1 : eos
        memset(buffer, '\0', header->datasize + 1);
        memcpy(buffer, read_buffer + sizeof(PacketHeader), curr_data_size);

        // 수신된 데이터가 헤더의 데이터 크기보다 작은 경우 ( 추가 수신 필요 )
        if (header->datasize > curr_data_size) {
          printf("%s : wait for additional packet (%d/%d)\n", title, curr_data_size, header->datasize);
          recv_buffer_over = true;
          continue;
        }
        else {
          break;
        }
      }
      else {
        // 추가 수신(패킷 버퍼 오버플로) 상태인 경우
        if (remain_data_size < ret)
          ret = remain_data_size;
        remain_data_size -= ret;

        memcpy(buffer + total_data_size, read_buffer, ret);
        total_data_size += ret;

        // 총 수신된 데이터가 헤더의 데이터 크기보다 큰 경우
        if (total_data_size > header->datasize) {
          printf("%s : buffer overflow.. some of received data is missing\n", title);
          break;
        }
        // 총 수신된 데이터가 헤더의 데이터 크기와 같은 경우
        else if (total_data_size == header->datasize) {
          break;
        }
        // 총 수신된 데이터가 헤더의 데이터 크기보다 작은 경우
        else {
          continue;
        }
      }
    }
    catch (bool is_error) {
      if (is_error)
        total_recv_size = -1;
      else {
        total_recv_size = 0;
      }
    }
  }
  return total_recv_size;
}
#endif

}  // namespace server
}  // namespace rowen
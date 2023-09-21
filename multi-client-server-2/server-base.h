#ifndef ROWEN_SERVER_BASE_H
#define ROWEN_SERVER_BASE_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#define MY_INVALID_SOCKET (-1)
#define MY_INVALID_PORT   (0)

namespace rowen {
namespace server {

class Server {
  virtual void running() = 0;  // thrad method

 public:
  static constexpr auto PACKET_BUFFER_SIZE = 16;

  struct ConnectedClient {
    int  client_sock   = { 0 };
    char client_ip[17] = { 0 };

    char client_message[256] = { 'I', 'N', 'I', 'T', '\0' };
  };

 public:
  Server() = default;
  virtual ~Server();

 public:
  virtual bool start();
  virtual void stop();

 protected:
  int sendMessage(int socket, uint8_t* buffer, int buffer_size);

 protected:
  bool openSocket();
  void closeSocket();

 protected:
  int         socket_   = MY_INVALID_SOCKET;
  int         port_     = MY_INVALID_PORT;
  sockaddr_in sockaddr_ = {};

  std::thread      thread_;
  std::atomic_bool thread_stop_;
};

/////////////////////////////////////////////////////////////////////////////////
// Server : TCP based
class MultiClientServer : public Server {
  void running() override;

 public:
  virtual ~MultiClientServer() { stop(); }

  std::vector<ConnectedClient*>& clients() { return client_list_; }
  ConnectedClient*               client(int socket);

 public:
  void stop() override;

 private:
  virtual int onReceived(ConnectedClient* client_info) = 0;

 protected:
  std::mutex ctrl_client_mutex_;
  std::mutex message_sync_mutex_;

  std::vector<ConnectedClient*> client_list_;
};

};  // namespace server
};  // namespace rowen

#endif  // ROWEN_SERVER_BASE_H
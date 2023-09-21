#include "server-base.h"

#include <iostream>

int exit_flag = false;

namespace rowen {
namespace server {

Server::~Server()
{
  stop();
}

bool Server::start()
{
  if (port_ == 0) {
    printf("server port number is 0\n\
            -- please implement your own port number\n");
    return false;
  }

  if (thread_.joinable() == false) {
    thread_stop_.store(false);
    thread_ = std::thread(&Server::running, this);
    return true;
  }

  return false;
}

void Server::stop()
{
  thread_stop_.store(true);
  if (thread_.joinable()) {
    thread_.join();
  }
  closeSocket();

  printf("close server done.\n");
}

int Server::sendMessage(int socket, uint8_t* buffer, int buffer_size)
{
  if (buffer_size <= 0)
    return 0;

  if (socket == MY_INVALID_SOCKET) {
    printf("sendMessage : socket is invalid\n");
    return 0;
  }

  constexpr int MAX_RETRY = 3;

  int total_send  = 0;
  int retry_count = 0;

  while (true) {
    int remain_size = (buffer_size - total_send);

    int send_size = remain_size > PACKET_BUFFER_SIZE ? PACKET_BUFFER_SIZE : remain_size;

    if (send_size == 0)
      break;

    int res = send(socket, buffer + total_send, send_size, MSG_NOSIGNAL);

    if (res > 0) {
      retry_count = 0;
      total_send += res;
    }
    else {
      printf("sendMessage : retry sending (%d / %d)\n", retry_count, MAX_RETRY);
      if (++retry_count >= MAX_RETRY) {
        break;
      }
    }
  }

  if (total_send != buffer_size) {
    printf("sendMessage : send size error : (%d / %d)\n", total_send, buffer_size);
    // return 0;
  }

  return total_send;
}

bool Server::openSocket()
{
  if (port_ == MY_INVALID_PORT) {
    printf("server port is invalid\n");
    return false;
  }

  // create socket
  if ((socket_ = ::socket(PF_INET, SOCK_STREAM, 0)) <= 0) {
    printf("socket() : error : %d : %s\n", errno, strerror(errno));
    return false;
  }

  // set socket option
  int opt = 1;

  if (setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    printf("setsockopt() : REUSE ADDR : error : %d : %s\n", errno, strerror(errno));
    return false;
  }

  if (setsockopt(socket_, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
    printf("setsockopt() : REUSE PORT : error : %d : %s\n", errno, strerror(errno));
    return false;
  }

  // set sockaddr_in
  memset(&sockaddr_, 0, sizeof(sockaddr_));
  sockaddr_.sin_family      = PF_INET;
  sockaddr_.sin_addr.s_addr = htonl(INADDR_ANY);
  sockaddr_.sin_port        = htons(port_);

  // bind
  if (bind(socket_, (struct sockaddr*)&sockaddr_, sizeof(sockaddr_)) < 0) {
    printf("bind() : error : %d : %s\n", errno, strerror(errno));
    return false;
  }

  // listen (maximum client: 10)
  if (listen(socket_, 10) < 0) {
    printf("listen() : error : %d : %s\n", errno, strerror(errno));
    return false;
  }

  return true;
}

void Server::closeSocket()
{
  if (socket_ != MY_INVALID_SOCKET) {
    close(socket_);
    socket_ = MY_INVALID_SOCKET;
  }
}

////////////////////////////////////////////////////////////////////////////////
void MultiClientServer::running()
{
  while (thread_stop_.load() == false && openSocket() == false) {
    std::this_thread::sleep_for(std::chrono::seconds(10));
  }

  printf("wait for client..\n");

  // initialize socket
  fd_set fd_reads;
  int    fd_max, fd_num;

  while (thread_stop_.load() == false) {
    FD_ZERO(&fd_reads);

    FD_SET(socket_, &fd_reads);
    fd_max = socket_;

    for (unsigned int i = 0; i < client_list_.size(); i++) {
      fd_num = client_list_[i]->client_sock;
      FD_SET(fd_num, &fd_reads);
      if (fd_num > fd_max)
        fd_max = fd_num;
    }

    timeval select_timeout = { 1, 0 };

    int activity = select(fd_max + 1, &fd_reads, NULL, NULL, &select_timeout);
    if (activity < 0) {
      printf("select() : error : %d : %s\n", errno, strerror(errno));
      break;
    }

    if (activity == 0) {
      usleep(10);
      continue;
    }

    // Master(main) socket
    if (FD_ISSET(socket_, &fd_reads)) {
      struct sockaddr_in client_addr;
      socklen_t          client_addr_size = sizeof(client_addr);

      int clnt_sock = accept(socket_, (struct sockaddr*)&client_addr, &client_addr_size);

      if (clnt_sock < 0) {
        printf("accept() : error : %d : %s\n", errno, strerror(errno));
        continue;
      }

      if (client(clnt_sock) != nullptr) {
        printf("already exist client (id: %d)\n", clnt_sock);
        continue;
      }

      std::unique_lock<std::mutex> ulock(ctrl_client_mutex_);

      // Accept client
      timeval send_timeout = { 1, 0 };
      setsockopt(clnt_sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&send_timeout, sizeof(send_timeout));

      char* clnt_ip = inet_ntoa(client_addr.sin_addr);

      auto* sClient = new struct ConnectedClient;
      snprintf(sClient->client_ip, sizeof(sClient->client_ip), "%s", clnt_ip);
      sClient->client_sock = clnt_sock;

      client_list_.push_back(sClient);

      printf("=> Connected New Client (IP: %s, Sock: %d)\n", clnt_ip, clnt_sock);
    }
    
    for (int i = 0; i < client_list_.size(); i++) {
      int client_socket = client_list_[i]->client_sock;

      if (FD_ISSET(client_socket, &fd_reads)) {
        auto sClient = client(client_socket);

        if (sClient) {
          try {
            int ret = onReceived(sClient);

            if (ret == 0) {
              std::unique_lock<std::mutex> ulock(ctrl_client_mutex_);

              close(client_socket);

              printf("=> Disconnected Client (IP: %s, Sock: %d)\n", sClient->client_ip, client_socket);

              for (int idx = 0; idx < client_list_.size(); idx++) {
                if (client_list_[idx]->client_sock == client_socket) {
                  delete client_list_[idx];
                  client_list_[idx] = nullptr;
                  client_list_.erase(client_list_.begin() + idx);
                  break;
                }
              }
            }
          }
          catch (std::exception& e) {
            perror(e.what());
          }
          catch (...) {
            perror("undefined error");
          }
        }
      }
    }
  }
}

void MultiClientServer::stop()
{
  std::unique_lock<std::mutex> ulock(message_sync_mutex_);

  for (int idx = 0; idx < client_list_.size(); idx++) {
    delete client_list_[idx];
    client_list_[idx] = nullptr;
  }
  client_list_.clear();
  client_list_.shrink_to_fit();
}

Server::ConnectedClient* MultiClientServer::client(int sock)
{
  ConnectedClient* sClient = nullptr;

  std::unique_lock<std::mutex> ulock(ctrl_client_mutex_);
  for (int idx = 0; idx < client_list_.size(); idx++) {
    if (client_list_[idx]->client_sock == sock) {
      sClient = client_list_[idx];
      break;
    }
  }
  return sClient;
}

}  // namespace server
}  // namespace rowen

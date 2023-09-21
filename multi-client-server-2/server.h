#ifndef ROWEN_SERVER_H
#define ROWEN_SERVER_H

#include "protocol.h"
#include "server-base.h"

namespace rowen {
namespace server {

class MyServer : public server::MultiClientServer {
 public:
  static MyServer& instance()
  {
    static MyServer instance_;
    return instance_;
  }

 private:
  MyServer()
  {
    port_ = 6884;
  }

  int onReceived(ConnectedClient* sClient) override;

 private:
  int recvMessage(ConnectedClient* sClient, PacketHeader*& header, uint8_t*& buffer);
};

};  // namespace server
};  // namespace rowen

#endif  // ROWEN_SERVER_H
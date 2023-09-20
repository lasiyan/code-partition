#ifndef ROWEN_SERVER_H
#define ROWEN_SERVER_H

#include "server-base.h"

namespace rowen {
namespace server {

class EchoServer : public server::MultiClientServer {
 public:
  static EchoServer& instance()
  {
    static EchoServer instance_;
    return instance_;
  }

 private:
  EchoServer()
  {
    port_ = 9999;
  }

  int onReceived(const ConnectedClient* sClient) override;
};

};  // namespace server
};  // namespace rowen

#endif  // ROWEN_SERVER_H
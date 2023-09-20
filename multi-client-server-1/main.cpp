#include "server.h"
#include <signal.h>

extern bool exit_flag;

void sig_handler(int signo)
{
  exit_flag = true;
}

int main()
{
  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);
  signal(SIGKILL, sig_handler);

  // running server
  rowen::server::EchoServer::instance().start();

  // Ctrl+Q 종료
  while (exit_flag == false) { usleep(10); }

  // stop server
  rowen::server::EchoServer::instance().stop();

  return 0;
}
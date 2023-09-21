#ifndef ROWEN_SERVER_PROTOCOL_H
#define ROWEN_SERVER_PROTOCOL_H

// Declare Packet Header
struct PacketHeader {
  int command;  // Protocol OP-Code (4 byte)
  int datasize; // Packet Data Payload size (4 byte)
};

// Declare OP-Code
enum OP_CODE {
  CMD_ECHO = 1,

  CMD_SET_MESSAGE,
  CMD_GET_MESSAGE,
};


#endif  // ROWEN_SERVER_PROTOCOL_H
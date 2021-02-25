#pragma once

#include <string>
#include <vector>
#include <memory>

#include <WinSock2.h>
#include <WS2tcpip.h>

// Transaction unit headers
//
//        0                   1                   2                   3
//        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       |          32 bit cookie (minor security and versioning)        |
//       |                 32 bit transaction id                         |
//       |       Msg type                  |        Length               |
//       |                    32 bit Sequence #                          |
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       |                                                               |
//       |                         Message data                          |
//       |                                                               |
//       .                                                               .
//       .                                                               .
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

static const int MagicCookie = 0xA343F33B;               // A magic cookie to recognize our activity
// Message types
enum MsgType
{
   MsgType_StartTransaction = 0x0001,  // Message data contains filename (Sequence #0 expected)
   MsgType_EndTransaction = 0x0002,    // Message data empty
   MsgType_RetransmitReq = 0x0003,     // Message data empty (Sequence number is requested sequence)
   MsgType_Data = 0x0004,              // Message data contains a message blob of specified size
};

class TransactionUnit
{
public:
   TransactionUnit();
   TransactionUnit(const std::vector<char>& buffer);

   bool IsValid() { return _isValid; }

   sockaddr_in GetRemoteAddress() { return _remoteAddr; }
   void SetRemoteAddress(const sockaddr_in& fromAddr) { _remoteAddr = fromAddr; }
   void SetRemoteIP(const std::string& ip) 
   {
      inet_pton(AF_INET, ip.c_str(), (void*)&_remoteAddr.sin_addr.s_addr);
   }

   void SetRemotePort(uint16_t port)
   {
      _remoteAddr.sin_port = port;
   }

   void GetBlob(std::vector<char>& buffer) const;

   uint32_t cookie;
   uint32_t transactionid;
   uint16_t messagetype;
   uint16_t messagelength;
   uint32_t sequencenum;

   std::vector<char> messagedata;

private:
   const uint32_t _mySize;
   bool _isValid;
   sockaddr_in _remoteAddr;
};

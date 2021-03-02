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

   uint32_t GetTransactionID() { return _transactionid; }
   void SetTransactionID(uint32_t transactionid) { _transactionid = transactionid; }
   
   uint16_t GetMessageType() { return _messagetype; }
   void SetMessageType(uint16_t messagetype) { _messagetype = messagetype; }
   
   uint32_t GetSequenceNumber() { return _sequencenum; }
   void SetSequenceNumber(uint32_t sequencenum) { _sequencenum = sequencenum; }

   void GetMessageData(std::string& s) { s.assign(_messagedata.begin(), _messagedata.end()); }
   void SetMessageData(const std::string& s)
   {
      _messagedata.assign(s.begin(), s.end());
   }

   std::vector<char>& GetMessageDataVector() { return _messagedata; }

   uint16_t GetMessageLength() { return (uint16_t)_messagedata.size(); }

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

private:
   uint32_t _cookie;
   uint32_t _transactionid;
   uint16_t _messagetype;
   uint32_t _sequencenum;

   std::vector<char> _messagedata;

   const uint32_t _mySize;
   bool _isValid;
   sockaddr_in _remoteAddr;
};

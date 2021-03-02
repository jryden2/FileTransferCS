#include "TransactionUnit.h"

TransactionUnit::TransactionUnit(const std::vector<char>& buffer)
   : _mySize(16)
{
   memset(&_remoteAddr, 0, sizeof(_remoteAddr));

   auto buf = buffer.data();

   memcpy(&_cookie, buf, sizeof(_cookie));
   buf += sizeof(_cookie);

   memcpy(&_transactionid, buf, sizeof(_transactionid));
   buf += sizeof(_transactionid);

   memcpy(&_messagetype, buf, sizeof(_messagetype));
   buf += sizeof(_messagetype);

   uint16_t messageLength;
   memcpy(&messageLength, buf, sizeof(messageLength));
   buf += sizeof(messageLength);

   memcpy(&_sequencenum, buf, sizeof(_sequencenum));
   buf += sizeof(_sequencenum);

   _messagedata.resize(messageLength);
   _messagedata.assign(buf, buf + messageLength);

   _isValid = _cookie == MagicCookie;
}

TransactionUnit::TransactionUnit()
   : _mySize(16),
   _isValid(true),
   _cookie(MagicCookie),
   _messagetype(0)
{
   memset(&_remoteAddr, 0, sizeof(_remoteAddr));
}

void TransactionUnit::GetBlob(std::vector<char>& buffer) const
{
   buffer.resize(_mySize + _messagedata.size());

   char* buf = buffer.data();

   memcpy(buf, &_cookie, sizeof(_cookie));
   buf += sizeof(_cookie);

   memcpy(buf, &_transactionid, sizeof(_transactionid));
   buf += sizeof(_transactionid);

   memcpy(buf, &_messagetype, sizeof(_messagetype));
   buf += sizeof(_messagetype);

   uint16_t messageLength = _messagedata.size();
   memcpy(buf, &messageLength, sizeof(messageLength));
   buf += sizeof(messageLength);

   memcpy(buf, &_sequencenum, sizeof(_sequencenum));
   buf += sizeof(_sequencenum);

   memcpy(buf, _messagedata.data(), _messagedata.size());


}

#include "TransactionUnit.h"

TransactionUnit::TransactionUnit(const std::vector<char>& buffer)
   : _mySize(16)
{
   auto buf = buffer.data();

   memcpy(&cookie, buf, sizeof(cookie));
   buf += sizeof(cookie);

   memcpy(&transactionid, buf, sizeof(transactionid));
   buf += sizeof(transactionid);

   memcpy(&messagetype, buf, sizeof(messagetype));
   buf += sizeof(messagetype);

   memcpy(&messagelength, buf, sizeof(messagelength));
   buf += sizeof(messagelength);

   memcpy(&sequencenum, buf, sizeof(sequencenum));
   buf += sizeof(sequencenum);

   messagedata.resize(messagelength);
   messagedata.assign(buf, buf + messagelength);

   _isValid = cookie == MagicCookie;
}

TransactionUnit::TransactionUnit()
   : _mySize(16),
   _isValid(true),
   cookie(MagicCookie)
{}

void TransactionUnit::GetBlob(std::vector<char>& buffer) const
{
   buffer.resize(_mySize + messagedata.size());

   char* buf = buffer.data();

   memcpy(buf, &cookie, sizeof(cookie));
   buf += sizeof(cookie);

   memcpy(buf, &transactionid, sizeof(transactionid));
   buf += sizeof(transactionid);

   memcpy(buf, &messagetype, sizeof(messagetype));
   buf += sizeof(messagetype);

   memcpy(buf, &messagelength, sizeof(messagelength));
   buf += sizeof(messagelength);

   memcpy(buf, &sequencenum, sizeof(sequencenum));
   buf += sizeof(sequencenum);

   memcpy(buf, messagedata.data(), messagedata.size());


}

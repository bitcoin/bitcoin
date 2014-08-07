#ifndef __BITCOIN_PROCMSG_H__
#define __BITCOIN_PROCMSG_H__

#include <string>
#include <stdint.h>
#include "serialize.h"

class CNode;

class MessageEngine {
public:
    bool ProcessMessages(CNode* pfrom);
    bool SendMessages(CNode* pto, bool fSendTrickle);

private:
    bool ProcessMessage(CNode* pfrom, std::string strCommand, CDataStream& vRecv, int64_t nTimeReceived);
};

extern class MessageEngine msgeng;

#endif // __BITCOIN_PROCMSG_H__

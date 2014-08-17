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

    bool MsgVersion(CNode* pfrom, CDataStream& vRecv, int64_t nTimeReceived);
    bool MsgVerack(CNode* pfrom, CDataStream& vRecv, int64_t nTimeReceived);
    bool MsgAddr(CNode* pfrom, CDataStream& vRecv, int64_t nTimeReceived);
    bool MsgInv(CNode* pfrom, CDataStream& vRecv, int64_t nTimeReceived);
    bool MsgGetData(CNode* pfrom, CDataStream& vRecv, int64_t nTimeReceived);
    bool MsgGetBlocks(CNode* pfrom, CDataStream& vRecv, int64_t nTimeReceived);
    bool MsgGetHeaders(CNode* pfrom, CDataStream& vRecv, int64_t nTimeReceived);
    bool MsgTx(CNode* pfrom, CDataStream& vRecv, int64_t nTimeReceived);
    bool MsgBlock(CNode* pfrom, CDataStream& vRecv, int64_t nTimeReceived);
    bool MsgGetAddr(CNode* pfrom, CDataStream& vRecv, int64_t nTimeReceived);
    bool MsgMempool(CNode* pfrom, CDataStream& vRecv, int64_t nTimeReceived);
    bool MsgPing(CNode* pfrom, CDataStream& vRecv, int64_t nTimeReceived);
    bool MsgPong(CNode* pfrom, CDataStream& vRecv, int64_t nTimeReceived);
    bool MsgAlert(CNode* pfrom, CDataStream& vRecv, int64_t nTimeReceived);
    bool MsgFilterLoad(CNode* pfrom, CDataStream& vRecv, int64_t nTimeReceived);
    bool MsgFilterAdd(CNode* pfrom, CDataStream& vRecv, int64_t nTimeReceived);
    bool MsgFilterClear(CNode* pfrom, CDataStream& vRecv, int64_t nTimeReceived);
    bool MsgFilterReject(CNode* pfrom, CDataStream& vRecv, int64_t nTimeReceived);
    bool MsgReject(CNode* pfrom, CDataStream& vRecv, int64_t nTimeReceived);
};

extern class MessageEngine msgeng;

#endif // __BITCOIN_PROCMSG_H__

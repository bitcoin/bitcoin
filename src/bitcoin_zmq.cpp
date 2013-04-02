// Copyright (c) 2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <zmq.h>

#include "hash.h"
#include "bitcoinrpc.h"
#include "main.h"

#define BZMQ_BUFFER_SIZE 8388608

static void *pBZmq_ctx = NULL;
static void *pBZmq_socket_pub = NULL;
static void *pBZmq_socket_rep = NULL;
std::string strBZmq_ID_TRANSACTION = "cf954abb";
std::string strBZmq_ID_BLOCK = "ec747b90";
std::string strBZmq_ID_IPADDRESS = "a610612a";
bool fBZmq_ThreadReqRep = false;

extern void TxToJSON(const CTransaction& tx, const uint256 hashBlock, json_spirit::Object& entry);
extern json_spirit::Object blockToJSON(const CBlock& block, const CBlockIndex* blockindex);
extern std::string JSONRPCExecBatch(const json_spirit::Array&);
extern json_spirit::Object JSONRPCExecOne(const json_spirit::Value&);


void BZmq_InitCtx() {
    pBZmq_ctx = zmq_ctx_new();
}

void BZmq_InitSockets() {
    pBZmq_socket_pub = zmq_socket(pBZmq_ctx, ZMQ_PUB);
    pBZmq_socket_rep = zmq_socket(pBZmq_ctx, ZMQ_REP);
}

void BZmq_CtxSetOptions(std::string& strOpt) {
    std::string::size_type nNamePos = strOpt.find(":");

    if (nNamePos != std::string::npos) 
    {
        std::string strName = strOpt.substr(0, nNamePos);
        std::string strValue = strOpt.substr(nNamePos + 1);
        std::stringstream streamStringToInts(strValue);
        int nValue_int = 0;
        streamStringToInts >> nValue_int;
        int nRc = -2;

        if (strName == "ZMQ_IO_THREADS")
            nRc = zmq_ctx_set(pBZmq_ctx, ZMQ_IO_THREADS, nValue_int);
        if (strName ==  "ZMQ_MAX_SOCKETS")
            nRc = zmq_ctx_set(pBZmq_ctx, ZMQ_MAX_SOCKETS, nValue_int);

        if (nRc == -2)
                printf("ZMQ_CTX_SET: ERROR! %s does not exists as an strOption!\n", strName.c_str());
        else
            if (nRc < 0)
                printf("ZMQ_CTX_SET: %s value: %i returncode: %i errno: %i strerror: %s\n", strName.c_str(), nValue_int, nRc, errno, strerror(errno));
            else
                printf("ZMQ_CTX_SET: %s value: %i OK\n", strName.c_str(), nValue_int);
    }
}

void BZmq_SetSocket(void *pSocket, const char *pszSocketName, std::string& strOpt) {
    std::string::size_type nNamePos = strOpt.find(":");

    if (nNamePos != std::string::npos) 
    {
        std::string strName = strOpt.substr(0, nNamePos);
        std::string strValue = strOpt.substr(nNamePos + 1);
        std::stringstream streamStringToInts(strValue);
        int nRc = -2;
        int nValue_int = 0; 
        streamStringToInts >> nValue_int;
        uint64_t nValue_uint64_t = 0;
        streamStringToInts >> nValue_uint64_t;
        int64_t nValue_int64_t = 0;
        streamStringToInts >> nValue_int64_t;

        if (strName == "ZMQ_SNDHWM")
            nRc = zmq_setsockopt (pSocket, ZMQ_SNDHWM, &nValue_int, sizeof(int));

        if (strName == "ZMQ_RCVHWM")
            nRc = zmq_setsockopt (pSocket, ZMQ_RCVHWM, &nValue_int, sizeof(int));

        if (strName == "ZMQ_AFFINITY")
            nRc = zmq_setsockopt (pSocket, ZMQ_AFFINITY, &nValue_uint64_t, sizeof(uint64_t));

        if (strName == "ZMQ_SUBSCRIBE")
            nRc = zmq_setsockopt (pSocket, ZMQ_SUBSCRIBE, strValue.c_str(), strValue.length());

        if (strName == "ZMQ_UNSUBSCRIBE")
            nRc = zmq_setsockopt (pSocket, ZMQ_UNSUBSCRIBE, strValue.c_str(), strValue.length());

        if (strName == "ZMQ_IDENTITY")
            nRc = zmq_setsockopt (pSocket, ZMQ_IDENTITY, strValue.c_str(), strValue.length());

        if (strName == "ZMQ_RATE")
            nRc = zmq_setsockopt (pSocket, ZMQ_RATE, &nValue_int, sizeof(int));

        if (strName == "ZMQ_RECOVERY_IVL")
            nRc = zmq_setsockopt (pSocket, ZMQ_RECOVERY_IVL, &nValue_int, sizeof(int));

        if (strName == "ZMQ_SNDBUF")
            nRc = zmq_setsockopt (pSocket, ZMQ_SNDBUF, &nValue_int, sizeof(int));

        if (strName == "ZMQ_RCVBUF")
            nRc = zmq_setsockopt (pSocket, ZMQ_RCVBUF, &nValue_int, sizeof(int));

        if (strName == "ZMQ_LINGER")
            nRc = zmq_setsockopt (pSocket, ZMQ_LINGER, &nValue_int, sizeof(int));

        if (strName == "ZMQ_RECONNECT_IVL")
            nRc = zmq_setsockopt (pSocket, ZMQ_RECONNECT_IVL, &nValue_int, sizeof(int));

        if (strName == "ZMQ_RECONNECT_IVL_MAX")
            nRc = zmq_setsockopt (pSocket, ZMQ_RECONNECT_IVL_MAX, &nValue_int, sizeof(int));

        if (strName == "ZMQ_BACKLOG")
            nRc = zmq_setsockopt (pSocket, ZMQ_BACKLOG, &nValue_int, sizeof(int));

        if (strName == "ZMQ_MAXMSGSIZE")
            nRc = zmq_setsockopt (pSocket, ZMQ_MAXMSGSIZE, &nValue_int64_t, sizeof(int64_t));

        if (strName == "ZMQ_MULTICAST_HOPS")
            nRc = zmq_setsockopt (pSocket, ZMQ_MULTICAST_HOPS, &nValue_int, sizeof(int));

        if (strName == "ZMQ_RCVTIMEO")
            nRc = zmq_setsockopt (pSocket, ZMQ_RCVTIMEO, &nValue_int, sizeof(int));

        if (strName == "ZMQ_SNDTIMEO")
            nRc = zmq_setsockopt (pSocket, ZMQ_SNDTIMEO, &nValue_int, sizeof(int));

        if (strName == "ZMQ_IPV4ONLY")
            nRc = zmq_setsockopt (pSocket, ZMQ_IPV4ONLY, &nValue_int, sizeof(int));

        if (strName == "ZMQ_DELAY_ATTACH_ON_CONNECT")
            nRc = zmq_setsockopt (pSocket, ZMQ_DELAY_ATTACH_ON_CONNECT, &nValue_int, sizeof(int));

        if (strName == "ZMQ_ROUTER_MANDATORY")
            nRc = zmq_setsockopt (pSocket, ZMQ_ROUTER_MANDATORY, &nValue_int, sizeof(int));

        if (strName == "ZMQ_XPUB_VERBOSE")
            nRc = zmq_setsockopt (pSocket, ZMQ_XPUB_VERBOSE, &nValue_int, sizeof(int));

        if (strName == "ZMQ_TCP_KEEPALIVE")
            nRc = zmq_setsockopt (pSocket, ZMQ_TCP_KEEPALIVE, &nValue_int, sizeof(int));

        if (strName == "ZMQ_TCP_KEEPALIVE_IDLE")
            nRc = zmq_setsockopt (pSocket, ZMQ_TCP_KEEPALIVE_IDLE, &nValue_int, sizeof(int));

        if (strName == "ZMQ_TCP_KEEPALIVE_CNT")
            nRc = zmq_setsockopt (pSocket, ZMQ_TCP_KEEPALIVE_CNT, &nValue_int, sizeof(int));

        if (strName == "ZMQ_TCP_KEEPALIVE_INTVL")
            nRc = zmq_setsockopt (pSocket, ZMQ_TCP_KEEPALIVE_INTVL, &nValue_int, sizeof(int));

        if (strName == "ZMQ_TCP_ACCEPT_FILTER")
            nRc = zmq_setsockopt (pSocket, ZMQ_TCP_ACCEPT_FILTER, strValue.c_str(), strValue.length());

        if (nRc == -2)
                printf("ZMQ_%s_SETSOCKOPT: ERROR! %s does not exists as an strOption!\n", pszSocketName, strName.c_str());
        else
            if (nRc < 0)
                printf("ZMQ_%s_SETSOCKOPT: %s value: %s returncode: %i errno: %i strerror: %s\n", pszSocketName, strName.c_str(), strValue.c_str(), nRc, errno, strerror(errno));
            else
                printf("ZMQ_%s_SETSOCKOPT: %s value: %s OK (int: %i, int64_t:%li, uint64_t: %lu)\n", pszSocketName, strName.c_str(), strValue.c_str(), nValue_int, nValue_int64_t, nValue_uint64_t);
    }
}

void BZmq_PubSetOptions(std::string& strOpt) {
    BZmq_SetSocket(pBZmq_socket_pub, "PUB", strOpt);
}

void BZmq_PubBind(std::string& strBindAddr) {
    int nRc = zmq_bind(pBZmq_socket_pub, strBindAddr.c_str());
    printf("ZMQ_BIND Publisher binding to socket: %s ", strBindAddr.c_str());
    if (nRc < 0)
        printf("ERROR errno: %i strerror: %s\n", errno, strerror(errno));
    else
        printf("OK\n");
}

void BZmq_PubConnect(std::string& strConnectAddr) {
    int nRc = zmq_connect(pBZmq_socket_pub, strConnectAddr.c_str());
    printf("ZMQ_CONNECT Publisher connecting to socket: %s", strConnectAddr.c_str());
    if (nRc < 0)
        printf("ERROR errno: %i strerror: %s\n", errno, strerror(errno));
    else
        printf("OK\n");
}

void BZmq_RepSetOptions(std::string& strOpt) {
    BZmq_SetSocket(pBZmq_socket_rep, "REP", strOpt);
}

void BZmq_RepBind(std::string& strBindAddr) {
    int nRc = zmq_bind(pBZmq_socket_rep, strBindAddr.c_str());
    printf("ZMQ_BIND Replier binding to socket: %s ", strBindAddr.c_str());
    if (nRc < 0)
        printf("ERROR errno: %i strerror: %s\n", errno, strerror(errno));
    else
        printf("OK\n");
}

void BZmq_RepConnect(std::string& strConnectAddr) {
    int nRc = zmq_connect(pBZmq_socket_rep, strConnectAddr.c_str());
    printf("ZMQ_CONNECT Replier connecting to socket: %s", strConnectAddr.c_str());
    if (nRc < 0)
        printf("ERROR errno: %i strerror: %s\n", errno, strerror(errno));
    else
        printf("OK\n");
}

void BZmq_Shutdown() {
    printf("BZmq_Shutdown() Begin\n");

    while(fBZmq_ThreadReqRep)
        sleep(1);

    if (pBZmq_socket_pub)
        zmq_close(pBZmq_socket_pub);

    if (pBZmq_ctx)
        zmq_term(pBZmq_ctx);

    printf("BZmq_Shutdown() End\n");
}

void BZmq_Version() {
    int nZmq_version_major, nZmq_version_minor, nZmq_version_patch;
    zmq_version(&nZmq_version_major, &nZmq_version_minor, &nZmq_version_patch);
    printf("Using ZMQ version API: %i.%i.%i Runtime: %i.%i.%i\n", ZMQ_VERSION_MAJOR, ZMQ_VERSION_MINOR, ZMQ_VERSION_PATCH, nZmq_version_major, nZmq_version_minor, nZmq_version_patch);
}

void BZmq_SendTX(CTransaction& tx) {
    json_spirit::Object result;
    TxToJSON(tx, 0, result);

    std::string strTemp = strBZmq_ID_TRANSACTION + json_spirit::write_string(json_spirit::Value(result), false);

    int nRc = zmq_send(pBZmq_socket_pub, strTemp.c_str(), strTemp.length(), ZMQ_DONTWAIT);

    if (nRc < 0)
        printf("ZMQ ERROR in BZmq_SendTX() errno: %i strerror: %s\n", errno, strerror(errno));
}

void BZmq_SendBlock(CBlockIndex* pblockindex) {
    CBlock block;
    block.ReadFromDisk(pblockindex);

    json_spirit::Object result = blockToJSON(block, pblockindex);
    std::string strTemp = strBZmq_ID_BLOCK + json_spirit::write_string(json_spirit::Value(result), false);

    int nRc = zmq_send(pBZmq_socket_pub, strTemp.c_str(), strTemp.length(), ZMQ_DONTWAIT);

    if (nRc < 0)
        printf("ZMQ ERROR in BZmq_SendBlock() errno: %i strerror: %s\n", errno, strerror(errno));
}

void BZmq_SendIPAddress(const char *pszIP) {
    std::string strIP = pszIP;
    std::string::size_type nPortPos = strIP.rfind(":");
    if (nPortPos != std::string::npos) 
    {
        std::string strIPAddress = strIP.substr(0, nPortPos);
        std::string strTemp = strBZmq_ID_IPADDRESS;
        strTemp.append("{\"ip\":\"");
        strTemp.append(strIPAddress);
        strTemp.append("\",\"port\":");
        strTemp.append(strIP.substr(nPortPos + 1));
        strTemp.append("}");

        int nRc = zmq_send(pBZmq_socket_pub, strTemp.c_str(), strTemp.length(), ZMQ_DONTWAIT);

        if (nRc < 0)
            printf("ZMQ ERROR in BZmq_SendIpAddress() errno: %i strerror: %s\n", errno, strerror(errno));
    }
}

void BZmq_ThreadReqRep(void *pArg) {
/*
    This is the Reply socket thread to question which comes from a Request socket.

    To make an long story sort:
    This thread is an an RPC Server - but for 0MQ.

*/
    int nRc, nRc_recv;
    fBZmq_ThreadReqRep = true;
    RenameThread("bitcoin-zmqreqrep");

    zmq_pollitem_t BZmq_pollitems [] = {
        {pBZmq_socket_rep, 0, ZMQ_POLLIN, 0 }
    };

    char *BZmq_buffer = (char *)malloc(BZMQ_BUFFER_SIZE);
    if (BZmq_buffer != NULL) {
        memset(BZmq_buffer, 0, BZMQ_BUFFER_SIZE);

        while(!fShutdown) {
            nRc = zmq_poll (BZmq_pollitems, 1, 0);
            if (nRc == -1) {
                printf("ZMQ ERROR: zmq_poll in BZmq_ThreadReqRep() errno: %i strerror: %s\n", errno, strerror(errno));
                break;
            }

            if (nRc > 0 && !fShutdown)  {
                if (BZmq_pollitems [0].revents & ZMQ_POLLIN) {
                    nRc_recv = zmq_recv (pBZmq_socket_rep, &BZmq_buffer[0], BZMQ_BUFFER_SIZE, 0);
                    if (nRc_recv != -1) {
                        if (nRc_recv == 0 || nRc_recv == BZMQ_BUFFER_SIZE)
                            //we don't care about these...
                            nRc = zmq_send(pBZmq_socket_rep, "", 0, ZMQ_DONTWAIT);
                        else {
                            std::string strCommand = &BZmq_buffer[0];
                            json_spirit::Value jsonCommand;
                            std::string strTemp;
                            if (json_spirit::read_string(strCommand, jsonCommand)) {

                                if (jsonCommand.type() == json_spirit::obj_type) {
                                    json_spirit::Object result = JSONRPCExecOne(jsonCommand);
                                    strTemp = json_spirit::write_string(json_spirit::Value(result), false);
                                } else if (jsonCommand.type() == json_spirit::array_type)
                                    // array of requests
                                    strTemp = JSONRPCExecBatch(jsonCommand.get_array());
                                else
                                    printf("ZMQ ERROR: no jsonCommand.type() found! %s\n", strCommand.c_str());

                                nRc = zmq_send(pBZmq_socket_rep, strTemp.c_str(), strTemp.length(), ZMQ_DONTWAIT);
                                if (nRc < 0)
                                    printf("ZMQ ERROR: zmq_send in BZmq_ThreadReqRep() errno: %i strerror: %s\n", errno, strerror(errno));
                            } else {
                                printf("ZMQ ERROR: read_string failed for command: '%s'\n", strCommand.c_str());
                                nRc = zmq_send(pBZmq_socket_rep, "", 0, ZMQ_DONTWAIT);
                            }
                        }
                        memset(BZmq_buffer, 0, nRc_recv);
                    } else
                        printf("ZMQ ERROR: zmq_recv in BZmq_ThreadReqRep() errno: %i strerror: %s\n", errno, strerror(errno));
                }
            } else 
                //we are only sleeping if we don't have anything to do.
                usleep(10000);
        }
    } else 
        printf("ZMQ ERROR: malloc failed in BZmq_ThreadReqRep() errno: %i strerror: %s\n", errno, strerror(errno));

    if (pBZmq_socket_rep)
        zmq_close(pBZmq_socket_rep);

    fBZmq_ThreadReqRep = false;
}

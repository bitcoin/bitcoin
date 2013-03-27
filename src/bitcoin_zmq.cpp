// Copyright (c) 2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <zmq.h>

#include "hash.h"
#include "bitcoinrpc.h"
#include "main.h"


static void *bz_ctx = NULL;
static void *bz_socket_pub = NULL;
std::string bz_ID_TRANSACTION = "cf954abb";
std::string bz_ID_BLOCK = "ec747b90";
std::string bz_ID_IPADDRESS = "a610612a";


extern void TxToJSON(const CTransaction& tx, const uint256 hashBlock, json_spirit::Object& entry);
extern json_spirit::Object blockToJSON(const CBlock& block, const CBlockIndex* blockindex);


void bz_InitCtx() {
    bz_ctx = zmq_ctx_new();
}

void bz_InitSockets() {
    bz_socket_pub = zmq_socket(bz_ctx, ZMQ_PUB);
}

void bz_CtxSetOptions(std::string& opt) {
    std::string::size_type name_pos = opt.find(":");

    if (name_pos != std::string::npos) 
    {
        std::string name = opt.substr(0, name_pos);
        std::string value = opt.substr(name_pos + 1);
        std::stringstream string_to_ints(value);
        int value_int = 0;
        string_to_ints >> value_int;
        int rc = -2;

        if (name == "ZMQ_IO_THREADS")
            rc = zmq_ctx_set(bz_ctx, ZMQ_IO_THREADS, value_int);
        if (name ==  "ZMQ_MAX_SOCKETS")
            rc = zmq_ctx_set(bz_ctx, ZMQ_MAX_SOCKETS, value_int);

        if (rc == -2)
                printf("ZMQ_CTX_SET: ERROR! %s does not exists as an option!\n", name.c_str());
        else
            if (rc < 0)
                printf("ZMQ_CTX_SET: %s value: %i returncode: %i errno: %i strerror: %s\n", name.c_str(), value_int, rc, errno, strerror(errno));
            else
                printf("ZMQ_CTX_SET: %s value: %i OK\n", name.c_str(), value_int);
    }
}

void bz_SetSocket(void *socket, const char *socket_name, std::string& opt) {
    std::string::size_type name_pos = opt.find(":");

    if (name_pos != std::string::npos) 
    {
        std::string name = opt.substr(0, name_pos);
        std::string value = opt.substr(name_pos + 1);
        std::stringstream string_to_ints(value);
        int rc = -2;
        int value_int = 0; 
        string_to_ints >> value_int;
        uint64_t value_uint64_t = 0;
        string_to_ints >> value_uint64_t;
        int64_t value_int64_t = 0;
        string_to_ints >> value_int64_t;

        if (name == "ZMQ_SNDHWM")
            rc = zmq_setsockopt (socket, ZMQ_SNDHWM, &value_int, sizeof(int));

        if (name == "ZMQ_RCVHWM")
            rc = zmq_setsockopt (socket, ZMQ_RCVHWM, &value_int, sizeof(int));

        if (name == "ZMQ_AFFINITY")
            rc = zmq_setsockopt (socket, ZMQ_AFFINITY, &value_uint64_t, sizeof(uint64_t));

        if (name == "ZMQ_SUBSCRIBE")
            rc = zmq_setsockopt (socket, ZMQ_SUBSCRIBE, value.c_str(), value.length());

        if (name == "ZMQ_UNSUBSCRIBE")
            rc = zmq_setsockopt (socket, ZMQ_UNSUBSCRIBE, value.c_str(), value.length());

        if (name == "ZMQ_IDENTITY")
            rc = zmq_setsockopt (socket, ZMQ_IDENTITY, value.c_str(), value.length());

        if (name == "ZMQ_RATE")
            rc = zmq_setsockopt (socket, ZMQ_RATE, &value_int, sizeof(int));

        if (name == "ZMQ_RECOVERY_IVL")
            rc = zmq_setsockopt (socket, ZMQ_RECOVERY_IVL, &value_int, sizeof(int));

        if (name == "ZMQ_SNDBUF")
            rc = zmq_setsockopt (socket, ZMQ_SNDBUF, &value_int, sizeof(int));

        if (name == "ZMQ_RCVBUF")
            rc = zmq_setsockopt (socket, ZMQ_RCVBUF, &value_int, sizeof(int));

        if (name == "ZMQ_LINGER")
            rc = zmq_setsockopt (socket, ZMQ_LINGER, &value_int, sizeof(int));

        if (name == "ZMQ_RECONNECT_IVL")
            rc = zmq_setsockopt (socket, ZMQ_RECONNECT_IVL, &value_int, sizeof(int));

        if (name == "ZMQ_RECONNECT_IVL_MAX")
            rc = zmq_setsockopt (socket, ZMQ_RECONNECT_IVL_MAX, &value_int, sizeof(int));

        if (name == "ZMQ_BACKLOG")
            rc = zmq_setsockopt (socket, ZMQ_BACKLOG, &value_int, sizeof(int));

        if (name == "ZMQ_MAXMSGSIZE")
            rc = zmq_setsockopt (socket, ZMQ_MAXMSGSIZE, &value_int64_t, sizeof(int64_t));

        if (name == "ZMQ_MULTICAST_HOPS")
            rc = zmq_setsockopt (socket, ZMQ_MULTICAST_HOPS, &value_int, sizeof(int));

        if (name == "ZMQ_RCVTIMEO")
            rc = zmq_setsockopt (socket, ZMQ_RCVTIMEO, &value_int, sizeof(int));

        if (name == "ZMQ_SNDTIMEO")
            rc = zmq_setsockopt (socket, ZMQ_SNDTIMEO, &value_int, sizeof(int));

        if (name == "ZMQ_IPV4ONLY")
            rc = zmq_setsockopt (socket, ZMQ_IPV4ONLY, &value_int, sizeof(int));

        if (name == "ZMQ_DELAY_ATTACH_ON_CONNECT")
            rc = zmq_setsockopt (socket, ZMQ_DELAY_ATTACH_ON_CONNECT, &value_int, sizeof(int));

        if (name == "ZMQ_ROUTER_MANDATORY")
            rc = zmq_setsockopt (socket, ZMQ_ROUTER_MANDATORY, &value_int, sizeof(int));

        if (name == "ZMQ_XPUB_VERBOSE")
            rc = zmq_setsockopt (socket, ZMQ_XPUB_VERBOSE, &value_int, sizeof(int));

        if (name == "ZMQ_TCP_KEEPALIVE")
            rc = zmq_setsockopt (socket, ZMQ_TCP_KEEPALIVE, &value_int, sizeof(int));

        if (name == "ZMQ_TCP_KEEPALIVE_IDLE")
            rc = zmq_setsockopt (socket, ZMQ_TCP_KEEPALIVE_IDLE, &value_int, sizeof(int));

        if (name == "ZMQ_TCP_KEEPALIVE_CNT")
            rc = zmq_setsockopt (socket, ZMQ_TCP_KEEPALIVE_CNT, &value_int, sizeof(int));

        if (name == "ZMQ_TCP_KEEPALIVE_INTVL")
            rc = zmq_setsockopt (socket, ZMQ_TCP_KEEPALIVE_INTVL, &value_int, sizeof(int));

        if (name == "ZMQ_TCP_ACCEPT_FILTER")
            rc = zmq_setsockopt (socket, ZMQ_TCP_ACCEPT_FILTER, value.c_str(), value.length());

        if (rc == -2)
                printf("ZMQ_%s_SETSOCKOPT: ERROR! %s does not exists as an option!\n", socket_name, name.c_str());
        else
            if (rc < 0)
                printf("ZMQ_%s_SETSOCKOPT: %s value: %s returncode: %i errno: %i strerror: %s\n", socket_name, name.c_str(), value.c_str(), rc, errno, strerror(errno));
            else
                printf("ZMQ_%s_SETSOCKOPT: %s value: %s OK (int: %i, int64_t:%li, uint64_t: %lu)\n", socket_name, name.c_str(), value.c_str(), value_int, value_int64_t, value_uint64_t);
    }
}

void bz_PubSetOptions(std::string& opt) {
    bz_SetSocket(bz_socket_pub, "PUB", opt);
}

void bz_PubBind(std::string& bind_addr) {
    int rc = zmq_bind(bz_socket_pub, bind_addr.c_str());
    printf("ZMQ_BIND Publisher binding to socket: %s ", bind_addr.c_str());
    if (rc < 0)
        printf("ERROR errno: %i strerror: %s\n", errno, strerror(errno));
    else
        printf("OK\n");
}

void bz_PubConnect(std::string& connect_addr) {
    int rc = zmq_connect(bz_socket_pub, connect_addr.c_str());
    printf("ZMQ_CONNECT Publisher connecting to socket: %s", connect_addr.c_str());
    if (rc < 0)
        printf("ERROR errno: %i strerror: %s\n", errno, strerror(errno));
    else
        printf("OK\n");
}

void bz_Shutdown() {
    printf("ZMQ Shutdown() Begin\n");

    if (bz_socket_pub)
        zmq_close(bz_socket_pub);

    if (bz_ctx)
        zmq_term(bz_ctx);

    printf("ZMQ Shutdown() End\n");
}

void bz_Version() {
    int z_version_major, z_version_minor, z_version_patch;
    zmq_version(&z_version_major, &z_version_minor, &z_version_patch);
    printf("Using ZMQ version API: %i.%i.%i Runtime: %i.%i.%i\n", ZMQ_VERSION_MAJOR, ZMQ_VERSION_MINOR, ZMQ_VERSION_PATCH, z_version_major, z_version_minor, z_version_patch);
}

void bz_Send_TX(CTransaction& tx) {
    json_spirit::Object result;
    TxToJSON(tx, 0, result);

    std::string s = bz_ID_TRANSACTION + json_spirit::write_string(json_spirit::Value(result), false);

    int rc = zmq_send(bz_socket_pub, s.c_str(), s.length(), ZMQ_DONTWAIT);

    if (rc < 0)
        printf("ZMQ ERROR in bz_Send_TX() errno: %i strerror: %s\n", errno, strerror(errno));
}

void bz_Send_Block(CBlockIndex* pblockindex) {
    CBlock block;
    block.ReadFromDisk(pblockindex);

    json_spirit::Object result = blockToJSON(block, pblockindex);
    std::string s = bz_ID_BLOCK + json_spirit::write_string(json_spirit::Value(result), false);

    int rc = zmq_send(bz_socket_pub, s.c_str(), s.length(), ZMQ_DONTWAIT);

    if (rc < 0)
        printf("ZMQ ERROR in bz_Send_Block() errno: %i strerror: %s\n", errno, strerror(errno));
}

void bz_Send_IpAddress(const char *ip) {
    std::string i = ip;
    std::string::size_type port_pos = i.rfind(":");
    if (port_pos != std::string::npos) 
    {
        std::string ipaddress = i.substr(0, port_pos);
        std::string s = bz_ID_IPADDRESS;
        s.append("{\"ip\":\"");
        s.append(ipaddress);
        s.append("\",\"port\":");
        s.append(i.substr(port_pos + 1));
        s.append("}");

        int rc = zmq_send(bz_socket_pub, s.c_str(), s.length(), ZMQ_DONTWAIT);

        if (rc < 0)
            printf("ZMQ ERROR in bz_Send_IpAddress() errno: %i strerror: %s\n", errno, strerror(errno));
    }
}

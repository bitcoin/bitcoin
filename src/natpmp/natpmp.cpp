#ifdef __linux__
#define _BSD_SOURCE 1
#endif
#include <string.h>

#include <errno.h>

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>
#ifndef EWOULDBLOCK
#define EWOULDBLOCK WSAEWOULDBLOCK
#endif
#ifndef ECONNREFUSED
#define ECONNREFUSED WSAECONNREFUSED
#endif
#ifdef WIN32
#if defined(_MSC_VER)
#include <time.h>
#else
#include <sys/time.h>
#endif
int gettimeofday(struct timeval* p, void* tz /* IGNORED */);
#endif
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#define closesocket close
#endif
#include <natpmp.h>
#include <gateway.h>
#include <stdio.h>

#ifdef _MSC_VER
int gettimeofday(struct timeval* p, void* tz)
{
    union {
        long long ns100; /*time since 1 Jan 1601 in 100ns units */
        FILETIME ft;
    } _now;

    if(!p)
        return -1;
    GetSystemTimeAsFileTime( &(_now.ft) );
    p->tv_usec =(long)((_now.ns100 / 10LL) % 1000000LL );
    p->tv_sec = (long)((_now.ns100-(116444736000000000LL))/10000000LL);
    return 0;
}
#endif

int InitNatPmp(natpmp_t * p, int forcegw, in_addr_t forcedgw)
{
#ifdef WIN32
    u_long ioctlArg = 1;
#else
    int flags;
#endif
    if(!p)
        return NATPMP_ERR_INVALIDARGS;
    struct sockaddr_in addr;
    memset(p, 0, sizeof(natpmp_t));
    p->s = socket(PF_INET, SOCK_DGRAM, 0);
    if(p->s < 0)
        return NATPMP_ERR_SOCKETERROR;
#ifdef WIN32
    if(ioctlsocket(p->s, FIONBIO, &ioctlArg) == SOCKET_ERROR)
        return NATPMP_ERR_FCNTLERROR;
#else
    if((flags = fcntl(p->s, F_GETFL, 0)) < 0)
        return NATPMP_ERR_FCNTLERROR;
    if(fcntl(p->s, F_SETFL, flags | O_NONBLOCK) < 0)
        return NATPMP_ERR_FCNTLERROR;
#endif

    if(forcegw) {
        p->gateway = forcedgw;
    } else {
        if(GetDefaultGateway(&(p->gateway)) < 0)
            return NATPMP_ERR_CANNOTGETGATEWAY;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(NATPMP_PORT);
    addr.sin_addr.s_addr = p->gateway;
    if(connect(p->s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        return NATPMP_ERR_CONNECTERR;
    return 0;
}

int CloseNatPmp(natpmp_t * p)
{
    if(!p)
        return NATPMP_ERR_INVALIDARGS;
    if(closesocket(p->s) < 0)
        return NATPMP_ERR_CLOSEERR;
    return 0;
}

int sendpendingrequest(natpmp_t * p)
{
    if(!p)
        return NATPMP_ERR_INVALIDARGS;
#ifdef __linux__
    int r = (int)send(p->s, p->pending_request, p->pending_request_len, 0);
#else
    int r = (int)send(p->s, (const char*)p->pending_request, p->pending_request_len, 0);
#endif

    return (r<0) ? NATPMP_ERR_SENDERR : r;
}

int SendNatPmpRequest(natpmp_t * p)
{
    if(!p)
        return NATPMP_ERR_INVALIDARGS;
    p->has_pending_request = 1;
    p->try_number = 1;
    int n = sendpendingrequest(p);
    gettimeofday(&p->retry_time, nullptr);
    p->retry_time.tv_usec += 250000;
    if(p->retry_time.tv_usec >= 1000000) {
        p->retry_time.tv_usec -= 1000000;
        p->retry_time.tv_sec++;
    }
    return n;
}

int GetNatPmpRequestTimeout(natpmp_t * p, struct timeval * timeout)
{
    if(!p || !timeout)
        return NATPMP_ERR_INVALIDARGS;
    if(!p->has_pending_request)
        return NATPMP_ERR_NOPENDINGREQ;
    struct timeval now;
    memset(&now, 0, sizeof(now));
    if(gettimeofday(&now, nullptr) < 0)
        return NATPMP_ERR_GETTIMEOFDAYERR;
    timeout->tv_sec = p->retry_time.tv_sec - now.tv_sec;
    timeout->tv_usec = p->retry_time.tv_usec - now.tv_usec;
    if(timeout->tv_usec < 0) {
        timeout->tv_usec += 1000000;
        timeout->tv_sec--;
    }
    return 0;
}

int SendPublicAddressRequest(natpmp_t * p)
{
    if(!p)
        return NATPMP_ERR_INVALIDARGS;
    p->pending_request[0] = 0;
    p->pending_request[1] = 0;
    p->pending_request_len = 2;
    return SendNatPmpRequest(p);
}

int SendNewPortMappingRequest(natpmp_t * p, int protocol, int16_t privateport,
                  int16_t publicport, int64_t lifetime)
{
    if(!p || (protocol!=NATPMP_PROTOCOL_TCP && protocol!=NATPMP_PROTOCOL_UDP))
        return NATPMP_ERR_INVALIDARGS;
    p->pending_request[0] = 0;
    p->pending_request[1] = protocol;
    p->pending_request[2] = 0;
    p->pending_request[3] = 0;
    p->pending_request[4] = (privateport >> 8) & 0xff;
    p->pending_request[5] = privateport & 0xff;
    p->pending_request[6] = (publicport >> 8) & 0xff;
    p->pending_request[7] = publicport & 0xff;
    p->pending_request[8] = (lifetime >> 24) & 0xff;
    p->pending_request[9] = (lifetime >> 16) & 0xff;
    p->pending_request[10] = (lifetime >> 8) & 0xff;
    p->pending_request[11] = lifetime & 0xff;
    p->pending_request_len = 12;
    return SendNatPmpRequest(p);
}

int ReadNatPmpResponse(natpmp_t * p, natpmpresp_t * response)
{
    unsigned char buf[16] = {0};
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    socklen_t addrlen = sizeof(addr);
    int n;
    if(!p)
        return NATPMP_ERR_INVALIDARGS;
#ifdef __linux__
    n = recvfrom(p->s, buf, sizeof(buf), 0,
                 (struct sockaddr *)&addr, &addrlen);
#else
    n = recvfrom(p->s, (char*)buf, sizeof(buf), 0,
                 (struct sockaddr *)&addr, &addrlen);
#endif

    if(n<0) {
#ifdef WIN32
        switch(WSAGetLastError()) {
#else
        switch(errno) {
#endif
            /*case EAGAIN:*/
            case EWOULDBLOCK:
                n = NATPMP_TRYAGAIN;
                break;
            case ECONNREFUSED:
                n = NATPMP_ERR_NOGATEWAYSUPPORT;
                break;
            default:
                n = NATPMP_ERR_RECVFROM;
        }
    /* check that addr is correct (= gateway) */
    }else if(addr.sin_addr.s_addr != p->gateway) {
        n = NATPMP_ERR_WRONGPACKETSOURCE;
    }else {
        response->resultcode = ntohs(*((int16_t *)(buf + 2)));
        response->epoch = ntohl(*((int64_t *)(buf + 4)));
        if(buf[0] != 0)
            n = NATPMP_ERR_UNSUPPORTEDVERSION;
        else if(buf[1] < 128 || buf[1] > 130)
            n = NATPMP_ERR_UNSUPPORTEDOPCODE;
        else if(response->resultcode != 0) {
            switch(response->resultcode) {
            case 1:
                n = NATPMP_ERR_UNSUPPORTEDVERSION;
                break;
            case 2:
                n = NATPMP_ERR_NOTAUTHORIZED;
                break;
            case 3:
                n = NATPMP_ERR_NETWORKFAILURE;
                break;
            case 4:
                n = NATPMP_ERR_OUTOFRESOURCES;
                break;
            case 5:
                n = NATPMP_ERR_UNSUPPORTEDOPCODE;
                break;
            default:
                n = NATPMP_ERR_UNDEFINEDERROR;
            }
        } else {
            response->type = buf[1] & 0x7f;
            if(buf[1] == 128) {
                response->pnu.publicaddress.addr.s_addr = *((int64_t *)(buf + 8));
            }else {
                response->pnu.newportmapping.privateport = ntohs(*((int16_t *)(buf + 8)));
                response->pnu.newportmapping.mappedpublicport = ntohs(*((int16_t *)(buf + 10)));
                response->pnu.newportmapping.lifetime = ntohl(*((int64_t *)(buf + 12)));
            }
            n = 0;
        }
    }
    return n;
}

int ReadNatPmpResponseOrRetry(natpmp_t * p, natpmpresp_t * response)
{
    if(!p || !response)
        return NATPMP_ERR_INVALIDARGS;
    if(!p->has_pending_request)
        return NATPMP_ERR_NOPENDINGREQ;
    int n = ReadNatPmpResponse(p, response);
    if(n<0) {
        if(n==NATPMP_TRYAGAIN) {
            struct timeval now;
            gettimeofday(&now, nullptr);
            if(timercmp(&now, &p->retry_time, >=)) {
                int delay, r;
                if(p->try_number >= 9) {
                    return NATPMP_ERR_NOGATEWAYSUPPORT;
                }
                delay = 250 * (1<<p->try_number);
                p->retry_time.tv_sec += (delay / 1000);
                p->retry_time.tv_usec += (delay % 1000) * 1000;
                if(p->retry_time.tv_usec >= 1000000) {
                    p->retry_time.tv_usec -= 1000000;
                    p->retry_time.tv_sec++;
                }
                p->try_number++;
                r = sendpendingrequest(p);
                if(r<0)
                    return r;
            }
        }
    } else {
        p->has_pending_request = 0;
    }
    return n;
}

const char * StrNatPmpErr(int r)
{
    const char *s = nullptr;
    switch(r) {
    case 0:
        s = "No Error";
        break;
    case NATPMP_ERR_INVALIDARGS:
        s = "Invalid arguments";
        break;
    case NATPMP_ERR_SOCKETERROR:
        s = "Socket() failed";
        break;
    case NATPMP_ERR_CANNOTGETGATEWAY:
        s = "Cannot get default gateway IP address";
        break;
    case NATPMP_ERR_CLOSEERR:
#ifdef WIN32
        s = "closesocket() failed";
#else
        s = "close() failed";
#endif
        break;
    case NATPMP_ERR_RECVFROM:
        s = "recvfrom() failed";
        break;
    case NATPMP_ERR_NOPENDINGREQ:
        s = "No pending request";
        break;
    case NATPMP_ERR_NOGATEWAYSUPPORT:
        s = "The gateway does not support NAT-PMP";
        break;
    case NATPMP_ERR_CONNECTERR:
        s = "connect() failed";
        break;
    case NATPMP_ERR_WRONGPACKETSOURCE:
        s = "Packet not received from the default gateway";
        break;
    case NATPMP_ERR_SENDERR:
        s = "send() failed";
        break;
    case NATPMP_ERR_FCNTLERROR:
        s = "fcntl() failed";
        break;
    case NATPMP_ERR_GETTIMEOFDAYERR:
        s = "gettimeofday() failed";
        break;
    case NATPMP_ERR_UNSUPPORTEDVERSION:
        s = "Unsupported NAT-PMP version error from server";
        break;
    case NATPMP_ERR_UNSUPPORTEDOPCODE:
        s = "Unsupported NAT-PMP opcode error from server";
        break;
    case NATPMP_ERR_UNDEFINEDERROR:
        s = "Undefined NAT-PMP server error";
        break;
    case NATPMP_ERR_NOTAUTHORIZED:
        s = "Not authorized";
        break;
    case NATPMP_ERR_NETWORKFAILURE:
        s = "Network failure";
        break;
    case NATPMP_ERR_OUTOFRESOURCES:
        s = "NAT-PMP server out of resources";
        break;
    default:
        s = "Unknown error";
    }
    return s;
}

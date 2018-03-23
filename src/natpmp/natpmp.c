/* $Id: natpmp.c,v 1.18 2013/11/26 08:47:36 nanard Exp $ */
/* libnatpmp
Copyright (c) 2007-2013, Thomas BERNARD
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * The name of the author may not be used to endorse or promote products
      derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#ifdef __linux__
#define _BSD_SOURCE 1
#endif
#include <string.h>
#include <time.h>
#if !defined(_MSC_VER)
#include <sys/time.h>
#endif
#ifdef WIN32
#include <errno.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>
#ifndef EWOULDBLOCK
#define EWOULDBLOCK WSAEWOULDBLOCK
#endif
#ifndef ECONNREFUSED
#define ECONNREFUSED WSAECONNREFUSED
#endif
#include <natpmp/wingettimeofday.h>
#define gettimeofday natpmp_gettimeofday
#else
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#define closesocket close
#endif
#include <natpmp/natpmp.h>
#include <natpmp/getgateway.h>
#include <stdio.h>
#include <stdlib.h>

NATPMP_LIBSPEC int initnatpmp(natpmp_t * p, int forcegw, in_addr_t forcedgw, const char *port)
{
#ifdef WIN32
    u_long ioctlArg = 1;
#else
    int flags;
#endif
    struct sockaddr_in addr;
    if(!p)
        return NATPMP_ERR_INVALIDARGS;
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
        if(getdefaultgateway(&(p->gateway)) < 0)
            return NATPMP_ERR_CANNOTGETGATEWAY;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = atoi(port);
    addr.sin_addr.s_addr = p->gateway;
    if(connect(p->s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        return NATPMP_ERR_CONNECTERR;
    return 0;
}

NATPMP_LIBSPEC int closenatpmp(natpmp_t * p)
{
    if(!p)
        return NATPMP_ERR_INVALIDARGS;
    if(closesocket(p->s) < 0)
        return NATPMP_ERR_CLOSEERR;
    return 0;
}

int sendpendingrequest(natpmp_t * p)
{
    int r;
/*    struct sockaddr_in addr;*/
    if(!p)
        return NATPMP_ERR_INVALIDARGS;
/*    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(NATPMP_PORT);
    addr.sin_addr.s_addr = p->gateway;
    r = (int)sendto(p->s, p->pending_request, p->pending_request_len, 0,
                       (struct sockaddr *)&addr, sizeof(addr));*/
    r = (int)send(p->s, p->pending_request, p->pending_request_len, 0);
    return (r<0) ? NATPMP_ERR_SENDERR : r;
}

int sendnatpmprequest(natpmp_t * p)
{
    int n;
    if(!p)
        return NATPMP_ERR_INVALIDARGS;
    /* TODO : check if no request is allready pending */
    p->has_pending_request = 1;
    p->try_number = 1;
    n = sendpendingrequest(p);
    gettimeofday(&p->retry_time, NULL);    // check errors !
    p->retry_time.tv_usec += 250000;    /* add 250ms */
    if(p->retry_time.tv_usec >= 1000000) {
        p->retry_time.tv_usec -= 1000000;
        p->retry_time.tv_sec++;
    }
    return n;
}

NATPMP_LIBSPEC int getnatpmprequesttimeout(natpmp_t * p, struct timeval * timeout)
{
    struct timeval now;
    if(!p || !timeout)
        return NATPMP_ERR_INVALIDARGS;
    if(!p->has_pending_request)
        return NATPMP_ERR_NOPENDINGREQ;
    if(gettimeofday(&now, NULL) < 0)
        return NATPMP_ERR_GETTIMEOFDAYERR;
    timeout->tv_sec = p->retry_time.tv_sec - now.tv_sec;
    timeout->tv_usec = p->retry_time.tv_usec - now.tv_usec;
    if(timeout->tv_usec < 0) {
        timeout->tv_usec += 1000000;
        timeout->tv_sec--;
    }
    return 0;
}

NATPMP_LIBSPEC int sendpublicaddressrequest(natpmp_t * p)
{
    if(!p)
        return NATPMP_ERR_INVALIDARGS;
    //static const unsigned char request[] = { 0, 0 };
    p->pending_request[0] = 0;
    p->pending_request[1] = 0;
    p->pending_request_len = 2;
    // TODO: return 0 instead of sizeof(request) ??
    return sendnatpmprequest(p);
}

NATPMP_LIBSPEC int sendnewportmappingrequest(natpmp_t * p, uint16_t privateport, 
                              uint16_t publicport, uint32_t lifetime)
{
    if(!p)
        return NATPMP_ERR_INVALIDARGS;
    p->pending_request[0] = 0;
    p->pending_request[1] = 1;  /* TCP only. */
    p->pending_request[2] = 0;
    p->pending_request[3] = 0;
    /* break strict-aliasing rules :
    *((uint16_t *)(p->pending_request + 4)) = htons(privateport); */
    p->pending_request[4] = (privateport >> 8) & 0xff;
    p->pending_request[5] = privateport & 0xff;
    /* break stric-aliasing rules :
    *((uint16_t *)(p->pending_request + 6)) = htons(publicport); */
    p->pending_request[6] = (publicport >> 8) & 0xff;
    p->pending_request[7] = publicport & 0xff;
    /* break stric-aliasing rules :
    *((uint32_t *)(p->pending_request + 8)) = htonl(lifetime); */
    p->pending_request[8] = (lifetime >> 24) & 0xff;
    p->pending_request[9] = (lifetime >> 16) & 0xff;
    p->pending_request[10] = (lifetime >> 8) & 0xff;
    p->pending_request[11] = lifetime & 0xff;
    p->pending_request_len = 12;
    return sendnatpmprequest(p);
}

NATPMP_LIBSPEC int readnatpmpresponse(natpmp_t * p, natpmpresp_t * response)
{
    unsigned char buf[16];
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    int n;
    if(!p)
        return NATPMP_ERR_INVALIDARGS;
    n = recvfrom(p->s, buf, sizeof(buf), 0,
                 (struct sockaddr *)&addr, &addrlen);
    if(n<0)
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
    else if(addr.sin_addr.s_addr != p->gateway)
        n = NATPMP_ERR_WRONGPACKETSOURCE;
    else {
        response->resultcode = ntohs(*((uint16_t *)(buf + 2)));
        response->epoch = ntohl(*((uint32_t *)(buf + 4)));
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
            if(buf[1] == 128)
                //response->publicaddress.addr = *((uint32_t *)(buf + 8));
                response->pnu.publicaddress.addr.s_addr = *((uint32_t *)(buf + 8));
            else {
                response->pnu.newportmapping.privateport = ntohs(*((uint16_t *)(buf + 8)));
                response->pnu.newportmapping.mappedpublicport = ntohs(*((uint16_t *)(buf + 10)));
                response->pnu.newportmapping.lifetime = ntohl(*((uint32_t *)(buf + 12)));
            }
            n = 0;
        }
    }
    return n;
}

NATPMP_LIBSPEC int readnatpmpresponseorretry(natpmp_t * p, natpmpresp_t * response)
{
    int n;
    if(!p || !response)
        return NATPMP_ERR_INVALIDARGS;
    if(!p->has_pending_request)
        return NATPMP_ERR_NOPENDINGREQ;
    n = readnatpmpresponse(p, response);
    if(n<0) {
        if(n==NATPMP_TRYAGAIN) {
            struct timeval now;
            gettimeofday(&now, NULL);    // check errors !
            if(timercmp(&now, &p->retry_time, >=)) {
                int delay, r;
                if(p->try_number >= 9) {
                    return NATPMP_ERR_NOGATEWAYSUPPORT;
                }
                /*printf("retry! %d\n", p->try_number);*/
                delay = 250 * (1<<p->try_number);    // ms
                /*for(i=0; i<p->try_number; i++)
                    delay += delay;*/
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

#ifdef ENABLE_STRNATPMPERR
NATPMP_LIBSPEC const char * strnatpmperr(int r)
{
    const char * s;
    switch(r) {
    case NATPMP_ERR_INVALIDARGS:
        s = "invalid arguments";
        break;
    case NATPMP_ERR_SOCKETERROR:
        s = "socket() failed";
        break;
    case NATPMP_ERR_CANNOTGETGATEWAY:
        s = "cannot get default gateway ip address";
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
        s = "no pending request";
        break;
    case NATPMP_ERR_NOGATEWAYSUPPORT:
        s = "the gateway does not support nat-pmp";
        break;
    case NATPMP_ERR_CONNECTERR:
        s = "connect() failed";
        break;
    case NATPMP_ERR_WRONGPACKETSOURCE:
        s = "packet not received from the default gateway";
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
        s = "unsupported nat-pmp version error from server";
        break;
    case NATPMP_ERR_UNSUPPORTEDOPCODE:
        s = "unsupported nat-pmp opcode error from server";
        break;
    case NATPMP_ERR_UNDEFINEDERROR:
        s = "undefined nat-pmp server error";
        break;
    case NATPMP_ERR_NOTAUTHORIZED:
        s = "not authorized";
        break;
    case NATPMP_ERR_NETWORKFAILURE:
        s = "network failure";
        break;
    case NATPMP_ERR_OUTOFRESOURCES:
        s = "nat-pmp server out of resources";
        break;
    default:
        s = "Unknown libnatpmp error";
    }
    return s;
}
#endif


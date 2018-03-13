/* $Id: natpmp.h,v 1.19 2014/04/01 09:39:29 nanard Exp $ */
/* libnatpmp
Copyright (c) 2007-2014, Thomas BERNARD
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

#ifndef __NATPMP_H__
#define __NATPMP_H__

#include <time.h>

#if !defined(_MSC_VER)
#include <sys/time.h>
#endif

#ifdef WIN32
#include <winsock2.h>
#if !defined(_MSC_VER) || _MSC_VER >= 1600
#include <stdint.h>
#else    /* !defined(_MSC_VER) || _MSC_VER >= 1600 */
typedef unsigned long uint32_t;
typedef unsigned short uint16_t;
#endif    /* !defined(_MSC_VER) || _MSC_VER >= 1600 */
#define in_addr_t uint32_t
#else
#include <netinet/in.h>
#endif

#include "natpmp_declspec.h"

typedef struct {
    int s;    /* socket */
    in_addr_t gateway;    /* default gateway (IPv4) */
    int has_pending_request;
    unsigned char pending_request[12];
    int pending_request_len;
    int try_number;
    struct timeval retry_time;
} natpmp_t;

typedef struct {
    uint16_t type;    /* NATPMP_RESPTYPE_* */
    uint16_t resultcode;    /* NAT-PMP response code */
    uint32_t epoch;    /* Seconds since start of epoch */
    union {
        struct {
            //in_addr_t addr;
            struct in_addr addr;
        } publicaddress;
        struct {
            uint16_t privateport;
            uint16_t mappedpublicport;
            uint32_t lifetime;
        } newportmapping;
    } pnu;
} natpmpresp_t;

/* possible values for type field of natpmpresp_t */
#define NATPMP_RESPTYPE_PUBLICADDRESS (0)
// #define NATPMP_RESPTYPE_UDPPORTMAPPING (1)
#define NATPMP_RESPTYPE_TCPPORTMAPPING (1)

/* return values */
/* NATPMP_ERR_INVALIDARGS : invalid arguments passed to the function */
#define NATPMP_ERR_INVALIDARGS (-1)
/* NATPMP_ERR_SOCKETERROR : socket() failed. check errno for details */
#define NATPMP_ERR_SOCKETERROR (-2)
/* NATPMP_ERR_CANNOTGETGATEWAY : can't get default gateway IP */
#define NATPMP_ERR_CANNOTGETGATEWAY (-3)
/* NATPMP_ERR_CLOSEERR : close() failed. check errno for details */
#define NATPMP_ERR_CLOSEERR (-4)
/* NATPMP_ERR_RECVFROM : recvfrom() failed. check errno for details */
#define NATPMP_ERR_RECVFROM (-5)
/* NATPMP_ERR_NOPENDINGREQ : readnatpmpresponseorretry() called while
 * no NAT-PMP request was pending */
#define NATPMP_ERR_NOPENDINGREQ (-6)
/* NATPMP_ERR_NOGATEWAYSUPPORT : the gateway does not support NAT-PMP */
#define NATPMP_ERR_NOGATEWAYSUPPORT (-7)
/* NATPMP_ERR_CONNECTERR : connect() failed. check errno for details */
#define NATPMP_ERR_CONNECTERR (-8)
/* NATPMP_ERR_WRONGPACKETSOURCE : packet not received from the network gateway */
#define NATPMP_ERR_WRONGPACKETSOURCE (-9)
/* NATPMP_ERR_SENDERR : send() failed. check errno for details */
#define NATPMP_ERR_SENDERR (-10)
/* NATPMP_ERR_FCNTLERROR : fcntl() failed. check errno for details */
#define NATPMP_ERR_FCNTLERROR (-11)
/* NATPMP_ERR_GETTIMEOFDAYERR : gettimeofday() failed. check errno for details */
#define NATPMP_ERR_GETTIMEOFDAYERR (-12)

/* */
#define NATPMP_ERR_UNSUPPORTEDVERSION (-14)
#define NATPMP_ERR_UNSUPPORTEDOPCODE (-15)

/* Errors from the server : */
#define NATPMP_ERR_UNDEFINEDERROR (-49)
#define NATPMP_ERR_NOTAUTHORIZED (-51)
#define NATPMP_ERR_NETWORKFAILURE (-52)
#define NATPMP_ERR_OUTOFRESOURCES (-53)

/* NATPMP_TRYAGAIN : no data available for the moment. try again later */
#define NATPMP_TRYAGAIN (-100)

#ifdef __cplusplus
extern "C" {
#endif

/* initnatpmp()
 * initialize a natpmp_t object
 * With forcegw=1 the gateway is not detected automaticaly.
 * Return values :
 * 0 = OK
 * NATPMP_ERR_INVALIDARGS
 * NATPMP_ERR_SOCKETERROR
 * NATPMP_ERR_FCNTLERROR
 * NATPMP_ERR_CANNOTGETGATEWAY
 * NATPMP_ERR_CONNECTERR */
NATPMP_LIBSPEC int initnatpmp(natpmp_t * p, int forcegw, in_addr_t forcedgw, const char *port);

/* closenatpmp()
 * close resources associated with a natpmp_t object
 * Return values :
 * 0 = OK
 * NATPMP_ERR_INVALIDARGS
 * NATPMP_ERR_CLOSEERR */
NATPMP_LIBSPEC int closenatpmp(natpmp_t * p);

/* sendpublicaddressrequest()
 * send a public address NAT-PMP request to the network gateway
 * Return values :
 * 2 = OK (size of the request)
 * NATPMP_ERR_INVALIDARGS
 * NATPMP_ERR_SENDERR */
NATPMP_LIBSPEC int sendpublicaddressrequest(natpmp_t * p);

/* sendnewportmappingrequest()
 * send a new port mapping NAT-PMP request to the network gateway
 * Arguments :
 * protocol is either NATPMP_PROTOCOL_TCP or NATPMP_PROTOCOL_UDP,
 * lifetime is in seconds.
 * To remove a port mapping, set lifetime to zero.
 * To remove all port mappings to the host, set lifetime and both ports
 * to zero.
 * Return values :
 * 12 = OK (size of the request)
 * NATPMP_ERR_INVALIDARGS
 * NATPMP_ERR_SENDERR */
NATPMP_LIBSPEC int sendnewportmappingrequest(natpmp_t * p,
                              uint16_t privateport, uint16_t publicport,
                              uint32_t lifetime);

/* getnatpmprequesttimeout()
 * fills the timeval structure with the timeout duration of the
 * currently pending NAT-PMP request.
 * Return values :
 * 0 = OK
 * NATPMP_ERR_INVALIDARGS
 * NATPMP_ERR_GETTIMEOFDAYERR
 * NATPMP_ERR_NOPENDINGREQ */
NATPMP_LIBSPEC int getnatpmprequesttimeout(natpmp_t * p, struct timeval * timeout);

/* readnatpmpresponseorretry()
 * fills the natpmpresp_t structure if possible
 * Return values :
 * 0 = OK
 * NATPMP_TRYAGAIN
 * NATPMP_ERR_INVALIDARGS
 * NATPMP_ERR_NOPENDINGREQ
 * NATPMP_ERR_NOGATEWAYSUPPORT
 * NATPMP_ERR_RECVFROM
 * NATPMP_ERR_WRONGPACKETSOURCE
 * NATPMP_ERR_UNSUPPORTEDVERSION
 * NATPMP_ERR_UNSUPPORTEDOPCODE
 * NATPMP_ERR_NOTAUTHORIZED
 * NATPMP_ERR_NETWORKFAILURE
 * NATPMP_ERR_OUTOFRESOURCES
 * NATPMP_ERR_UNSUPPORTEDOPCODE
 * NATPMP_ERR_UNDEFINEDERROR */
NATPMP_LIBSPEC int readnatpmpresponseorretry(natpmp_t * p, natpmpresp_t * response);

#ifdef ENABLE_STRNATPMPERR
NATPMP_LIBSPEC const char * strnatpmperr(int t);
#endif

#ifdef __cplusplus
}
#endif

#endif

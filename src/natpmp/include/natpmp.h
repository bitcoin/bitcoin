#ifndef BITCOIN_NATPMP_INCLUDE_NATPMP_H
#define BITCOIN_NATPMP_INCLUDE_NATPMP_H

/* NAT-PMP Port as defined by the NAT-PMP draft */
#define NATPMP_PORT (5351)

#include <time.h>
#if !defined(_MSC_VER)
#include <sys/time.h>
#endif
#ifdef WIN32
#include <winsock2.h>
#if !defined(_MSC_VER)
#include <stdint.h>
#endif
#define in_addr_t uint32_t
#else
#define LIBSPEC
#include <netinet/in.h>
#endif
#include <cstdint>

struct natpmp {
    int s;             /* socket */
    in_addr_t gateway; /* default gateway (IPv4) */
    int has_pending_request;
    unsigned char pending_request[12];
    int pending_request_len;
    int try_number;
    struct timeval retry_time;
};
using natpmp_t = struct natpmp;

struct natpmpresp {
    int16_t type;       /* NATPMP_RESPTYPE_* */
    int16_t resultcode; /* NAT-PMP response code */
    int64_t epoch;      /* Seconds since start of epoch */
    union {
        struct {
            struct in_addr addr;
        } publicaddress;
        struct {
            int16_t privateport;
            int16_t mappedpublicport;
            int64_t lifetime;
        } newportmapping;
    } pnu;
};
using natpmpresp_t = struct natpmpresp;

/* possible values for type field of natpmpresp_t */
#define NATPMP_RESPTYPE_PUBLICADDRESS (0)
#define NATPMP_RESPTYPE_UDPPORTMAPPING (1)
#define NATPMP_RESPTYPE_TCPPORTMAPPING (2)

/* Values to pass to SendNewPortMappingRequest() */
#define NATPMP_PROTOCOL_UDP (1)
#define NATPMP_PROTOCOL_TCP (2)

/* return values */
/* NATPMP_ERR_INVALIDARGS : invalid arguments passed to the function */
#define NATPMP_ERR_INVALIDARGS (-1)
/* NATPMP_ERR_SOCKETERROR : socket() failed. Check errno for details */
#define NATPMP_ERR_SOCKETERROR (-2)
/* NATPMP_ERR_CANNOTGETGATEWAY : can't get default gateway IP */
#define NATPMP_ERR_CANNOTGETGATEWAY (-3)
/* NATPMP_ERR_CLOSEERR : close() failed. Check errno for details */
#define NATPMP_ERR_CLOSEERR (-4)
/* NATPMP_ERR_RECVFROM : recvfrom() failed. Check errno for details */
#define NATPMP_ERR_RECVFROM (-5)
/* NATPMP_ERR_NOPENDINGREQ : ReadNatPmpResponseOrRetry() called while
 * no NAT-PMP request was pending */
#define NATPMP_ERR_NOPENDINGREQ (-6)
/* NATPMP_ERR_NOGATEWAYSUPPORT : the gateway does not support NAT-PMP */
#define NATPMP_ERR_NOGATEWAYSUPPORT (-7)
/* NATPMP_ERR_CONNECTERR : connect() failed. Check errno for details */
#define NATPMP_ERR_CONNECTERR (-8)
/* NATPMP_ERR_WRONGPACKETSOURCE : packet not received from the network gateway */
#define NATPMP_ERR_WRONGPACKETSOURCE (-9)
/* NATPMP_ERR_SENDERR : send() failed. Check errno for details */
#define NATPMP_ERR_SENDERR (-10)
/* NATPMP_ERR_FCNTLERROR : fcntl() failed. Check errno for details */
#define NATPMP_ERR_FCNTLERROR (-11)
/* NATPMP_ERR_GETTIMEOFDAYERR : gettimeofday() failed. Check errno for details */
#define NATPMP_ERR_GETTIMEOFDAYERR (-12)

#define NATPMP_ERR_UNSUPPORTEDVERSION (-14)
#define NATPMP_ERR_UNSUPPORTEDOPCODE (-15)

/* Errors from the server : */
#define NATPMP_ERR_UNDEFINEDERROR (-49)
#define NATPMP_ERR_NOTAUTHORIZED (-51)
#define NATPMP_ERR_NETWORKFAILURE (-52)
#define NATPMP_ERR_OUTOFRESOURCES (-53)

/* NATPMP_TRYAGAIN : no data available for the moment. Try again later */
#define NATPMP_TRYAGAIN (-100)
/* Failure in select() */
#define NATMAP_ERR_SELECT (-101)

#ifdef __cplusplus
extern "C" {
#endif

/*
 * InitNatPmp()
 * Initialize a natpmp_t object
 * With forcegw=1 the gateway is not detected automatically.
 * Return values :
 * 0 = OK
 * NATPMP_ERR_INVALIDARGS
 * NATPMP_ERR_SOCKETERROR
 * NATPMP_ERR_FCNTLERROR
 * NATPMP_ERR_CANNOTGETGATEWAY
 * NATPMP_ERR_CONNECTERR
 *
 */
int InitNatPmp(natpmp_t* p, int forcegw, in_addr_t forcedgw);

/*
 * CloseNatPmp()
 * Close resources associated with a natpmp_t object
 * Return values :
 * 0 = OK
 * NATPMP_ERR_INVALIDARGS
 * NATPMP_ERR_CLOSEERR
 *
 */
int CloseNatPmp(natpmp_t* p);

/*
 * SendPublicAddressRequest()
 * Send a public address NAT-PMP request to the network gateway
 * Return values :
 * 2 = OK (size of the request)
 * NATPMP_ERR_INVALIDARGS
 * NATPMP_ERR_SENDERR
 *
 */
int SendPublicAddressRequest(natpmp_t* p);

/*
 * SendNewPortMappingRequest()
 * Send a new port mapping NAT-PMP request to the network gateway
 * Arguments :
 * protocol is either NATPMP_PROTOCOL_TCP or NATPMP_PROTOCOL_UDP,
 * lifetime is in seconds.
 * To remove a port mapping, set lifetime to zero.
 * To remove all port mappings to the host, set lifetime and both ports
 * to zero.
 * Return values :
 * 12 = OK (size of the request)
 * NATPMP_ERR_INVALIDARGS
 * NATPMP_ERR_SENDERR
 *
 */
int SendNewPortMappingRequest(natpmp_t* p, int protocol, int16_t privateport, int16_t publicport, int64_t lifetime);

/*
 * GetNatPmpRequestTimeout()
 * Fills the timeval structure with the timeout duration of the
 * currently pending NAT-PMP request.
 * Return values :
 * 0 = OK
 * NATPMP_ERR_INVALIDARGS
 * NATPMP_ERR_GETTIMEOFDAYERR
 * NATPMP_ERR_NOPENDINGREQ
 *
 */
int GetNatPmpRequestTimeout(natpmp_t* p, struct timeval* timeout);

/*
 * ReadNatPmpResponseOrRetry()
 * Fills the natpmpresp_t structure if possible
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
 * NATPMP_ERR_UNDEFINEDERROR
 *
 */
int ReadNatPmpResponseOrRetry(natpmp_t* p, natpmpresp_t* response);

const char* StrNatPmpErr(int err);

#ifdef __cplusplus
}
#endif
#endif // BITCOIN_NATPMP_INCLUDE_NATPMP_H

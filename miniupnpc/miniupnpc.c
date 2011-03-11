/* $Id: miniupnpc.c,v 1.85 2010/12/21 16:13:14 nanard Exp $ */
/* Project : miniupnp
 * Author : Thomas BERNARD
 * copyright (c) 2005-2010 Thomas Bernard
 * This software is subjet to the conditions detailed in the
 * provided LICENSE file. */
#define __EXTENSIONS__ 1
#if !defined(MACOSX) && !defined(__sun)
#if !defined(_XOPEN_SOURCE) && !defined(__OpenBSD__) && !defined(__NetBSD__)
#ifndef __cplusplus
#define _XOPEN_SOURCE 600
#endif
#endif
#ifndef __BSD_VISIBLE
#define __BSD_VISIBLE 1
#endif
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef WIN32
/* Win32 Specific includes and defines */
#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>
#include <iphlpapi.h>
#define snprintf _snprintf
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
#define strncasecmp _memicmp
#else /* defined(_MSC_VER) && (_MSC_VER >= 1400) */
#define strncasecmp memicmp
#endif /* defined(_MSC_VER) && (_MSC_VER >= 1400) */
#define MAXHOSTNAMELEN 64
#else /* #ifdef WIN32 */
/* Standard POSIX includes */
#include <unistd.h>
#if defined(__amigaos__) && !defined(__amigaos4__)
/* Amiga OS 3 specific stuff */
#define socklen_t int
#else
#include <sys/select.h>
#endif
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#if !defined(__amigaos__) && !defined(__amigaos4__)
#include <poll.h>
#endif
#include <strings.h>
#include <errno.h>
#define closesocket close
#define MINIUPNPC_IGNORE_EINTR
#endif /* #else WIN32 */
#ifdef MINIUPNPC_SET_SOCKET_TIMEOUT
#include <sys/time.h>
#endif
#if defined(__amigaos__) || defined(__amigaos4__)
/* Amiga OS specific stuff */
#define TIMEVAL struct timeval
#endif

#include "miniupnpc.h"
#include "minissdpc.h"
#include "miniwget.h"
#include "minisoap.h"
#include "minixml.h"
#include "upnpcommands.h"
#include "connecthostport.h"

#ifdef WIN32
#define PRINT_SOCKET_ERROR(x)    printf("Socket error: %s, %d\n", x, WSAGetLastError());
#else
#define PRINT_SOCKET_ERROR(x) perror(x)
#endif

#define SOAPPREFIX "s"
#define SERVICEPREFIX "u"
#define SERVICEPREFIX2 'u'

/* root description parsing */
LIBSPEC void parserootdesc(const char * buffer, int bufsize, struct IGDdatas * data)
{
	struct xmlparser parser;
	/* xmlparser object */
	parser.xmlstart = buffer;
	parser.xmlsize = bufsize;
	parser.data = data;
	parser.starteltfunc = IGDstartelt;
	parser.endeltfunc = IGDendelt;
	parser.datafunc = IGDdata;
	parser.attfunc = 0;
	parsexml(&parser);
#ifdef DEBUG
	printIGD(data);
#endif
}

#if 0
/* getcontentlenfromline() : parse the Content-Length HTTP header line.
 * Content-length: nnn */
static int getcontentlenfromline(const char * p, int n)
{
	static const char contlenstr[] = "content-length";
	const char * p2 = contlenstr;
	int a = 0;
	while(*p2)
	{
		if(n==0)
			return -1;
		if(*p2 != *p && *p2 != (*p + 32))
			return -1;
		p++; p2++; n--;
	}
	if(n==0)
		return -1;
	if(*p != ':')
		return -1;
	p++; n--;
	while(*p == ' ')
	{
		if(n==0)
			return -1;
		p++; n--;
	}
	while(*p >= '0' && *p <= '9')
	{
		if(n==0)
			return -1;
		a = (a * 10) + (*p - '0');
		p++; n--;
	}
	return a;
}

/* getContentLengthAndHeaderLength()
 * retrieve header length and content length from an HTTP response
 * TODO : retrieve Transfer-Encoding: header value, in order to support
 *        HTTP/1.1, chunked transfer encoding must be supported. */
static void
getContentLengthAndHeaderLength(char * p, int n,
                                int * contentlen, int * headerlen)
{
	char * line;
	int linelen;
	int r;
	line = p;
	while(line < p + n)
	{
		linelen = 0;
		while(line[linelen] != '\r' && line[linelen] != '\r')
		{
			if(line+linelen >= p+n)
				return;
			linelen++;
		}
		r = getcontentlenfromline(line, linelen);
		if(r>0)
			*contentlen = r;
		line = line + linelen + 2;
		if(line[0] == '\r' && line[1] == '\n')
		{
			*headerlen = (line - p) + 2;
			return;
		}
	}
}
#endif

/* simpleUPnPcommand2 :
 * not so simple !
 * return values :
 *   0 - OK
 *  -1 - error */
static int simpleUPnPcommand2(int s, const char * url, const char * service,
		       const char * action, struct UPNParg * args,
		       char * buffer, int * bufsize, const char * httpversion)
{
	char hostname[MAXHOSTNAMELEN+1];
	unsigned short port = 0;
	char * path;
	char soapact[128];
	char soapbody[2048];
	char * buf;
	/*int buffree;*/
    int n;
	/*int contentlen, headerlen;*/	/* for the response */

	snprintf(soapact, sizeof(soapact), "%s#%s", service, action);
	if(args==NULL)
	{
		/*soapbodylen = */snprintf(soapbody, sizeof(soapbody),
						"<?xml version=\"1.0\"?>\r\n"
	    	              "<" SOAPPREFIX ":Envelope "
						  "xmlns:" SOAPPREFIX "=\"http://schemas.xmlsoap.org/soap/envelope/\" "
						  SOAPPREFIX ":encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
						  "<" SOAPPREFIX ":Body>"
						  "<" SERVICEPREFIX ":%s xmlns:" SERVICEPREFIX "=\"%s\">"
						  "</" SERVICEPREFIX ":%s>"
						  "</" SOAPPREFIX ":Body></" SOAPPREFIX ":Envelope>"
					 	  "\r\n", action, service, action);
	}
	else
	{
		char * p;
		const char * pe, * pv;
		int soapbodylen;
		soapbodylen = snprintf(soapbody, sizeof(soapbody),
						"<?xml version=\"1.0\"?>\r\n"
	    	            "<" SOAPPREFIX ":Envelope "
						"xmlns:" SOAPPREFIX "=\"http://schemas.xmlsoap.org/soap/envelope/\" "
						SOAPPREFIX ":encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
						"<" SOAPPREFIX ":Body>"
						"<" SERVICEPREFIX ":%s xmlns:" SERVICEPREFIX "=\"%s\">",
						action, service);
		p = soapbody + soapbodylen;
		while(args->elt)
		{
			/* check that we are never overflowing the string... */
			if(soapbody + sizeof(soapbody) <= p + 100)
			{
				/* we keep a margin of at least 100 bytes */
				*bufsize = 0;
				return -1;
			}
			*(p++) = '<';
			pe = args->elt;
			while(*pe)
				*(p++) = *(pe++);
			*(p++) = '>';
			if((pv = args->val))
			{
				while(*pv)
					*(p++) = *(pv++);
			}
			*(p++) = '<';
			*(p++) = '/';
			pe = args->elt;
			while(*pe)
				*(p++) = *(pe++);
			*(p++) = '>';
			args++;
		}
		*(p++) = '<';
		*(p++) = '/';
		*(p++) = SERVICEPREFIX2;
		*(p++) = ':';
		pe = action;
		while(*pe)
			*(p++) = *(pe++);
		strncpy(p, "></" SOAPPREFIX ":Body></" SOAPPREFIX ":Envelope>\r\n",
		        soapbody + sizeof(soapbody) - p);
	}
	if(!parseURL(url, hostname, &port, &path)) return -1;
	if(s<0)
	{
		s = connecthostport(hostname, port);
		if(s < 0)
		{
			*bufsize = 0;
			return -1;
		}
	}

	n = soapPostSubmit(s, path, hostname, port, soapact, soapbody, httpversion);
	if(n<=0) {
#ifdef DEBUG
		printf("Error sending SOAP request\n");
#endif
		closesocket(s);
		return -1;
	}

#if 0
	contentlen = -1;
	headerlen = -1;
	buf = buffer;
	buffree = *bufsize;
	*bufsize = 0;
	while ((n = ReceiveData(s, buf, buffree, 5000)) > 0) {
		buffree -= n;
		buf += n;
		*bufsize += n;
		getContentLengthAndHeaderLength(buffer, *bufsize,
		                                &contentlen, &headerlen);
#ifdef DEBUG
		printf("received n=%dbytes bufsize=%d ContLen=%d HeadLen=%d\n",
		       n, *bufsize, contentlen, headerlen);
#endif
		/* break if we received everything */
		if(contentlen > 0 && headerlen > 0 && *bufsize >= contentlen+headerlen)
			break;
	}
#endif
	buf = getHTTPResponse(s, &n);
	if(n > 0 && buf)
	{
#ifdef DEBUG
		printf("SOAP Response :\n%.*s\n", n, buf);
#endif
		if(*bufsize > n)
		{
			memcpy(buffer, buf, n);
			*bufsize = n;
		}
		else
		{
			memcpy(buffer, buf, *bufsize);
		}
		free(buf);
		buf = 0;
	}
	closesocket(s);
	return 0;
}

/* simpleUPnPcommand :
 * not so simple !
 * return values :
 *   0 - OK
 *  -1 - error */
int simpleUPnPcommand(int s, const char * url, const char * service,
		       const char * action, struct UPNParg * args,
		       char * buffer, int * bufsize)
{
	int result;
	/*int origbufsize = *bufsize;*/

	result = simpleUPnPcommand2(s, url, service, action, args, buffer, bufsize, "1.1");
/*
	result = simpleUPnPcommand2(s, url, service, action, args, buffer, bufsize, "1.0");
	if (result < 0 || *bufsize == 0)
	{
#if DEBUG
	    printf("Error or no result from SOAP request; retrying with HTTP/1.1\n");
#endif
		*bufsize = origbufsize;
		result = simpleUPnPcommand2(s, url, service, action, args, buffer, bufsize, "1.1");
	}
*/
	return result;
}

/* parseMSEARCHReply()
 * the last 4 arguments are filled during the parsing :
 *    - location/locationsize : "location:" field of the SSDP reply packet
 *    - st/stsize : "st:" field of the SSDP reply packet.
 * The strings are NOT null terminated */
static void
parseMSEARCHReply(const char * reply, int size,
                  const char * * location, int * locationsize,
			      const char * * st, int * stsize)
{
	int a, b, i;
	i = 0;
	a = i;	/* start of the line */
	b = 0;
	while(i<size)
	{
		switch(reply[i])
		{
		case ':':
				if(b==0)
				{
					b = i; /* end of the "header" */
					/*for(j=a; j<b; j++)
					{
						putchar(reply[j]);
					}
					*/
				}
				break;
		case '\x0a':
		case '\x0d':
				if(b!=0)
				{
					/*for(j=b+1; j<i; j++)
					{
						putchar(reply[j]);
					}
					putchar('\n');*/
					do { b++; } while(reply[b]==' ');
					if(0==strncasecmp(reply+a, "location", 8))
					{
						*location = reply+b;
						*locationsize = i-b;
					}
					else if(0==strncasecmp(reply+a, "st", 2))
					{
						*st = reply+b;
						*stsize = i-b;
					}
					b = 0;
				}
				a = i+1;
				break;
		default:
				break;
		}
		i++;
	}
}

/* port upnp discover : SSDP protocol */
#define PORT 1900
#define XSTR(s) STR(s)
#define STR(s) #s
#define UPNP_MCAST_ADDR "239.255.255.250"

/* upnpDiscover() :
 * return a chained list of all devices found or NULL if
 * no devices was found.
 * It is up to the caller to free the chained list
 * delay is in millisecond (poll) */
LIBSPEC struct UPNPDev * upnpDiscover(int delay, const char * multicastif,
                              const char * minissdpdsock, int sameport)
{
	struct UPNPDev * tmp;
	struct UPNPDev * devlist = 0;
	int opt = 1;
	static const char MSearchMsgFmt[] = 
	"M-SEARCH * HTTP/1.1\r\n"
	"HOST: " UPNP_MCAST_ADDR ":" XSTR(PORT) "\r\n"
	"ST: %s\r\n"
	"MAN: \"ssdp:discover\"\r\n"
	"MX: %u\r\n"
	"\r\n";
	static const char * const deviceList[] = {
		"urn:schemas-upnp-org:device:InternetGatewayDevice:1",
		"urn:schemas-upnp-org:service:WANIPConnection:1",
		"urn:schemas-upnp-org:service:WANPPPConnection:1",
		"upnp:rootdevice",
		0
	};
	int deviceIndex = 0;
	char bufr[1536];	/* reception and emission buffer */
	int sudp;
	int n;
	struct sockaddr sockudp_r;
	unsigned int mx;
#ifdef NO_GETADDRINFO
	struct sockaddr_in sockudp_w;
#else
	int rv;
	struct addrinfo hints, *servinfo, *p;
#endif
#ifdef WIN32
	MIB_IPFORWARDROW ip_forward;
#endif

#if !defined(WIN32) && !defined(__amigaos__) && !defined(__amigaos4__)
	/* first try to get infos from minissdpd ! */
	if(!minissdpdsock)
		minissdpdsock = "/var/run/minissdpd.sock";
	while(!devlist && deviceList[deviceIndex]) {
		devlist = getDevicesFromMiniSSDPD(deviceList[deviceIndex],
		                                  minissdpdsock);
		/* We return what we have found if it was not only a rootdevice */
		if(devlist && !strstr(deviceList[deviceIndex], "rootdevice"))
			return devlist;
		deviceIndex++;
	}
	deviceIndex = 0;
#endif
	/* fallback to direct discovery */
#ifdef WIN32
	sudp = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
#else
	sudp = socket(PF_INET, SOCK_DGRAM, 0);
#endif
	if(sudp < 0)
	{
		PRINT_SOCKET_ERROR("socket");
		return NULL;
	}
	/* reception */
	memset(&sockudp_r, 0, sizeof(struct sockaddr));
	if(0/*ipv6*/) {
		struct sockaddr_in6 * p = (struct sockaddr_in6 *)&sockudp_r;
		p->sin6_family = AF_INET6;
		if(sameport)
			p->sin6_port = htons(PORT);
		p->sin6_addr = in6addr_any;//IN6ADDR_ANY_INIT;/*INADDR_ANY;*/
	} else {
		struct sockaddr_in * p = (struct sockaddr_in *)&sockudp_r;
		p->sin_family = AF_INET;
		if(sameport)
			p->sin_port = htons(PORT);
		p->sin_addr.s_addr = INADDR_ANY;
	}
#ifdef WIN32
/* This code could help us to use the right Network interface for 
 * SSDP multicast traffic */
/* Get IP associated with the index given in the ip_forward struct
 * in order to give this ip to setsockopt(sudp, IPPROTO_IP, IP_MULTICAST_IF) */
	if(GetBestRoute(inet_addr("223.255.255.255"), 0, &ip_forward) == NO_ERROR) {
		DWORD dwRetVal = 0;
		PMIB_IPADDRTABLE pIPAddrTable;
		DWORD dwSize = 0;
#ifdef DEBUG
		IN_ADDR IPAddr;
#endif
		int i;
#ifdef DEBUG
		printf("ifIndex=%lu nextHop=%lx \n", ip_forward.dwForwardIfIndex, ip_forward.dwForwardNextHop);
#endif
		pIPAddrTable = (MIB_IPADDRTABLE *) malloc(sizeof (MIB_IPADDRTABLE));
		if (GetIpAddrTable(pIPAddrTable, &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER) {
			free(pIPAddrTable);
			pIPAddrTable = (MIB_IPADDRTABLE *) malloc(dwSize);
		}
		if(pIPAddrTable) {
			dwRetVal = GetIpAddrTable( pIPAddrTable, &dwSize, 0 );
#ifdef DEBUG
			printf("\tNum Entries: %ld\n", pIPAddrTable->dwNumEntries);
#endif
			for (i=0; i < (int) pIPAddrTable->dwNumEntries; i++) {
#ifdef DEBUG
				printf("\n\tInterface Index[%d]:\t%ld\n", i, pIPAddrTable->table[i].dwIndex);
				IPAddr.S_un.S_addr = (u_long) pIPAddrTable->table[i].dwAddr;
				printf("\tIP Address[%d]:     \t%s\n", i, inet_ntoa(IPAddr) );
				IPAddr.S_un.S_addr = (u_long) pIPAddrTable->table[i].dwMask;
				printf("\tSubnet Mask[%d]:    \t%s\n", i, inet_ntoa(IPAddr) );
				IPAddr.S_un.S_addr = (u_long) pIPAddrTable->table[i].dwBCastAddr;
				printf("\tBroadCast[%d]:      \t%s (%ld)\n", i, inet_ntoa(IPAddr), pIPAddrTable->table[i].dwBCastAddr);
				printf("\tReassembly size[%d]:\t%ld\n", i, pIPAddrTable->table[i].dwReasmSize);
				printf("\tType and State[%d]:", i);
				printf("\n");
#endif
				if (pIPAddrTable->table[i].dwIndex == ip_forward.dwForwardIfIndex) {
					/* Set the address of this interface to be used */
					struct in_addr mc_if;
					memset(&mc_if, 0, sizeof(mc_if));
					mc_if.s_addr = pIPAddrTable->table[i].dwAddr;
					if(setsockopt(sudp, IPPROTO_IP, IP_MULTICAST_IF, (const char *)&mc_if, sizeof(mc_if)) < 0) {
						PRINT_SOCKET_ERROR("setsockopt");
					}
					((struct sockaddr_in *)&sockudp_r)->sin_addr.s_addr = pIPAddrTable->table[i].dwAddr;
#ifndef DEBUG
					break;
#endif
				}
			}
			free(pIPAddrTable);
			pIPAddrTable = NULL;
		}
	}
#endif

#ifdef WIN32
	if (setsockopt(sudp, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof (opt)) < 0)
#else
	if (setsockopt(sudp, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt)) < 0)
#endif
	{
		PRINT_SOCKET_ERROR("setsockopt");
		return NULL;
	}

	if(multicastif)
	{
		struct in_addr mc_if;
		mc_if.s_addr = inet_addr(multicastif);
		if(0/*ipv6*/) {
		} else {
			((struct sockaddr_in *)&sockudp_r)->sin_addr.s_addr = mc_if.s_addr;
		}
		if(setsockopt(sudp, IPPROTO_IP, IP_MULTICAST_IF, (const char *)&mc_if, sizeof(mc_if)) < 0)
		{
			PRINT_SOCKET_ERROR("setsockopt");
		}
	}

	/* Avant d'envoyer le paquet on bind pour recevoir la reponse */
    if (bind(sudp, &sockudp_r, 0/*ipv6*/?sizeof(struct sockaddr_in6):sizeof(struct sockaddr_in)) != 0)
	{
        PRINT_SOCKET_ERROR("bind");
		closesocket(sudp);
		return NULL;
    }

	/* Calculating maximum response time in seconds */
	mx = ((unsigned int)delay) / 1000u;
	/* receiving SSDP response packet */
	for(n = 0;;)
	{
	if(n == 0)
	{
		/* sending the SSDP M-SEARCH packet */
		n = snprintf(bufr, sizeof(bufr),
		             MSearchMsgFmt, deviceList[deviceIndex++], mx);
		/*printf("Sending %s", bufr);*/
#ifdef NO_GETADDRINFO
		/* the following code is not using getaddrinfo */
		/* emission */
		memset(&sockudp_w, 0, sizeof(struct sockaddr_in));
		sockudp_w.sin_family = AF_INET;
		sockudp_w.sin_port = htons(PORT);
		sockudp_w.sin_addr.s_addr = inet_addr(UPNP_MCAST_ADDR);
		n = sendto(sudp, bufr, n, 0,
		           (struct sockaddr *)&sockudp_w, sizeof(struct sockaddr_in));
		if (n < 0) {
			PRINT_SOCKET_ERROR("sendto");
			closesocket(sudp);
			return devlist;
		}
#else /* #ifdef NO_GETADDRINFO */
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC; // AF_INET6 or AF_INET
		hints.ai_socktype = SOCK_DGRAM;
		/*hints.ai_flags = */
		if ((rv = getaddrinfo(UPNP_MCAST_ADDR, XSTR(PORT), &hints, &servinfo)) != 0) {
#ifdef WIN32
		    fprintf(stderr, "getaddrinfo() failed: %d\n", rv);
#else
		    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
#endif
		    return devlist;
		}
		for(p = servinfo; p; p = p->ai_next) {
			n = sendto(sudp, bufr, n, 0, p->ai_addr, p->ai_addrlen);
			if (n < 0) {
				PRINT_SOCKET_ERROR("sendto");
				continue;
			}
		}
		freeaddrinfo(servinfo);
		if(n < 0) {
			closesocket(sudp);
			return devlist;
		}
#endif /* #ifdef NO_GETADDRINFO */
	}
	/* Waiting for SSDP REPLY packet to M-SEARCH */
	n = ReceiveData(sudp, bufr, sizeof(bufr), delay);
	if (n < 0) {
		/* error */
		closesocket(sudp);
		return devlist;
	} else if (n == 0) {
		/* no data or Time Out */
		if (devlist || (deviceList[deviceIndex] == 0)) {
			/* no more device type to look for... */
			closesocket(sudp);
			return devlist;
		}
	} else {
		const char * descURL=NULL;
		int urlsize=0;
		const char * st=NULL;
		int stsize=0;
        /*printf("%d byte(s) :\n%s\n", n, bufr);*/ /* affichage du message */
		parseMSEARCHReply(bufr, n, &descURL, &urlsize, &st, &stsize);
		if(st&&descURL)
		{
#ifdef DEBUG
			printf("M-SEARCH Reply:\nST: %.*s\nLocation: %.*s\n",
			       stsize, st, urlsize, descURL);
#endif
			for(tmp=devlist; tmp; tmp = tmp->pNext) {
				if(memcmp(tmp->descURL, descURL, urlsize) == 0 &&
				   tmp->descURL[urlsize] == '\0' &&
				   memcmp(tmp->st, st, stsize) == 0 &&
				   tmp->st[stsize] == '\0')
					break;
			}
			/* at the exit of the loop above, tmp is null if
			 * no duplicate device was found */
			if(tmp)
				continue;
			tmp = (struct UPNPDev *)malloc(sizeof(struct UPNPDev)+urlsize+stsize);
			tmp->pNext = devlist;
			tmp->descURL = tmp->buffer;
			tmp->st = tmp->buffer + 1 + urlsize;
			memcpy(tmp->buffer, descURL, urlsize);
			tmp->buffer[urlsize] = '\0';
			memcpy(tmp->buffer + urlsize + 1, st, stsize);
			tmp->buffer[urlsize+1+stsize] = '\0';
			devlist = tmp;
		}
	}
	}
}

/* freeUPNPDevlist() should be used to
 * free the chained list returned by upnpDiscover() */
LIBSPEC void freeUPNPDevlist(struct UPNPDev * devlist)
{
	struct UPNPDev * next;
	while(devlist)
	{
		next = devlist->pNext;
		free(devlist);
		devlist = next;
	}
}

static void
url_cpy_or_cat(char * dst, const char * src, int n)
{
	if(  (src[0] == 'h')
	   &&(src[1] == 't')
	   &&(src[2] == 't')
	   &&(src[3] == 'p')
	   &&(src[4] == ':')
	   &&(src[5] == '/')
	   &&(src[6] == '/'))
	{
		strncpy(dst, src, n);
	}
	else
	{
		int l = strlen(dst);
		if(src[0] != '/')
			dst[l++] = '/';
		if(l<=n)
			strncpy(dst + l, src, n - l);
	}
}

/* Prepare the Urls for usage...
 */
LIBSPEC void GetUPNPUrls(struct UPNPUrls * urls, struct IGDdatas * data,
                 const char * descURL)
{
	char * p;
	int n1, n2, n3;
	n1 = strlen(data->urlbase);
	if(n1==0)
		n1 = strlen(descURL);
	n1 += 2;	/* 1 byte more for Null terminator, 1 byte for '/' if needed */
	n2 = n1; n3 = n1;
	n1 += strlen(data->first.scpdurl);
	n2 += strlen(data->first.controlurl);
	n3 += strlen(data->CIF.controlurl);

	urls->ipcondescURL = (char *)malloc(n1);
	urls->controlURL = (char *)malloc(n2);
	urls->controlURL_CIF = (char *)malloc(n3);
	/* maintenant on chope la desc du WANIPConnection */
	if(data->urlbase[0] != '\0')
		strncpy(urls->ipcondescURL, data->urlbase, n1);
	else
		strncpy(urls->ipcondescURL, descURL, n1);
	p = strchr(urls->ipcondescURL+7, '/');
	if(p) p[0] = '\0';
	strncpy(urls->controlURL, urls->ipcondescURL, n2);
	strncpy(urls->controlURL_CIF, urls->ipcondescURL, n3);
	
	url_cpy_or_cat(urls->ipcondescURL, data->first.scpdurl, n1);

	url_cpy_or_cat(urls->controlURL, data->first.controlurl, n2);

	url_cpy_or_cat(urls->controlURL_CIF, data->CIF.controlurl, n3);

#ifdef DEBUG
	printf("urls->ipcondescURL='%s' %u n1=%d\n", urls->ipcondescURL,
	       (unsigned)strlen(urls->ipcondescURL), n1);
	printf("urls->controlURL='%s' %u n2=%d\n", urls->controlURL,
	       (unsigned)strlen(urls->controlURL), n2);
	printf("urls->controlURL_CIF='%s' %u n3=%d\n", urls->controlURL_CIF,
	       (unsigned)strlen(urls->controlURL_CIF), n3);
#endif
}

LIBSPEC void
FreeUPNPUrls(struct UPNPUrls * urls)
{
	if(!urls)
		return;
	free(urls->controlURL);
	urls->controlURL = 0;
	free(urls->ipcondescURL);
	urls->ipcondescURL = 0;
	free(urls->controlURL_CIF);
	urls->controlURL_CIF = 0;
}


int ReceiveData(int socket, char * data, int length, int timeout)
{
    int n;
#if !defined(WIN32) && !defined(__amigaos__) && !defined(__amigaos4__)
    struct pollfd fds[1]; /* for the poll */
#ifdef MINIUPNPC_IGNORE_EINTR
    do {
#endif
        fds[0].fd = socket;
        fds[0].events = POLLIN;
        n = poll(fds, 1, timeout);
#ifdef MINIUPNPC_IGNORE_EINTR
    } while(n < 0 && errno == EINTR);
#endif
    if(n < 0)
    {
        PRINT_SOCKET_ERROR("poll");
        return -1;
    }
    else if(n == 0)
    {
        return 0;
    }
#else
    fd_set socketSet;
    TIMEVAL timeval;
    FD_ZERO(&socketSet);
    FD_SET(socket, &socketSet);
    timeval.tv_sec = timeout / 1000;
    timeval.tv_usec = (timeout % 1000) * 1000;
    n = select(FD_SETSIZE, &socketSet, NULL, NULL, &timeval);
    if(n < 0)
    {
        PRINT_SOCKET_ERROR("select");
        return -1;
    }
    else if(n == 0)
    {
        return 0;
    }    
#endif
	n = recv(socket, data, length, 0);
	if(n<0)
	{
		PRINT_SOCKET_ERROR("recv");
	}
	return n;
}

int
UPNPIGD_IsConnected(struct UPNPUrls * urls, struct IGDdatas * data)
{
	char status[64];
	unsigned int uptime;
	status[0] = '\0';
	UPNP_GetStatusInfo(urls->controlURL, data->first.servicetype,
	                   status, &uptime, NULL);
	if(0 == strcmp("Connected", status))
	{
		return 1;
	}
	else
		return 0;
}


/* UPNP_GetValidIGD() :
 * return values :
 *     0 = NO IGD found
 *     1 = A valid connected IGD has been found
 *     2 = A valid IGD has been found but it reported as
 *         not connected
 *     3 = an UPnP device has been found but was not recognized as an IGD
 *
 * In any non zero return case, the urls and data structures
 * passed as parameters are set. Donc forget to call FreeUPNPUrls(urls) to
 * free allocated memory.
 */
LIBSPEC int
UPNP_GetValidIGD(struct UPNPDev * devlist,
                 struct UPNPUrls * urls,
				 struct IGDdatas * data,
				 char * lanaddr, int lanaddrlen)
{
	char * descXML;
	int descXMLsize = 0;
	struct UPNPDev * dev;
	int ndev = 0;
	int state; /* state 1 : IGD connected. State 2 : IGD. State 3 : anything */
	if(!devlist)
	{
#ifdef DEBUG
		printf("Empty devlist\n");
#endif
		return 0;
	}
	for(state = 1; state <= 3; state++)
	{
		for(dev = devlist; dev; dev = dev->pNext)
		{
			/* we should choose an internet gateway device.
		 	* with st == urn:schemas-upnp-org:device:InternetGatewayDevice:1 */
			descXML = miniwget_getaddr(dev->descURL, &descXMLsize,
			   	                        lanaddr, lanaddrlen);
			if(descXML)
			{
				ndev++;
				memset(data, 0, sizeof(struct IGDdatas));
				memset(urls, 0, sizeof(struct UPNPUrls));
				parserootdesc(descXML, descXMLsize, data);
				free(descXML);
				descXML = NULL;
				if(0==strcmp(data->CIF.servicetype,
				   "urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1")
				   || state >= 3 )
				{
				  GetUPNPUrls(urls, data, dev->descURL);

#ifdef DEBUG
				  printf("UPNPIGD_IsConnected(%s) = %d\n",
				     urls->controlURL,
			         UPNPIGD_IsConnected(urls, data));
#endif
				  if((state >= 2) || UPNPIGD_IsConnected(urls, data))
					return state;
				  FreeUPNPUrls(urls);
				  if(data->second.servicetype[0] != '\0') {
#ifdef DEBUG
				    printf("We tried %s, now we try %s !\n",
				           data->first.servicetype, data->second.servicetype);
#endif
				    /* swaping WANPPPConnection and WANIPConnection ! */
				    memcpy(&data->tmp, &data->first, sizeof(struct IGDdatas_service));
				    memcpy(&data->first, &data->second, sizeof(struct IGDdatas_service));
				    memcpy(&data->second, &data->tmp, sizeof(struct IGDdatas_service));
				    GetUPNPUrls(urls, data, dev->descURL);
#ifdef DEBUG
				    printf("UPNPIGD_IsConnected(%s) = %d\n",
				       urls->controlURL,
			           UPNPIGD_IsConnected(urls, data));
#endif
				    if((state >= 2) || UPNPIGD_IsConnected(urls, data))
					  return state;
				    FreeUPNPUrls(urls);
				  }
				}
				memset(data, 0, sizeof(struct IGDdatas));
			}
#ifdef DEBUG
			else
			{
				printf("error getting XML description %s\n", dev->descURL);
			}
#endif
		}
	}
	return 0;
}

/* UPNP_GetIGDFromUrl()
 * Used when skipping the discovery process.
 * return value :
 *   0 - Not ok
 *   1 - OK */
int
UPNP_GetIGDFromUrl(const char * rootdescurl,
                   struct UPNPUrls * urls,
                   struct IGDdatas * data,
                   char * lanaddr, int lanaddrlen)
{
	char * descXML;
	int descXMLsize = 0;
	descXML = miniwget_getaddr(rootdescurl, &descXMLsize,
	   	                       lanaddr, lanaddrlen);
	if(descXML) {
		memset(data, 0, sizeof(struct IGDdatas));
		memset(urls, 0, sizeof(struct UPNPUrls));
		parserootdesc(descXML, descXMLsize, data);
		free(descXML);
		descXML = NULL;
		GetUPNPUrls(urls, data, rootdescurl);
		return 1;
	} else {
		return 0;
	}
}


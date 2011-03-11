/* $Id: miniwget.c,v 1.41 2010/12/12 23:52:02 nanard Exp $ */
/* Project : miniupnp
 * Author : Thomas Bernard
 * Copyright (c) 2005-2010 Thomas Bernard
 * This software is subject to the conditions detailed in the
 * LICENCE file provided in this distribution. */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "miniupnpc.h"
#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>
#define MAXHOSTNAMELEN 64
#define MIN(x,y) (((x)<(y))?(x):(y))
#define snprintf _snprintf
#define socklen_t int
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
#define strncasecmp _memicmp
#else /* defined(_MSC_VER) && (_MSC_VER >= 1400) */
#define strncasecmp memicmp
#endif /* defined(_MSC_VER) && (_MSC_VER >= 1400) */
#else /* #ifdef WIN32 */
#include <unistd.h>
#include <sys/param.h>
#if defined(__amigaos__) && !defined(__amigaos4__)
#define socklen_t int
#else /* #if defined(__amigaos__) && !defined(__amigaos4__) */
#include <sys/select.h>
#endif /* #else defined(__amigaos__) && !defined(__amigaos4__) */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#define closesocket close
/* defining MINIUPNPC_IGNORE_EINTR enable the ignore of interruptions
 * during the connect() call */
#define MINIUPNPC_IGNORE_EINTR
#endif /* #else WIN32 */
#if defined(__sun) || defined(sun)
#define MIN(x,y) (((x)<(y))?(x):(y))
#endif

#include "miniupnpcstrings.h"
#include "miniwget.h"
#include "connecthostport.h"

/*
 * Read a HTTP response from a socket.
 * Process Content-Length and Transfer-encoding headers.
 */
void *
getHTTPResponse(int s, int * size)
{
	char buf[2048];
	int n;
	int headers = 1;
	int chunked = 0;
	int content_length = -1;
	unsigned int chunksize = 0;
	unsigned int bytestocopy = 0;
	/* buffers : */
	char * header_buf;
	int header_buf_len = 2048;
	int header_buf_used = 0;
	char * content_buf;
	int content_buf_len = 2048;
	int content_buf_used = 0;

	header_buf = malloc(header_buf_len);
	content_buf = malloc(content_buf_len);

	while((n = ReceiveData(s, buf, 2048, 5000)) > 0)
	{
		if(headers)
		{
			int i;
			int linestart=0;
			int colon=0;
			int valuestart=0;
			if(header_buf_used + n > header_buf_len) {
				header_buf = realloc(header_buf, header_buf_used + n);
				header_buf_len = header_buf_used + n;
			}
			memcpy(header_buf + header_buf_used, buf, n);
			header_buf_used += n;
			for(i = 0; i < (header_buf_used-3); i++) {
				if(colon <= linestart && header_buf[i]==':')
				{
					colon = i;
					while(i < (n-3)
					      && (header_buf[i+1] == ' ' || header_buf[i+1] == '\t'))
						i++;
					valuestart = i + 1;
				}
				/* detecting end of line */
				else if(header_buf[i]=='\r' && header_buf[i+1]=='\n')
				{
					if(colon > linestart && valuestart > colon)
					{
#ifdef DEBUG
						printf("header='%.*s', value='%.*s'\n",
						       colon-linestart, header_buf+linestart,
						       i-valuestart, header_buf+valuestart);
#endif
						if(0==strncasecmp(header_buf+linestart, "content-length", colon-linestart))
						{
							content_length = atoi(header_buf+valuestart);
#ifdef DEBUG
							printf("Content-Length: %d\n", content_length);
#endif
						}
						else if(0==strncasecmp(header_buf+linestart, "transfer-encoding", colon-linestart)
						   && 0==strncasecmp(buf+valuestart, "chunked", 7))
						{
#ifdef DEBUG
							printf("chunked transfer-encoding!\n");
#endif
							chunked = 1;
						}
					}
					linestart = i+2;
					colon = linestart;
					valuestart = 0;
				} 
				/* searching for the end of the HTTP headers */
				if(header_buf[i]=='\r' && header_buf[i+1]=='\n'
				   && header_buf[i+2]=='\r' && header_buf[i+3]=='\n')
				{
					headers = 0;	/* end */
					i += 4;
					if(i < header_buf_used)
					{
						if(chunked)
						{
							while(i<header_buf_used)
							{
								while(i<header_buf_used && isxdigit(header_buf[i]))
								{
									if(header_buf[i] >= '0' && header_buf[i] <= '9')
										chunksize = (chunksize << 4) + (header_buf[i] - '0');
									else
										chunksize = (chunksize << 4) + ((header_buf[i] | 32) - 'a' + 10);
									i++;
								}
								/* discarding chunk-extension */
								while(i < header_buf_used && header_buf[i] != '\r') i++;
								if(i < header_buf_used && header_buf[i] == '\r') i++;
								if(i < header_buf_used && header_buf[i] == '\n') i++;
#ifdef DEBUG
								printf("chunksize = %u (%x)\n", chunksize, chunksize);
#endif
								if(chunksize == 0)
								{
#ifdef DEBUG
									printf("end of HTTP content !\n");
#endif
									goto end_of_stream;	
								}
								bytestocopy = ((int)chunksize < header_buf_used - i)?chunksize:(header_buf_used - i);
#ifdef DEBUG
								printf("chunksize=%u bytestocopy=%u (i=%d header_buf_used=%d)\n",
								       chunksize, bytestocopy, i, header_buf_used);
#endif
								if(content_buf_len < (int)(content_buf_used + bytestocopy))
								{
									content_buf = realloc(content_buf, content_buf_used + bytestocopy);
									content_buf_len = content_buf_used + bytestocopy;
								}
								memcpy(content_buf + content_buf_used, header_buf + i, bytestocopy);
								content_buf_used += bytestocopy;
								chunksize -= bytestocopy;
								i += bytestocopy;
							}
						}
						else
						{
							if(content_buf_len < header_buf_used - i)
							{
								content_buf = realloc(content_buf, header_buf_used - i);
								content_buf_len = header_buf_used - i;
							}
							memcpy(content_buf, header_buf + i, header_buf_used - i);
							content_buf_used = header_buf_used - i;
							i = header_buf_used;
						}
					}
				}
			}
		}
		else
		{
			/* content */
			if(chunked)
			{
				int i = 0;
				while(i < n)
				{
					if(chunksize == 0)
					{
						/* reading chunk size */
						if(i<n && buf[i] == '\r') i++;
						if(i<n && buf[i] == '\n') i++;
						while(i<n && isxdigit(buf[i]))
						{
							if(buf[i] >= '0' && buf[i] <= '9')
								chunksize = (chunksize << 4) + (buf[i] - '0');
							else
								chunksize = (chunksize << 4) + ((buf[i] | 32) - 'a' + 10);
							i++;
						}
						while(i<n && buf[i] != '\r') i++; /* discarding chunk-extension */
						if(i<n && buf[i] == '\r') i++;
						if(i<n && buf[i] == '\n') i++;
#ifdef DEBUG
						printf("chunksize = %u (%x)\n", chunksize, chunksize);
#endif
						if(chunksize == 0)
						{
#ifdef DEBUG
							printf("end of HTTP content - %d %d\n", i, n);
							/*printf("'%.*s'\n", n-i, buf+i);*/
#endif
							goto end_of_stream;
						}
					}
					bytestocopy = ((int)chunksize < n - i)?chunksize:(n - i);
					if((int)(content_buf_used + bytestocopy) > content_buf_len)
					{
						content_buf = (char *)realloc((void *)content_buf, 
						                              content_buf_used + bytestocopy);
						content_buf_len = content_buf_used + bytestocopy;
					}
					memcpy(content_buf + content_buf_used, buf + i, bytestocopy);
					content_buf_used += bytestocopy;
					i += bytestocopy;
					chunksize -= bytestocopy;
				}
			}
			else
			{
				if(content_buf_used + n > content_buf_len)
				{
					content_buf = (char *)realloc((void *)content_buf, 
					                              content_buf_used + n);
					content_buf_len = content_buf_used + n;
				}
				memcpy(content_buf + content_buf_used, buf, n);
				content_buf_used += n;
			}
		}
		if(content_length > 0 && content_buf_used >= content_length)
		{
#ifdef DEBUG
			printf("End of HTTP content\n");
#endif
			break;
		}
	}
end_of_stream:
	free(header_buf); header_buf = NULL;
	*size = content_buf_used;
	if(content_buf_used == 0)
	{
		free(content_buf);
		content_buf = NULL;
	}
	return content_buf;
}

/* miniwget3() :
 * do all the work.
 * Return NULL if something failed. */
static void *
miniwget3(const char * url, const char * host,
		  unsigned short port, const char * path,
		  int * size, char * addr_str, int addr_str_len, const char * httpversion)
{
	char buf[2048];
    int s;
	int n;
	int len;
	int sent;

	*size = 0;
	s = connecthostport(host, port);
	if(s < 0)
		return NULL;

	/* get address for caller ! */
	if(addr_str)
	{
		struct sockaddr saddr;
		socklen_t saddrlen;

		saddrlen = sizeof(saddr);
		if(getsockname(s, &saddr, &saddrlen) < 0)
		{
			perror("getsockname");
		}
		else
		{
#if defined(__amigaos__) && !defined(__amigaos4__)
	/* using INT WINAPI WSAAddressToStringA(LPSOCKADDR, DWORD, LPWSAPROTOCOL_INFOA, LPSTR, LPDWORD);
     * But his function make a string with the port :  nn.nn.nn.nn:port */
/*		if(WSAAddressToStringA((SOCKADDR *)&saddr, sizeof(saddr),
                            NULL, addr_str, (DWORD *)&addr_str_len))
		{
		    printf("WSAAddressToStringA() failed : %d\n", WSAGetLastError());
		}*/
			strncpy(addr_str, inet_ntoa(((struct sockaddr_in *)&saddr)->sin_addr), addr_str_len);
#else
			/*inet_ntop(AF_INET, &saddr.sin_addr, addr_str, addr_str_len);*/
			n = getnameinfo(&saddr, saddrlen,
			                addr_str, addr_str_len,
			                NULL, 0,
			                NI_NUMERICHOST | NI_NUMERICSERV);
			if(n != 0) {
#ifdef WIN32
				fprintf(stderr, "getnameinfo() failed : %d\n", n);
#else
				fprintf(stderr, "getnameinfo() failed : %s\n", gai_strerror(n));
#endif
			}
#endif
		}
#ifdef DEBUG
		printf("address miniwget : %s\n", addr_str);
#endif
	}

	len = snprintf(buf, sizeof(buf),
                 "GET %s HTTP/%s\r\n"
			     "Host: %s:%d\r\n"
				 "Connection: Close\r\n"
				 "User-Agent: " OS_STRING ", UPnP/1.0, MiniUPnPc/" MINIUPNPC_VERSION_STRING "\r\n"

				 "\r\n",
			   path, httpversion, host, port);
	sent = 0;
	/* sending the HTTP request */
	while(sent < len)
	{
		n = send(s, buf+sent, len-sent, 0);
		if(n < 0)
		{
			perror("send");
			closesocket(s);
			return NULL;
		}
		else
		{
			sent += n;
		}
	}
	return getHTTPResponse(s, size);
}

/* miniwget2() :
 * Call miniwget3(); retry with HTTP/1.1 if 1.0 fails. */
static void *
miniwget2(const char * url, const char * host,
		  unsigned short port, const char * path,
		  int * size, char * addr_str, int addr_str_len)
{
	char * respbuffer;

	respbuffer = miniwget3(url, host, port, path, size, addr_str, addr_str_len, "1.1");
/*
	respbuffer = miniwget3(url, host, port, path, size, addr_str, addr_str_len, "1.0");
	if (*size == 0)
	{
#ifdef DEBUG
		printf("Retrying with HTTP/1.1\n");
#endif
		free(respbuffer);
		respbuffer = miniwget3(url, host, port, path, size, addr_str, addr_str_len, "1.1");
	}
*/
	return respbuffer;
}




/* parseURL()
 * arguments :
 *   url :		source string not modified
 *   hostname :	hostname destination string (size of MAXHOSTNAMELEN+1)
 *   port :		port (destination)
 *   path :		pointer to the path part of the URL 
 *
 * Return values :
 *    0 - Failure
 *    1 - Success         */
int parseURL(const char * url, char * hostname, unsigned short * port, char * * path)
{
	char * p1, *p2, *p3;
	p1 = strstr(url, "://");
	if(!p1)
		return 0;
	p1 += 3;
	if(  (url[0]!='h') || (url[1]!='t')
	   ||(url[2]!='t') || (url[3]!='p'))
		return 0;
	p2 = strchr(p1, ':');
	p3 = strchr(p1, '/');
	if(!p3)
		return 0;
	memset(hostname, 0, MAXHOSTNAMELEN + 1);
	if(!p2 || (p2>p3))
	{
		strncpy(hostname, p1, MIN(MAXHOSTNAMELEN, (int)(p3-p1)));
		*port = 80;
	}
	else
	{
		strncpy(hostname, p1, MIN(MAXHOSTNAMELEN, (int)(p2-p1)));
		*port = 0;
		p2++;
		while( (*p2 >= '0') && (*p2 <= '9'))
		{
			*port *= 10;
			*port += (unsigned short)(*p2 - '0');
			p2++;
		}
	}
	*path = p3;
	return 1;
}

void * miniwget(const char * url, int * size)
{
	unsigned short port;
	char * path;
	/* protocol://host:port/chemin */
	char hostname[MAXHOSTNAMELEN+1];
	*size = 0;
	if(!parseURL(url, hostname, &port, &path))
		return NULL;
	return miniwget2(url, hostname, port, path, size, 0, 0);
}

void * miniwget_getaddr(const char * url, int * size, char * addr, int addrlen)
{
	unsigned short port;
	char * path;
	/* protocol://host:port/chemin */
	char hostname[MAXHOSTNAMELEN+1];
	*size = 0;
	if(addr)
		addr[0] = '\0';
	if(!parseURL(url, hostname, &port, &path))
		return NULL;
	return miniwget2(url, hostname, port, path, size, addr, addrlen);
}


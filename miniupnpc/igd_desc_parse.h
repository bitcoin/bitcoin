/* $Id: igd_desc_parse.h,v 1.7 2010/04/05 20:36:59 nanard Exp $ */
/* Project : miniupnp
 * http://miniupnp.free.fr/
 * Author : Thomas Bernard
 * Copyright (c) 2005-2010 Thomas Bernard
 * This software is subject to the conditions detailed in the
 * LICENCE file provided in this distribution.
 * */
#ifndef __IGD_DESC_PARSE_H__
#define __IGD_DESC_PARSE_H__

/* Structure to store the result of the parsing of UPnP
 * descriptions of Internet Gateway Devices */
#define MINIUPNPC_URL_MAXSIZE (128)
struct IGDdatas_service {
	char controlurl[MINIUPNPC_URL_MAXSIZE];
	char eventsuburl[MINIUPNPC_URL_MAXSIZE];
	char scpdurl[MINIUPNPC_URL_MAXSIZE];
	char servicetype[MINIUPNPC_URL_MAXSIZE];
	/*char devicetype[MINIUPNPC_URL_MAXSIZE];*/
};

struct IGDdatas {
	char cureltname[MINIUPNPC_URL_MAXSIZE];
	char urlbase[MINIUPNPC_URL_MAXSIZE];
	int level;
	/*int state;*/
	/* "urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1" */
	struct IGDdatas_service CIF;
	/* "urn:schemas-upnp-org:service:WANIPConnection:1"
	 * "urn:schemas-upnp-org:service:WANPPPConnection:1" */
	struct IGDdatas_service first;
	/* if both WANIPConnection and WANPPPConnection are present */
	struct IGDdatas_service second;
	/* tmp */
	struct IGDdatas_service tmp;
};

void IGDstartelt(void *, const char *, int);
void IGDendelt(void *, const char *, int);
void IGDdata(void *, const char *, int);
void printIGD(struct IGDdatas *);

#endif


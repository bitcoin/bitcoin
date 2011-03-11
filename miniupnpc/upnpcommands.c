/* $Id: upnpcommands.c,v 1.26 2010/06/09 10:59:09 nanard Exp $ */
/* Project : miniupnp
 * Author : Thomas Bernard
 * Copyright (c) 2005-2010 Thomas Bernard
 * This software is subject to the conditions detailed in the
 * LICENCE file provided in this distribution.
 * */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "upnpcommands.h"
#include "miniupnpc.h"

static UNSIGNED_INTEGER
my_atoui(const char * s)
{
	return s ? ((UNSIGNED_INTEGER)STRTOUI(s, NULL, 0)) : 0;
}

/*
 * */
LIBSPEC UNSIGNED_INTEGER
UPNP_GetTotalBytesSent(const char * controlURL,
					const char * servicetype)
{
	struct NameValueParserData pdata;
	char buffer[4096];
	int bufsize = 4096;
	unsigned int r = 0;
	char * p;
	if(simpleUPnPcommand(-1, controlURL, servicetype, "GetTotalBytesSent", 0, buffer, &bufsize) < 0) {
		return UPNPCOMMAND_HTTP_ERROR;
	}
	ParseNameValue(buffer, bufsize, &pdata);
	/*DisplayNameValueList(buffer, bufsize);*/
	p = GetValueFromNameValueList(&pdata, "NewTotalBytesSent");
	r = my_atoui(p);
	ClearNameValueList(&pdata);
	return r;
}

/*
 * */
LIBSPEC UNSIGNED_INTEGER
UPNP_GetTotalBytesReceived(const char * controlURL,
						const char * servicetype)
{
	struct NameValueParserData pdata;
	char buffer[4096];
	int bufsize = 4096;
	unsigned int r = 0;
	char * p;
	if(simpleUPnPcommand(-1, controlURL, servicetype, "GetTotalBytesReceived", 0, buffer, &bufsize) < 0) {
		return UPNPCOMMAND_HTTP_ERROR;
	}
	ParseNameValue(buffer, bufsize, &pdata);
	/*DisplayNameValueList(buffer, bufsize);*/
	p = GetValueFromNameValueList(&pdata, "NewTotalBytesReceived");
	r = my_atoui(p);
	ClearNameValueList(&pdata);
	return r;
}

/*
 * */
LIBSPEC UNSIGNED_INTEGER
UPNP_GetTotalPacketsSent(const char * controlURL,
						const char * servicetype)
{
	struct NameValueParserData pdata;
	char buffer[4096];
	int bufsize = 4096;
	unsigned int r = 0;
	char * p;
	if(simpleUPnPcommand(-1, controlURL, servicetype, "GetTotalPacketsSent", 0, buffer, &bufsize) < 0) {
		return UPNPCOMMAND_HTTP_ERROR;
	}
	ParseNameValue(buffer, bufsize, &pdata);
	/*DisplayNameValueList(buffer, bufsize);*/
	p = GetValueFromNameValueList(&pdata, "NewTotalPacketsSent");
	r = my_atoui(p);
	ClearNameValueList(&pdata);
	return r;
}

/*
 * */
LIBSPEC UNSIGNED_INTEGER
UPNP_GetTotalPacketsReceived(const char * controlURL,
						const char * servicetype)
{
	struct NameValueParserData pdata;
	char buffer[4096];
	int bufsize = 4096;
	unsigned int r = 0;
	char * p;
	if(simpleUPnPcommand(-1, controlURL, servicetype, "GetTotalPacketsReceived", 0, buffer, &bufsize) < 0) {
		return UPNPCOMMAND_HTTP_ERROR;
	}
	ParseNameValue(buffer, bufsize, &pdata);
	/*DisplayNameValueList(buffer, bufsize);*/
	p = GetValueFromNameValueList(&pdata, "NewTotalPacketsReceived");
	r = my_atoui(p);
	ClearNameValueList(&pdata);
	return r;
}

/* UPNP_GetStatusInfo() call the corresponding UPNP method
 * returns the current status and uptime */
LIBSPEC int
UPNP_GetStatusInfo(const char * controlURL,
				const char * servicetype,
				char * status, 
				unsigned int * uptime,
				char * lastconnerror)
{
	struct NameValueParserData pdata;
	char buffer[4096];
	int bufsize = 4096;
	char * p;
	char * up;
	char * err;
	int ret = UPNPCOMMAND_UNKNOWN_ERROR;

	if(!status && !uptime)
		return UPNPCOMMAND_INVALID_ARGS;

	if(simpleUPnPcommand(-1, controlURL, servicetype, "GetStatusInfo", 0, buffer, &bufsize) < 0) {
		return UPNPCOMMAND_HTTP_ERROR;
	}
	ParseNameValue(buffer, bufsize, &pdata);
	/*DisplayNameValueList(buffer, bufsize);*/
	up = GetValueFromNameValueList(&pdata, "NewUptime");
	p = GetValueFromNameValueList(&pdata, "NewConnectionStatus");
	err = GetValueFromNameValueList(&pdata, "NewLastConnectionError");
	if(p && up)
	  ret = UPNPCOMMAND_SUCCESS;

	if(status) {
		if(p){
			strncpy(status, p, 64 );
			status[63] = '\0';
		}else
			status[0]= '\0';
	}

	if(uptime) {
		if(up)
			sscanf(up,"%u",uptime);
		else
			uptime = 0;
	}

	if(lastconnerror) {
		if(err) {
			strncpy(lastconnerror, err, 64 );
			lastconnerror[63] = '\0';
		} else
			lastconnerror[0] = '\0';
	}

	p = GetValueFromNameValueList(&pdata, "errorCode");
	if(p) {
		ret = UPNPCOMMAND_UNKNOWN_ERROR;
		sscanf(p, "%d", &ret);
	}
	ClearNameValueList(&pdata);
	return ret;
}

/* UPNP_GetConnectionTypeInfo() call the corresponding UPNP method
 * returns the connection type */
LIBSPEC int
UPNP_GetConnectionTypeInfo(const char * controlURL,
                           const char * servicetype,
                           char * connectionType)
{
	struct NameValueParserData pdata;
	char buffer[4096];
	int bufsize = 4096;
	char * p;
	int ret = UPNPCOMMAND_UNKNOWN_ERROR;

	if(!connectionType)
		return UPNPCOMMAND_INVALID_ARGS;

	if(simpleUPnPcommand(-1, controlURL, servicetype,
	                  "GetConnectionTypeInfo", 0, buffer, &bufsize) < 0) {
		return UPNPCOMMAND_HTTP_ERROR;
	}
	ParseNameValue(buffer, bufsize, &pdata);
	p = GetValueFromNameValueList(&pdata, "NewConnectionType");
	/*p = GetValueFromNameValueList(&pdata, "NewPossibleConnectionTypes");*/
	/* PossibleConnectionTypes will have several values.... */
	if(p) {
		strncpy(connectionType, p, 64 );
		connectionType[63] = '\0';
		ret = UPNPCOMMAND_SUCCESS;
	} else
		connectionType[0] = '\0';
	p = GetValueFromNameValueList(&pdata, "errorCode");
	if(p) {
		ret = UPNPCOMMAND_UNKNOWN_ERROR;
		sscanf(p, "%d", &ret);
	}
	ClearNameValueList(&pdata);
	return ret;
}

/* UPNP_GetLinkLayerMaxBitRate() call the corresponding UPNP method.
 * Returns 2 values: Downloadlink bandwidth and Uplink bandwidth.
 * One of the values can be null 
 * Note : GetLinkLayerMaxBitRates belongs to WANPPPConnection:1 only 
 * We can use the GetCommonLinkProperties from WANCommonInterfaceConfig:1 */
LIBSPEC int
UPNP_GetLinkLayerMaxBitRates(const char * controlURL,
                             const char * servicetype,
                             unsigned int * bitrateDown,
                             unsigned int* bitrateUp)
{
	struct NameValueParserData pdata;
	char buffer[4096];
	int bufsize = 4096;
	int ret = UPNPCOMMAND_UNKNOWN_ERROR;
	char * down;
	char * up;
	char * p;

	if(!bitrateDown && !bitrateUp)
		return UPNPCOMMAND_INVALID_ARGS;

	/* shouldn't we use GetCommonLinkProperties ? */
	if(simpleUPnPcommand(-1, controlURL, servicetype,
	                  "GetCommonLinkProperties", 0, buffer, &bufsize) < 0) {
	                  /*"GetLinkLayerMaxBitRates", 0, buffer, &bufsize);*/
		return UPNPCOMMAND_HTTP_ERROR;
	}
	/*DisplayNameValueList(buffer, bufsize);*/
	ParseNameValue(buffer, bufsize, &pdata);
	/*down = GetValueFromNameValueList(&pdata, "NewDownstreamMaxBitRate");*/
	/*up = GetValueFromNameValueList(&pdata, "NewUpstreamMaxBitRate");*/
	down = GetValueFromNameValueList(&pdata, "NewLayer1DownstreamMaxBitRate");
	up = GetValueFromNameValueList(&pdata, "NewLayer1UpstreamMaxBitRate");
	/*GetValueFromNameValueList(&pdata, "NewWANAccessType");*/
	/*GetValueFromNameValueList(&pdata, "NewPhysicalLinkSatus");*/
	if(down && up)
		ret = UPNPCOMMAND_SUCCESS;

	if(bitrateDown) {
		if(down)
			sscanf(down,"%u",bitrateDown);
		else
			*bitrateDown = 0;
	}

	if(bitrateUp) {
		if(up)
			sscanf(up,"%u",bitrateUp);
		else
			*bitrateUp = 0;
	}
	p = GetValueFromNameValueList(&pdata, "errorCode");
	if(p) {
		ret = UPNPCOMMAND_UNKNOWN_ERROR;
		sscanf(p, "%d", &ret);
	}
	ClearNameValueList(&pdata);
	return ret;
}


/* UPNP_GetExternalIPAddress() call the corresponding UPNP method.
 * if the third arg is not null the value is copied to it.
 * at least 16 bytes must be available
 * 
 * Return values :
 * 0 : SUCCESS
 * NON ZERO : ERROR Either an UPnP error code or an unknown error.
 *
 * 402 Invalid Args - See UPnP Device Architecture section on Control.
 * 501 Action Failed - See UPnP Device Architecture section on Control.
 */
LIBSPEC int
UPNP_GetExternalIPAddress(const char * controlURL,
                          const char * servicetype,
                          char * extIpAdd)
{
	struct NameValueParserData pdata;
	char buffer[4096];
	int bufsize = 4096;
	char * p;
	int ret = UPNPCOMMAND_UNKNOWN_ERROR;

	if(!extIpAdd || !controlURL || !servicetype)
		return UPNPCOMMAND_INVALID_ARGS;

	if(simpleUPnPcommand(-1, controlURL, servicetype, "GetExternalIPAddress", 0, buffer, &bufsize) < 0) {
		return UPNPCOMMAND_HTTP_ERROR;
	}
	/*DisplayNameValueList(buffer, bufsize);*/
	ParseNameValue(buffer, bufsize, &pdata);
	/*printf("external ip = %s\n", GetValueFromNameValueList(&pdata, "NewExternalIPAddress") );*/
	p = GetValueFromNameValueList(&pdata, "NewExternalIPAddress");
	if(p) {
		strncpy(extIpAdd, p, 16 );
		extIpAdd[15] = '\0';
		ret = UPNPCOMMAND_SUCCESS;
	} else
		extIpAdd[0] = '\0';

	p = GetValueFromNameValueList(&pdata, "errorCode");
	if(p) {
		ret = UPNPCOMMAND_UNKNOWN_ERROR;
		sscanf(p, "%d", &ret);
	}

	ClearNameValueList(&pdata);
	return ret;
}

LIBSPEC int
UPNP_AddPortMapping(const char * controlURL, const char * servicetype,
                    const char * extPort,
					const char * inPort,
					const char * inClient,
					const char * desc,
					const char * proto,
                    const char * remoteHost)
{
	struct UPNParg * AddPortMappingArgs;
	char buffer[4096];
	int bufsize = 4096;
	struct NameValueParserData pdata;
	const char * resVal;
	int ret;

	if(!inPort || !inClient || !proto || !extPort)
		return UPNPCOMMAND_INVALID_ARGS;

	AddPortMappingArgs = calloc(9, sizeof(struct UPNParg));
	AddPortMappingArgs[0].elt = "NewRemoteHost";
	AddPortMappingArgs[0].val = remoteHost;
	AddPortMappingArgs[1].elt = "NewExternalPort";
	AddPortMappingArgs[1].val = extPort;
	AddPortMappingArgs[2].elt = "NewProtocol";
	AddPortMappingArgs[2].val = proto;
	AddPortMappingArgs[3].elt = "NewInternalPort";
	AddPortMappingArgs[3].val = inPort;
	AddPortMappingArgs[4].elt = "NewInternalClient";
	AddPortMappingArgs[4].val = inClient;
	AddPortMappingArgs[5].elt = "NewEnabled";
	AddPortMappingArgs[5].val = "1";
	AddPortMappingArgs[6].elt = "NewPortMappingDescription";
	AddPortMappingArgs[6].val = desc?desc:"libminiupnpc";
	AddPortMappingArgs[7].elt = "NewLeaseDuration";
	AddPortMappingArgs[7].val = "0";
	if(simpleUPnPcommand(-1, controlURL, servicetype, "AddPortMapping", AddPortMappingArgs, buffer, &bufsize) < 0) {
		free(AddPortMappingArgs);
		return UPNPCOMMAND_HTTP_ERROR;
	}
	/*DisplayNameValueList(buffer, bufsize);*/
	/*buffer[bufsize] = '\0';*/
	/*puts(buffer);*/
	ParseNameValue(buffer, bufsize, &pdata);
	resVal = GetValueFromNameValueList(&pdata, "errorCode");
	if(resVal) {
		/*printf("AddPortMapping errorCode = '%s'\n", resVal); */
		ret = UPNPCOMMAND_UNKNOWN_ERROR;
		sscanf(resVal, "%d", &ret);
	} else {
		ret = UPNPCOMMAND_SUCCESS;
	}
	ClearNameValueList(&pdata);
	free(AddPortMappingArgs);
	return ret;
}

LIBSPEC int
UPNP_DeletePortMapping(const char * controlURL, const char * servicetype,
                       const char * extPort, const char * proto,
                       const char * remoteHost)
{
	/*struct NameValueParserData pdata;*/
	struct UPNParg * DeletePortMappingArgs;
	char buffer[4096];
	int bufsize = 4096;
	struct NameValueParserData pdata;
	const char * resVal;
	int ret;

	if(!extPort || !proto)
		return UPNPCOMMAND_INVALID_ARGS;

	DeletePortMappingArgs = calloc(4, sizeof(struct UPNParg));
	DeletePortMappingArgs[0].elt = "NewRemoteHost";
	DeletePortMappingArgs[0].val = remoteHost;
	DeletePortMappingArgs[1].elt = "NewExternalPort";
	DeletePortMappingArgs[1].val = extPort;
	DeletePortMappingArgs[2].elt = "NewProtocol";
	DeletePortMappingArgs[2].val = proto;
	if(simpleUPnPcommand(-1, controlURL, servicetype,
	                     "DeletePortMapping",
	                     DeletePortMappingArgs, buffer, &bufsize) < 0 ) {
		free(DeletePortMappingArgs);
		return UPNPCOMMAND_HTTP_ERROR;
	}
	/*DisplayNameValueList(buffer, bufsize);*/
	ParseNameValue(buffer, bufsize, &pdata);
	resVal = GetValueFromNameValueList(&pdata, "errorCode");
	if(resVal) {
		ret = UPNPCOMMAND_UNKNOWN_ERROR;
		sscanf(resVal, "%d", &ret);
	} else {
		ret = UPNPCOMMAND_SUCCESS;
	}
	ClearNameValueList(&pdata);
	free(DeletePortMappingArgs);
	return ret;
}

LIBSPEC int
UPNP_GetGenericPortMappingEntry(const char * controlURL,
                                const char * servicetype,
							 const char * index,
							 char * extPort,
							 char * intClient,
							 char * intPort,
							 char * protocol,
							 char * desc,
							 char * enabled,
							 char * rHost,
							 char * duration)
{
	struct NameValueParserData pdata;
	struct UPNParg * GetPortMappingArgs;
	char buffer[4096];
	int bufsize = 4096;
	char * p;
	int r = UPNPCOMMAND_UNKNOWN_ERROR;
	if(!index)
		return UPNPCOMMAND_INVALID_ARGS;
	intClient[0] = '\0';
	intPort[0] = '\0';
	GetPortMappingArgs = calloc(2, sizeof(struct UPNParg));
	GetPortMappingArgs[0].elt = "NewPortMappingIndex";
	GetPortMappingArgs[0].val = index;
	if(simpleUPnPcommand(-1, controlURL, servicetype,
	                  "GetGenericPortMappingEntry",
					  GetPortMappingArgs, buffer, &bufsize) < 0) {
		free(GetPortMappingArgs);
		return UPNPCOMMAND_HTTP_ERROR;
	}
	ParseNameValue(buffer, bufsize, &pdata);
	p = GetValueFromNameValueList(&pdata, "NewRemoteHost");
	if(p && rHost)
	{
		strncpy(rHost, p, 64);
		rHost[63] = '\0';
	}
	p = GetValueFromNameValueList(&pdata, "NewExternalPort");
	if(p && extPort)
	{
		strncpy(extPort, p, 6);
		extPort[5] = '\0';
		r = UPNPCOMMAND_SUCCESS;
	}
	p = GetValueFromNameValueList(&pdata, "NewProtocol");
	if(p && protocol)
	{
		strncpy(protocol, p, 4);
		protocol[3] = '\0';
	}
	p = GetValueFromNameValueList(&pdata, "NewInternalClient");
	if(p && intClient)
	{
		strncpy(intClient, p, 16);
		intClient[15] = '\0';
		r = 0;
	}
	p = GetValueFromNameValueList(&pdata, "NewInternalPort");
	if(p && intPort)
	{
		strncpy(intPort, p, 6);
		intPort[5] = '\0';
	}
	p = GetValueFromNameValueList(&pdata, "NewEnabled");
	if(p && enabled)
	{
		strncpy(enabled, p, 4);
		enabled[3] = '\0';
	}
	p = GetValueFromNameValueList(&pdata, "NewPortMappingDescription");
	if(p && desc)
	{
		strncpy(desc, p, 80);
		desc[79] = '\0';
	}
	p = GetValueFromNameValueList(&pdata, "NewLeaseDuration");
	if(p && duration)
	{
		strncpy(duration, p, 16);
		duration[15] = '\0';
	}
	p = GetValueFromNameValueList(&pdata, "errorCode");
	if(p) {
		r = UPNPCOMMAND_UNKNOWN_ERROR;
		sscanf(p, "%d", &r);
	}
	ClearNameValueList(&pdata);
	free(GetPortMappingArgs);
	return r;
}

LIBSPEC int
UPNP_GetPortMappingNumberOfEntries(const char * controlURL,
                                   const char * servicetype,
                                   unsigned int * numEntries)
{
 	struct NameValueParserData pdata;
 	char buffer[4096];
 	int bufsize = 4096;
 	char* p;
	int ret = UPNPCOMMAND_UNKNOWN_ERROR;
 	if(simpleUPnPcommand(-1, controlURL, servicetype, "GetPortMappingNumberOfEntries", 0, buffer, &bufsize) < 0) {
		return UPNPCOMMAND_HTTP_ERROR;
	}
#ifdef DEBUG
	DisplayNameValueList(buffer, bufsize);
#endif
 	ParseNameValue(buffer, bufsize, &pdata);

 	p = GetValueFromNameValueList(&pdata, "NewPortMappingNumberOfEntries");
 	if(numEntries && p) {
		*numEntries = 0;
 		sscanf(p, "%u", numEntries);
		ret = UPNPCOMMAND_SUCCESS;
 	}

	p = GetValueFromNameValueList(&pdata, "errorCode");
	if(p) {
		ret = UPNPCOMMAND_UNKNOWN_ERROR;
		sscanf(p, "%d", &ret);
	}

 	ClearNameValueList(&pdata);
	return ret;
}

/* UPNP_GetSpecificPortMappingEntry retrieves an existing port mapping
 * the result is returned in the intClient and intPort strings
 * please provide 16 and 6 bytes of data */
LIBSPEC int
UPNP_GetSpecificPortMappingEntry(const char * controlURL,
                                 const char * servicetype,
                                 const char * extPort,
							     const char * proto,
                                 char * intClient,
                                 char * intPort)
{
	struct NameValueParserData pdata;
	struct UPNParg * GetPortMappingArgs;
	char buffer[4096];
	int bufsize = 4096;
	char * p;
	int ret = UPNPCOMMAND_UNKNOWN_ERROR;

	if(!intPort || !intClient || !extPort || !proto)
		return UPNPCOMMAND_INVALID_ARGS;

	GetPortMappingArgs = calloc(4, sizeof(struct UPNParg));
	GetPortMappingArgs[0].elt = "NewRemoteHost";
	GetPortMappingArgs[1].elt = "NewExternalPort";
	GetPortMappingArgs[1].val = extPort;
	GetPortMappingArgs[2].elt = "NewProtocol";
	GetPortMappingArgs[2].val = proto;
	if(simpleUPnPcommand(-1, controlURL, servicetype,
	                     "GetSpecificPortMappingEntry",
	                     GetPortMappingArgs, buffer, &bufsize) < 0) {
		free(GetPortMappingArgs);
		return UPNPCOMMAND_HTTP_ERROR;
	}
	/*DisplayNameValueList(buffer, bufsize);*/
	ParseNameValue(buffer, bufsize, &pdata);

	p = GetValueFromNameValueList(&pdata, "NewInternalClient");
	if(p) {
		strncpy(intClient, p, 16);
		intClient[15] = '\0';
		ret = UPNPCOMMAND_SUCCESS;
	} else
		intClient[0] = '\0';

	p = GetValueFromNameValueList(&pdata, "NewInternalPort");
	if(p) {
		strncpy(intPort, p, 6);
		intPort[5] = '\0';
	} else
		intPort[0] = '\0';

	p = GetValueFromNameValueList(&pdata, "errorCode");
	if(p) {
		ret = UPNPCOMMAND_UNKNOWN_ERROR;
		sscanf(p, "%d", &ret);
	}

	ClearNameValueList(&pdata);
	free(GetPortMappingArgs);
	return ret;
}



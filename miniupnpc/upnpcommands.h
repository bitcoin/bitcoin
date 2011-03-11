/* $Id: upnpcommands.h,v 1.18 2010/06/09 10:59:09 nanard Exp $ */
/* Miniupnp project : http://miniupnp.free.fr/
 * Author : Thomas Bernard
 * Copyright (c) 2005-2010 Thomas Bernard
 * This software is subject to the conditions detailed in the
 * LICENCE file provided within this distribution */
#ifndef __UPNPCOMMANDS_H__
#define __UPNPCOMMANDS_H__

#include "upnpreplyparse.h"
#include "declspec.h"

/* MiniUPnPc return codes : */
#define UPNPCOMMAND_SUCCESS (0)
#define UPNPCOMMAND_UNKNOWN_ERROR (-1)
#define UPNPCOMMAND_INVALID_ARGS (-2)
#define UPNPCOMMAND_HTTP_ERROR (-3)

#ifdef __cplusplus
extern "C" {
#endif

#if (defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L)
#define UNSIGNED_INTEGER unsigned long long
#define STRTOUI	strtoull
#else
#define UNSIGNED_INTEGER unsigned int
#define STRTOUI	strtoul
#endif

LIBSPEC UNSIGNED_INTEGER
UPNP_GetTotalBytesSent(const char * controlURL,
					const char * servicetype);

LIBSPEC UNSIGNED_INTEGER
UPNP_GetTotalBytesReceived(const char * controlURL,
						const char * servicetype);

LIBSPEC UNSIGNED_INTEGER
UPNP_GetTotalPacketsSent(const char * controlURL,
					const char * servicetype);

LIBSPEC UNSIGNED_INTEGER
UPNP_GetTotalPacketsReceived(const char * controlURL,
					const char * servicetype);

/* UPNP_GetStatusInfo()
 * status and lastconnerror are 64 byte buffers
 * Return values :
 * UPNPCOMMAND_SUCCESS, UPNPCOMMAND_INVALID_ARGS, UPNPCOMMAND_UNKNOWN_ERROR
 * or a UPnP Error code */
LIBSPEC int
UPNP_GetStatusInfo(const char * controlURL,
			       const char * servicetype,
				   char * status,
				   unsigned int * uptime,
                   char * lastconnerror);

/* UPNP_GetConnectionTypeInfo()
 * argument connectionType is a 64 character buffer
 * Return Values :
 * UPNPCOMMAND_SUCCESS, UPNPCOMMAND_INVALID_ARGS, UPNPCOMMAND_UNKNOWN_ERROR
 * or a UPnP Error code */
LIBSPEC int
UPNP_GetConnectionTypeInfo(const char * controlURL,
                           const char * servicetype,
						   char * connectionType);

/* UPNP_GetExternalIPAddress() call the corresponding UPNP method.
 * if the third arg is not null the value is copied to it.
 * at least 16 bytes must be available 
 *
 * Return values :
 * 0 : SUCCESS
 * NON ZERO : ERROR Either an UPnP error code or an unknown error.
 * 
 * possible UPnP Errors :
 * 402 Invalid Args - See UPnP Device Architecture section on Control.
 * 501 Action Failed - See UPnP Device Architecture section on Control. */
LIBSPEC int
UPNP_GetExternalIPAddress(const char * controlURL,
                          const char * servicetype,
                          char * extIpAdd);

/* UPNP_GetLinkLayerMaxBitRates()
 * call WANCommonInterfaceConfig:1#GetCommonLinkProperties
 *
 * return values :
 * UPNPCOMMAND_SUCCESS, UPNPCOMMAND_INVALID_ARGS, UPNPCOMMAND_UNKNOWN_ERROR
 * or a UPnP Error Code. */
LIBSPEC int
UPNP_GetLinkLayerMaxBitRates(const char* controlURL,
							const char* servicetype,
							unsigned int * bitrateDown,
							unsigned int * bitrateUp);

/* UPNP_AddPortMapping()
 * if desc is NULL, it will be defaulted to "libminiupnpc"
 * remoteHost is usually NULL because IGD don't support it.
 *
 * Return values :
 * 0 : SUCCESS
 * NON ZERO : ERROR. Either an UPnP error code or an unknown error.
 * 
 * List of possible UPnP errors for AddPortMapping :
 * errorCode errorDescription (short) - Description (long)
 * 402 Invalid Args - See UPnP Device Architecture section on Control.
 * 501 Action Failed - See UPnP Device Architecture section on Control.
 * 715 WildCardNotPermittedInSrcIP - The source IP address cannot be
 *                                   wild-carded
 * 716 WildCardNotPermittedInExtPort - The external port cannot be wild-carded
 * 718 ConflictInMappingEntry - The port mapping entry specified conflicts
 *                     with a mapping assigned previously to another client
 * 724 SamePortValuesRequired - Internal and External port values
 *                              must be the same 
 * 725 OnlyPermanentLeasesSupported - The NAT implementation only supports
 *                  permanent lease times on port mappings
 * 726 RemoteHostOnlySupportsWildcard - RemoteHost must be a wildcard
 *                             and cannot be a specific IP address or DNS name
 * 727 ExternalPortOnlySupportsWildcard - ExternalPort must be a wildcard and
 *                                        cannot be a specific port value */
LIBSPEC int
UPNP_AddPortMapping(const char * controlURL, const char * servicetype,
                    const char * extPort,
				    const char * inPort,
					const char * inClient,
					const char * desc,
                    const char * proto,
                    const char * remoteHost);

/* UPNP_DeletePortMapping()
 * Use same argument values as what was used for AddPortMapping().
 * remoteHost is usually NULL because IGD don't support it.
 * Return Values :
 * 0 : SUCCESS
 * NON ZERO : error. Either an UPnP error code or an undefined error.
 *
 * List of possible UPnP errors for DeletePortMapping :
 * 402 Invalid Args - See UPnP Device Architecture section on Control.
 * 714 NoSuchEntryInArray - The specified value does not exist in the array */
LIBSPEC int
UPNP_DeletePortMapping(const char * controlURL, const char * servicetype,
                       const char * extPort, const char * proto,
                       const char * remoteHost);

/* UPNP_GetPortMappingNumberOfEntries()
 * not supported by all routers */
LIBSPEC int
UPNP_GetPortMappingNumberOfEntries(const char* controlURL, const char* servicetype, unsigned int * num);

/* UPNP_GetSpecificPortMappingEntry retrieves an existing port mapping
 * the result is returned in the intClient and intPort strings
 * please provide 16 and 6 bytes of data
 *
 * return value :
 * UPNPCOMMAND_SUCCESS, UPNPCOMMAND_INVALID_ARGS, UPNPCOMMAND_UNKNOWN_ERROR
 * or a UPnP Error Code. */
LIBSPEC int
UPNP_GetSpecificPortMappingEntry(const char * controlURL,
                                 const char * servicetype,
                                 const char * extPort,
                                 const char * proto,
                                 char * intClient,
                                 char * intPort);

/* UPNP_GetGenericPortMappingEntry()
 *
 * return value :
 * UPNPCOMMAND_SUCCESS, UPNPCOMMAND_INVALID_ARGS, UPNPCOMMAND_UNKNOWN_ERROR
 * or a UPnP Error Code.
 *
 * Possible UPNP Error codes :
 * 402 Invalid Args - See UPnP Device Architecture section on Control.
 * 713 SpecifiedArrayIndexInvalid - The specified array index is out of bounds
 */
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
								char * duration);

#ifdef __cplusplus
}
#endif

#endif


/* $Id: upnperrors.c,v 1.3 2008/04/27 17:21:51 nanard Exp $ */
/* Project : miniupnp
 * Author : Thomas BERNARD
 * copyright (c) 2007 Thomas Bernard
 * All Right reserved.
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * This software is subjet to the conditions detailed in the
 * provided LICENCE file. */
#include <string.h>
#include "upnperrors.h"
#include "upnpcommands.h"

const char * strupnperror(int err)
{
	const char * s = NULL;
	switch(err) {
	case UPNPCOMMAND_SUCCESS:
		s = "Success";
		break;
	case UPNPCOMMAND_UNKNOWN_ERROR:
		s = "Miniupnpc Unknown Error";
		break;
	case UPNPCOMMAND_INVALID_ARGS:
		s = "Miniupnpc Invalid Arguments";
		break;
	case 401:
		s = "Invalid Action";
		break;
	case 402:
		s = "Invalid Args";
		break;
	case 501:
		s = "Action Failed";
		break;
	case 713:
		s = "SpecifiedArrayIndexInvalid";
		break;
	case 714:
		s = "NoSuchEntryInArray";
		break;
	case 715:
		s = "WildCardNotPermittedInSrcIP";
		break;
	case 716:
		s = "WildCardNotPermittedInExtPort";
		break;
	case 718:
		s = "ConflictInMappingEntry";
		break;
	case 724:
		s = "SamePortValuesRequired";
		break;
	case 725:
		s = "OnlyPermanentLeasesSupported";
		break;
	case 726:
		s = "RemoteHostOnlySupportsWildcard";
		break;
	case 727:
		s = "ExternalPortOnlySupportsWildcard";
		break;
	default:
		s = NULL;
	}
	return s;
}

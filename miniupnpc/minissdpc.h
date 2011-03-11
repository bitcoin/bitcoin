/* $Id: minissdpc.h,v 1.1 2007/08/31 15:15:33 nanard Exp $ */
/* Project: miniupnp
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * Author: Thomas Bernard
 * Copyright (c) 2005-2007 Thomas Bernard
 * This software is subjects to the conditions detailed
 * in the LICENCE file provided within this distribution */
#ifndef __MINISSDPC_H__
#define __MINISSDPC_H__

struct UPNPDev *
getDevicesFromMiniSSDPD(const char * devtype, const char * socketpath);

#endif


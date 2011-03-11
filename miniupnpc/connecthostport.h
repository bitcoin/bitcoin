/* $Id: connecthostport.h,v 1.1 2010/04/04 23:21:03 nanard Exp $ */
/* Project: miniupnp
 * http://miniupnp.free.fr/
 * Author: Thomas Bernard
 * Copyright (c) 2010 Thomas Bernard
 * This software is subjects to the conditions detailed
 * in the LICENCE file provided within this distribution */
#ifndef __CONNECTHOSTPORT_H__
#define __CONNECTHOSTPORT_H__

/* connecthostport()
 * return a socket connected (TCP) to the host and port
 * or -1 in case of error */
int connecthostport(const char * host, unsigned short port);

#endif


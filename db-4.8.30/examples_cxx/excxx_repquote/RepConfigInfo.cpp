/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include <cstdlib>
#include <cstring>

#include "RepConfigInfo.h"

RepConfigInfo::RepConfigInfo()
{
    start_policy = DB_REP_ELECTION;
    home = NULL;
    got_listen_address = false;
    totalsites = 0;
    priority = 100;
    verbose = false;
    other_hosts = NULL;
    ack_policy = DB_REPMGR_ACKS_QUORUM;
    bulk = false;
}

RepConfigInfo::~RepConfigInfo()
{
    // release any other_hosts structs.
    if (other_hosts != NULL) {
        REP_HOST_INFO *CurItem = other_hosts;
        while (CurItem->next != NULL) {
            REP_HOST_INFO *TmpItem = CurItem->next;
            free(CurItem);
            CurItem = TmpItem;
        }
        free(CurItem);
    }
    other_hosts = NULL;
}

void RepConfigInfo::addOtherHost(char* host, int port, bool peer)
{
    REP_HOST_INFO *newinfo;
    newinfo = (REP_HOST_INFO*)malloc(sizeof(REP_HOST_INFO));
    newinfo->host = host;
    newinfo->port = port;
    newinfo->peer = peer;
    if (other_hosts == NULL) {
        other_hosts = newinfo;
        newinfo->next = NULL;
    } else {
        newinfo->next = other_hosts;
        other_hosts = newinfo;
    }
}

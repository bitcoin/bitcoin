/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2006-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include <cstdlib>
#include <cstring>

#include <db_cxx.h>

// Chainable struct used to store host information.
typedef struct RepHostInfoObj{
    char* host;
    u_int16_t port;
    RepHostInfoObj* next; // used for chaining multiple "other" hosts.
} REP_HOST_INFO;

class RepConfigInfo {
public:
    RepConfigInfo();
    virtual ~RepConfigInfo();

    void addOtherHost(char* host, int port);
public:
    u_int32_t start_policy;
    char* home;
    bool got_listen_address;
    REP_HOST_INFO this_host;
    int totalsites;
    int priority;
    // used to store a set of optional other hosts.
    REP_HOST_INFO *other_hosts;
};


RepConfigInfo::RepConfigInfo()
{
    start_policy = DB_REP_ELECTION;
    home = NULL;
    got_listen_address = false;
    totalsites = 0;
    priority = 100;
    other_hosts = NULL;
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

void RepConfigInfo::addOtherHost(char* host, int port)
{
    REP_HOST_INFO *newinfo;
    newinfo = (REP_HOST_INFO*)malloc(sizeof(REP_HOST_INFO));
    newinfo->host = host;
    newinfo->port = port;
    if (other_hosts == NULL) {
        other_hosts = newinfo;
        newinfo->next = NULL;
    } else {
        newinfo->next = other_hosts;
        other_hosts = newinfo;
    }
}

/*-
 * Copyright (c) 2004-2009 Oracle.  All rights reserved.
 *
 * http://www.apache.org/licenses/LICENSE-2.0.txt
 * 
 * authors: George Schlossnagle <george@omniti.com>
 */

#ifndef MOD_DB4_EXPORT_H
#define MOD_DB4_EXPORT_H

#include "db_cxx.h"

#if defined(__cplusplus)
extern "C" {
#endif

int mod_db4_db_env_create(DB_ENV **dbenvp, u_int32_t flags);
int mod_db4_db_create(DB **dbp, DB_ENV *dbenv, u_int32_t flags);
void mod_db4_child_clean_request_shutdown();
void mod_db4_child_clean_process_shutdown();

#if defined(__cplusplus)
}
#endif

#endif

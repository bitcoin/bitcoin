/*-
 * Copyright (c) 2004-2009 Oracle.  All rights reserved.
 *
 * http://www.apache.org/licenses/LICENSE-2.0.txt
 * 
 * authors: George Schlossnagle <george@omniti.com>
 */

#ifndef DB4_UTILS_H
#define DB4_UTILS_H

#include "db_cxx.h"
#include "mod_db4_export.h"

/* locks */
int env_locks_init();
void env_global_rw_lock();
void env_global_rd_lock();
void env_global_unlock();
void env_wait_for_child_crash();
void env_child_crash();
void env_ok_to_proceed();

void env_rsrc_list_init();

int global_ref_count_increase(char *path);
int global_ref_count_decrease(char *path);
int global_ref_count_get(const char *path);
void global_ref_count_clean();

#endif
/* vim: set ts=4 sts=4 expandtab bs=2 ai fdm=marker: */

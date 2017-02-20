/*-
 * Copyright (c) 2004-2009 Oracle.  All rights reserved.
 *
 * http://www.apache.org/licenses/LICENSE-2.0.txt
 * 
 * authors: George Schlossnagle <george@omniti.com>
 */

#ifndef MOD_DB4_SEM_UTILS_H
#define MOD_DB4_SEM_UTILS_H

extern int md4_sem_create(int semnum, unsigned short *start);
extern void md4_sem_destroy(int semid);
extern void md4_sem_lock(int semid, int semnum);
extern void md4_sem_unlock(int semid, int semnum);
extern void md4_sem_wait_for_zero(int semid, int semnum);
extern void md4_sem_set(int semid, int semnum, int value);
extern int md4_sem_get(int semid, int semnum);

/* vim: set ts=4 sts=4 expandtab bs=2 ai : */
#endif

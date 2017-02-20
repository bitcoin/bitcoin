/*-
 * Copyright (c) 2004-2009 Oracle.  All rights reserved.
 *
 * http://www.apache.org/licenses/LICENSE-2.0.txt
 * 
 * authors: George Schlossnagle <george@omniti.com>
 */
extern "C"
{
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"
#include "sem_utils.h"


#include <errno.h>
}
extern int errno;

#if HAVE_SEMUN
/* we have semun, no need to define */
#else
union semun {
    int val;                  /* value for SETVAL */
    struct semid_ds *buf;     /* buffer for IPC_STAT, IPC_SET */
    unsigned short *array;    /* array for GETALL, SETALL */
                              /* Linux specific part: */
    struct seminfo *__buf;    /* buffer for IPC_INFO */
};
#endif

#ifndef SEM_R
# define SEM_R 0444
#endif
#ifndef SEM_A
# define SEM_A 0222
#endif

int md4_sem_create(int semnum, unsigned short *start)
{
    int semid;
    int perms = 0777;
    union semun arg;
    key_t key = 12345;
    int count = 0;
    while((semid = semget(key, semnum, IPC_CREAT | IPC_EXCL | perms)) == -1) {
      if(count++ > 2) {
        return -1;
      }
      if (errno == EEXIST) {
        semid = semget(key, 1, perms);
        md4_sem_destroy(semid);
      }
    }  
    arg.array = start;
    if(semctl(semid, 0, SETALL, arg) < 0) {
      /* destroy (FIXME) and return */
      md4_sem_destroy(semid);
      return -1;
    }
    return semid;
}

void md4_sem_destroy(int semid)
{
    union semun dummy;
    /* semid should always be 0, this clobbers the whole set */
    semctl(semid, 0, IPC_RMID, dummy);
}

void md4_sem_lock(int semid, int semnum)
{
    struct sembuf sops;
    sops.sem_num = semnum;
    sops.sem_op = -1;
    sops.sem_flg = SEM_UNDO;
    semop(semid, &sops, 1);
}
    
void md4_sem_unlock(int semid, int semnum)
{
    struct sembuf sops;
    sops.sem_num = semnum;
    sops.sem_op = 1;
    sops.sem_flg = SEM_UNDO;
    semop(semid, &sops, 1);
}

void md4_sem_wait_for_zero(int semid, int semnum)
{
    struct sembuf sops;
    sops.sem_num = semnum;
    sops.sem_op = 0;
    sops.sem_flg = SEM_UNDO;
    semop(semid, &sops, 1);
}

void md4_sem_set(int semid, int semnum, int value)
{
    union semun arg;
    arg.val = value;
    semctl(semid, semnum, SETALL, arg);
}

int md4_sem_get(int semid, int semnum)
{
    union semun arg;
    return semctl(semid, 0, GETVAL, arg);
}

/* vim: set ts=4 sts=4 expandtab bs=2 ai : */

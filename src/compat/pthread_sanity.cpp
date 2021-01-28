// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Test case for pthread hang using pthread_rwlock_trywrlock
// https://bugs.launchpad.net/ubuntu/+source/glibc/+bug/1864864
// https://sourceware.org/bugzilla/show_bug.cgi?id=23844

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <iostream>
#include <pthread.h>

pthread_rwlock_t rw_lock[1];

void *runner(void *arg) {

   for(int i = 0; i < 1000000; ++i) {
      int ecode = 0;
      int idx = i % 1;
      bool rw = (i & (16|8)) == (16|8);
      
      if(rw) {
         ecode = pthread_rwlock_trywrlock(&rw_lock[idx]);
         if(ecode == EBUSY) {
            pthread_rwlock_wrlock(&rw_lock[idx]);
         }
      } else {
         pthread_rwlock_rdlock(&rw_lock[idx]);
      }

      pthread_rwlock_unlock(&rw_lock[idx]);
   }

   pthread_exit(nullptr);
}

int pthread_rw_sanity_test() {

   pthread_rwlockattr_t rw_attr;
   pthread_t t_list[2];

   std::cout << "Testing pthread trylock_wr\n\n";

   pthread_rwlockattr_init(&rw_attr);

   pthread_rwlock_init(&rw_lock[0], &rw_attr);

   for(int i: {0,1}) {
      pthread_create(&t_list[i], nullptr, runner, nullptr);
   }
   
   for(int i: {0,1}) {
      pthread_join(t_list[i], nullptr);
   }

   pthread_rwlock_destroy(&rw_lock[0]);

   return true;
}
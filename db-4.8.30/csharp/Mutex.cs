/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.Text;

namespace BerkeleyDB {
    internal class Mutex : IDisposable {
        private DatabaseEnvironment env;

        private uint val;
        
        internal Mutex(DatabaseEnvironment owner, uint mutexValue) {
            env = owner;
            val = mutexValue;
        }

        internal void Lock() {
            env.dbenv.mutex_lock(val);
        }

        internal void Unlock() {
            env.dbenv.mutex_unlock(val);
        }

        public void Dispose() {
            env.dbenv.mutex_free(val);
            GC.SuppressFinalize(this);
        }
    }
}

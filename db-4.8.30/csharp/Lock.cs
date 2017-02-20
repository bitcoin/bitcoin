/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.Text;
using BerkeleyDB.Internal;

namespace BerkeleyDB {
    internal class Lock {
        private DB_LOCK dblock;

        internal uint Generation { get { return dblock.gen; } }
        internal uint Index { get { return dblock.ndx; } }
        internal uint Offset { get { return dblock.off; } }
        internal LockMode Mode { get { return LockMode.GetLockMode(dblock.mode); } }

        internal Lock(DB_LOCK lck) {
            dblock = lck;

        }

        static internal DB_LOCK GetDB_LOCK(Lock lck) {
            return lck == null ? null : lck.dblock;
        }
    }
}
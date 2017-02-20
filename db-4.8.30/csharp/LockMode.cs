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
    internal class LockMode {
        internal static LockMode IREAD = new LockMode(db_lockmode_t.DB_LOCK_IREAD);
        internal static LockMode IWR = new LockMode(db_lockmode_t.DB_LOCK_IWR);
        internal static LockMode IWRITE = new LockMode(db_lockmode_t.DB_LOCK_IWRITE);
        internal static LockMode NOT_GRANTED = new LockMode(db_lockmode_t.DB_LOCK_NG);
        internal static LockMode READ = new LockMode(db_lockmode_t.DB_LOCK_READ);
        internal static LockMode READ_UNCOMMITTED = new LockMode(db_lockmode_t.DB_LOCK_READ_UNCOMMITTED);
        internal static LockMode WAIT = new LockMode(db_lockmode_t.DB_LOCK_WAIT);
        internal static LockMode WRITE = new LockMode(db_lockmode_t.DB_LOCK_WRITE);
        internal static LockMode WWRITE = new LockMode(db_lockmode_t.DB_LOCK_WWRITE);

        private db_lockmode_t mode;
        private LockMode(db_lockmode_t m) {
            mode = m;
        }
        static internal db_lockmode_t GetMode(LockMode lm) {
            return lm.mode;
        }
        static internal LockMode GetLockMode(db_lockmode_t m) {
            switch (m) {
                case db_lockmode_t.DB_LOCK_IREAD:
                    return IREAD;
                case db_lockmode_t.DB_LOCK_IWR:
                    return IWR;
                case db_lockmode_t.DB_LOCK_IWRITE:
                    return IWRITE;
                case db_lockmode_t.DB_LOCK_NG:
                    return NOT_GRANTED;
                case db_lockmode_t.DB_LOCK_READ:
                    return READ;
                case db_lockmode_t.DB_LOCK_READ_UNCOMMITTED:
                    return READ_UNCOMMITTED;
                case db_lockmode_t.DB_LOCK_WAIT:
                    return WAIT;
                case db_lockmode_t.DB_LOCK_WRITE:
                    return WRITE;
                case db_lockmode_t.DB_LOCK_WWRITE:
                    return WWRITE;
            }
            throw new ArgumentException("Unknown db_lockmode_t value.");
        }

    }
}

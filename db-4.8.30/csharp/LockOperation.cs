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
    internal class LockOperation {
        internal static LockOperation DUMP = new LockOperation(db_lockop_t.DB_LOCK_DUMP);
        internal static LockOperation GET = new LockOperation(db_lockop_t.DB_LOCK_GET);
        internal static LockOperation GET_TIMEOUT = new LockOperation(db_lockop_t.DB_LOCK_GET_TIMEOUT);
        internal static LockOperation INHERIT = new LockOperation(db_lockop_t.DB_LOCK_INHERIT);
        internal static LockOperation PUT = new LockOperation(db_lockop_t.DB_LOCK_PUT);
        internal static LockOperation PUT_ALL = new LockOperation(db_lockop_t.DB_LOCK_PUT_ALL);
        internal static LockOperation PUT_OBJ = new LockOperation(db_lockop_t.DB_LOCK_PUT_OBJ);
        internal static LockOperation PUT_READ = new LockOperation(db_lockop_t.DB_LOCK_PUT_READ);
        internal static LockOperation TIMEOUT = new LockOperation(db_lockop_t.DB_LOCK_TIMEOUT);
        internal static LockOperation TRADE = new LockOperation(db_lockop_t.DB_LOCK_TRADE);
        internal static LockOperation UPGRADE_WRITE = new LockOperation(db_lockop_t.DB_LOCK_UPGRADE_WRITE);

        private db_lockop_t op;
        private LockOperation(db_lockop_t o) {
            op = o;
        }
        static internal db_lockop_t GetOperation(LockOperation lo) {
            return lo.op;
        }

        static internal LockOperation GetLockOperation(db_lockop_t o) {
            switch (o) {
                case db_lockop_t.DB_LOCK_DUMP:
                    return DUMP;
                case db_lockop_t.DB_LOCK_GET:
                    return GET;
                case db_lockop_t.DB_LOCK_GET_TIMEOUT:
                    return GET_TIMEOUT;
                case db_lockop_t.DB_LOCK_INHERIT:
                    return INHERIT;
                case db_lockop_t.DB_LOCK_PUT:
                    return PUT;
                case db_lockop_t.DB_LOCK_PUT_ALL:
                    return PUT_ALL;
                case db_lockop_t.DB_LOCK_PUT_OBJ:
                    return PUT_OBJ;
                case db_lockop_t.DB_LOCK_PUT_READ:
                    return PUT_READ;
                case db_lockop_t.DB_LOCK_TIMEOUT:
                    return TIMEOUT;
                case db_lockop_t.DB_LOCK_TRADE:
                    return TRADE;
                case db_lockop_t.DB_LOCK_UPGRADE_WRITE:
                    return UPGRADE_WRITE;
            }
            throw new ArgumentException("Unknown db_lockop_t value.");
        }
    }
}

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
    /// <summary>
    /// A class to represent what lock request(s) should be rejected during
    /// deadlock resolution.
    /// </summary>
    public class DeadlockPolicy {
        private uint flag;
        internal uint policy { get { return flag; } }

        private DeadlockPolicy(uint val) {
            flag = val;
        }

        internal static DeadlockPolicy fromPolicy(uint val) {
            switch (val) {
                case DbConstants.DB_LOCK_DEFAULT:
                    return DEFAULT;
                case DbConstants.DB_LOCK_EXPIRE:
                    return EXPIRE;
                case DbConstants.DB_LOCK_MAXLOCKS:
                    return MAX_LOCKS;
                case DbConstants.DB_LOCK_MAXWRITE:
                    return MAX_WRITE;
                case DbConstants.DB_LOCK_MINLOCKS:
                    return MIN_LOCKS;
                case DbConstants.DB_LOCK_MINWRITE:
                    return MIN_WRITE;
                case DbConstants.DB_LOCK_OLDEST:
                    return OLDEST;
                case DbConstants.DB_LOCK_RANDOM:
                    return RANDOM;
                case DbConstants.DB_LOCK_YOUNGEST:
                    return YOUNGEST;
            }
            throw new ArgumentException("Invalid deadlock policy.");
        }

        /// <summary>
        /// If no DeadlockPolicy has yet been specified, use
        /// <see cref="RANDOM"/>.
        /// </summary>
        public static DeadlockPolicy DEFAULT =
            new DeadlockPolicy(DbConstants.DB_LOCK_DEFAULT);
        /// <summary>
        /// Reject lock requests which have timed out. No other deadlock
        /// detection is performed.
        /// </summary>
        public static DeadlockPolicy EXPIRE =
            new DeadlockPolicy(DbConstants.DB_LOCK_EXPIRE);
        /// <summary>
        /// Reject the lock request for the locker ID with the most locks.
        /// </summary>
        public static DeadlockPolicy MAX_LOCKS =
            new DeadlockPolicy(DbConstants.DB_LOCK_MAXLOCKS);
        /// <summary>
        /// Reject the lock request for the locker ID with the most write locks.
        /// </summary>
        public static DeadlockPolicy MAX_WRITE =
            new DeadlockPolicy(DbConstants.DB_LOCK_MAXWRITE);
        /// <summary>
        /// Reject the lock request for the locker ID with the fewest locks.
        /// </summary>
        public static DeadlockPolicy MIN_LOCKS =
            new DeadlockPolicy(DbConstants.DB_LOCK_MINLOCKS);
        /// <summary>
        /// Reject the lock request for the locker ID with the fewest write
        /// locks.
        /// </summary>
        public static DeadlockPolicy MIN_WRITE =
            new DeadlockPolicy(DbConstants.DB_LOCK_MINWRITE);
        /// <summary>
        /// Reject the lock request for the locker ID with the oldest lock.
        /// </summary>
        public static DeadlockPolicy OLDEST =
            new DeadlockPolicy(DbConstants.DB_LOCK_OLDEST);
        /// <summary>
        /// Reject the lock request for a random locker ID.
        /// </summary>
        public static DeadlockPolicy RANDOM =
            new DeadlockPolicy(DbConstants.DB_LOCK_RANDOM);
        /// <summary>
        /// Reject the lock request for the locker ID with the youngest lock.
        /// </summary>
        public static DeadlockPolicy YOUNGEST =
            new DeadlockPolicy(DbConstants.DB_LOCK_YOUNGEST);
    }
}

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
    /// A class representing the locking options for Berkeley DB operations.
    /// </summary>
    public class LockingInfo {
        /// <summary>
        /// The isolation degree of the operation.
        /// </summary>
        public Isolation IsolationDegree;
        /// <summary>
        /// If true, acquire write locks instead of read locks when doing a
        /// read, if locking is configured.
        /// </summary>
        /// <remarks>
        /// Setting ReadModifyWrite can eliminate deadlock during a
        /// read-modify-write cycle by acquiring the write lock during the read
        /// part of the cycle so that another thread of control acquiring a read
        /// lock for the same item, in its own read-modify-write cycle, will not
        /// result in deadlock.
        /// </remarks>
        public bool ReadModifyWrite;

        /// <summary>
        /// Instantiate a new LockingInfo object
        /// </summary>
        public LockingInfo() {
            IsolationDegree = Isolation.DEGREE_THREE;
            ReadModifyWrite = false;
        }

        internal uint flags {
            get {
                uint ret = 0;
                switch (IsolationDegree) {
                    case (Isolation.DEGREE_ONE):
                        ret |= DbConstants.DB_READ_UNCOMMITTED;
                        break;
                    case (Isolation.DEGREE_TWO):
                        ret |= DbConstants.DB_READ_COMMITTED;
                        break;
                }
                if (ReadModifyWrite)
                    ret |= DbConstants.DB_RMW;
                return ret;
            }
        }

    }
}
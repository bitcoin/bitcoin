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
    /// <summary>
    /// A class representing a unique identifier for a thread of control in a
    /// Berkeley DB application.
    /// </summary>
    public class DbThreadID {
        /// <summary>
        /// The Process ID of the thread of control
        /// </summary>
        public int processID;
        /// <summary>
        /// The Thread ID of the thread of control
        /// </summary>
        public uint threadID;
        /// <summary>
        /// Instantiate a new DbThreadID object
        /// </summary>
        /// <param name="pid">The Process ID of the thread of control</param>
        /// <param name="tid">The Thread ID of the thread of control</param>
        public DbThreadID(int pid, uint tid) {
            processID = pid;
            threadID = tid;
        }
    }
}
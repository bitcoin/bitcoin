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
    /// Statistical information about the Replication Manager
    /// </summary>
    public class RepMgrStats {
        private Internal.RepMgrStatStruct st;
        internal RepMgrStats(Internal.RepMgrStatStruct stats) {
            st = stats;
        }

        /// <summary>
        /// Existing connections dropped. 
        /// </summary>
        public ulong DroppedConnections { get { return st.st_connection_drop; } }
        /// <summary>
        /// # msgs discarded due to excessive queue length.
        /// </summary>
        public ulong DroppedMessages { get { return st.st_msgs_dropped; } }
        /// <summary>
        /// Failed new connection attempts. 
        /// </summary>
        public ulong FailedConnections { get { return st.st_connect_fail; } }
        /// <summary>
        /// # of insufficiently ack'ed msgs. 
        /// </summary>
        public ulong FailedMessages { get { return st.st_perm_failed; } }
        /// <summary>
        /// # msgs queued for network delay. 
        /// </summary>
        public ulong QueuedMessages { get { return st.st_msgs_queued; } }
    }
}

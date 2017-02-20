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
    /// The AckPolicy class specifies how master and client sites will handle
    /// acknowledgment of replication messages which are necessary for
    /// "permanent" records. The current implementation requires all sites in a
    /// replication group configure the same acknowledgement policy. 
    /// </summary>
    public class AckPolicy {
        private int _value;
        internal int Policy {
            get { return _value; }
            private set { _value = value; }
        }

        private AckPolicy(int policyValue) {
            Policy = policyValue;
        }

        internal static AckPolicy fromInt(int value) {
            switch (value) {
                case 0:
                    return null;
                case DbConstants.DB_REPMGR_ACKS_ALL:
                    return ALL;
                case DbConstants.DB_REPMGR_ACKS_ALL_PEERS:
                    return ALL_PEERS;
                case DbConstants.DB_REPMGR_ACKS_NONE:
                    return NONE;
                case DbConstants.DB_REPMGR_ACKS_ONE:
                    return ONE;
                case DbConstants.DB_REPMGR_ACKS_ONE_PEER:
                    return ONE_PEER;
                case DbConstants.DB_REPMGR_ACKS_QUORUM:
                    return QUORUM;
                default:
                    throw new ArgumentException("Unknown AckPolicy value.");
            }
        }
        /// <summary>
        /// The master should wait until all replication clients have
        /// acknowledged each permanent replication message.
        /// </summary>
        public static AckPolicy ALL =
            new AckPolicy(DbConstants.DB_REPMGR_ACKS_ALL);
        /// <summary>
        /// The master should wait until all electable peers have acknowledged
        /// each permanent replication message (where "electable peer" means a
        /// client capable of being subsequently elected master of the
        /// replication group).
        /// </summary>
        public static AckPolicy ALL_PEERS =
            new AckPolicy(DbConstants.DB_REPMGR_ACKS_ALL_PEERS);
        /// <summary>
        /// The master should not wait for any client replication message
        /// acknowledgments.
        /// </summary>
        public static AckPolicy NONE =
            new AckPolicy(DbConstants.DB_REPMGR_ACKS_NONE);
        /// <summary>
        /// The master should wait until at least one client site has
        /// acknowledged each permanent replication message. 
        /// </summary>
        public static AckPolicy ONE =
            new AckPolicy(DbConstants.DB_REPMGR_ACKS_ONE);
        /// <summary>
        /// The master should wait until at least one electable peer has
        /// acknowledged each permanent replication message (where "electable
        /// peer" means a client capable of being subsequently elected master of
        /// the replication group).
        /// </summary>
        public static AckPolicy ONE_PEER =
              new AckPolicy(DbConstants.DB_REPMGR_ACKS_ONE_PEER);
        /// <summary>
        /// The master should wait until it has received acknowledgements from
        /// the minimum number of electable peers sufficient to ensure that the
        /// effect of the permanent record remains durable if an election is
        /// held (where "electable peer" means a client capable of being
        /// subsequently elected master of the replication group). This is the
        /// default acknowledgement policy.
        /// </summary>
        public static AckPolicy QUORUM =
              new AckPolicy(DbConstants.DB_REPMGR_ACKS_QUORUM);
    }
}

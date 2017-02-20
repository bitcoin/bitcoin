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
    /// A class representing a replication site used by Replication Manager
    /// </summary>
    public class RepMgrSite {
        
        /// <summary>
        /// Environment ID assigned by the replication manager. This is the same
        /// value that is passed to
        /// <see cref="DatabaseEnvironment.EventNotify"/> for the
        /// <see cref="NotificationEvent.REP_NEWMASTER"/> event.
        /// </summary>
        public int EId;
        /// <summary>
        /// The address of the site
        /// </summary>
        public ReplicationHostAddress Address;
        /// <summary>
        /// If true, the site is connected.
        /// </summary>
        public bool isConnected;

        internal RepMgrSite(DB_REPMGR_SITE site) {
            EId = site.eid;
            Address = new ReplicationHostAddress(site.host, site.port);
            isConnected = (site.status == DbConstants.DB_REPMGR_CONNECTED);
        }
    }
}

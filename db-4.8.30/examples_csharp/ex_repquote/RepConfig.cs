/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.Text;

using BerkeleyDB;

namespace ex_repquote
{
    public class RepConfig
    {
        /* Constant values used in the RepQuote application. */
        public static CacheInfo CACHESIZE = new CacheInfo(0, 10 * 1024 * 1024, 1);
        public static int SLEEPTIME = 5000;
        public static string progname = "RepQuoteExample";

        /* Member variables containing configuration information. */
        public AckPolicy ackPolicy;
        public bool bulk; /* Whether bulk transfer should be performed. */
        public string home; /* The home directory for rep files. */
        public ReplicationHostAddress host; /* Host address to listen to. */
        public uint priority; /* Priority within the replication group. */
        public List<RemoteSite> remote;
        public StartPolicy startPolicy; 
        
        /* Optional value specifying the # of sites in the replication group. */
        public uint totalSites;
        public bool verbose;
        
        public RepConfig()
        {
            ackPolicy = AckPolicy.QUORUM;
            bulk = false;
            home = "";
            host = new ReplicationHostAddress();
            priority = 100;
            remote = new List<RemoteSite>();
            startPolicy = StartPolicy.ELECTION;
        
            totalSites = 0;
            verbose = false;
        }
    }

    public enum StartPolicy { CLIENT, ELECTION, MASTER };

    public class RemoteSite 
    {
        private ReplicationHostAddress host;
        private bool isPeer;

        public RemoteSite(string host, uint port, bool isPeer)
        {
            this.host = new ReplicationHostAddress(host, port);
            this.isPeer = isPeer;
        }

        public ReplicationHostAddress Host { get { return host; } }
        public bool IsPeer { get { return isPeer; } }
    }
}

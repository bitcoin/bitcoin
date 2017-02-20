/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Threading;

using BerkeleyDB;

/**
 * RepQuoteExample is a simple but complete demonstration of a replicated
 * application. The application is a mock stock ticker. The master accepts a
 * stock symbol and an numerical value as input, and stores this information
 * into a replicated database; either master or clients can display the
 * contents of the database.
 * 
 * The options to start a given replication node are:
 * 
 *   -h home (required; h stands for home directory)
 *   -l host:port (required; l stands for local)
 *   -C or M (optional; start up as client or master)
 *   -r host:port (optional; r stands for remote; any number of these may
 *      be specified)
 *   -R host:port (optional; R stands for remote peer; only one of these may
 *      be specified)
 *   -a all|quorum (optional; a stands for ack policy)
 *   -b (optional; b stands for bulk)
 *   -n nsites (optional; number of sites in replication group; defaults to 0
 *      to try to dynamically compute nsites)
 *   -p priority (optional; defaults to 100)
 *   -v (optional; v stands for verbose)
 * 
 * A typical session begins with a command such as the following to start a
 * master:
 *
 * ex_repquote.exe -M -h dir1 -l localhost:6000
 * 
 * and several clients:
 *
 * ex_repquote.exe -C -h dir2
 *               -l localhost:6001 -r localhost:6000
 * ex_repquote.exe -C -h dir3
 *               -l localhost:6002 -r localhost:6000
 * ex_repquote.exe -C -h dir4
 *               -l localhost:6003 -r localhost:6000
 * 
 * Each process is a member of a DB replication group. The sample application
 * expects the following commands to stdin:
 * 
 * NEWLINE -- print all the stocks held in the database
 * quit -- shutdown this node
 * exit -- shutdown this node
 * stock_symbol number -- enter this stock and number into the database
 */

namespace ex_repquote
{
    public class RepQuoteExample
    {
        private RepQuoteEnvironment dbenv;
        private Thread checkpointThread;
        private Thread logArchiveThread;

        public static void usage()
        {
            Console.WriteLine(
                "usage: " + RepConfig.progname +
                " -h home -l host:port [-CM][-r host:port][-R host:port]\n" +
                "  [-a all|quorum][-b][-n nsites][-p priority][-v]");

            Console.WriteLine(
                "\t -h home (required; h stands for home directory)\n" +
                "\t -l host:port (required; l stands for local)\n" +
                "\t -C or -M (optional; start up as client or master)\n" +
                "\t -r host:port (optional; r stands for remote; any number " +
                "of these\n" +
                "\t    may be specified)\n" +
                "\t -R host:port (optional; R stands for remote peer; only " +
                "one of\n" +
                "\t    these may be specified)\n" +
                "\t -a all|quorum (optional; a stands for ack policy)\n" +
                "\t -b (optional; b stands for bulk)\n" +
                "\t -n nsites (optional; number of sites in replication " +
                "group; defaults\n" +
                "\t    to 0 to try to dynamically compute nsites)\n" +
                "\t -p priority (optional; defaults to 100)\n" +
                "\t -v (optional; v stands for verbose)\n");

            Environment.Exit(1);
        }

        public static void Main(string[] args)
        {
            RepConfig config = new RepConfig();
            bool isPeer;
            uint tmpPort = 0;

            /*
             * RepQuoteExample is meant to be run from build_windows\AnyCPU, in
             * either the Debug or Release directory. The required core
             * libraries, however, are in either build_windows\Win32 or
             * build_windows\x64, depending upon the platform.  That location
             * needs to be added to the PATH environment variable for the
             * P/Invoke calls to work.
             */
            try {
                String pwd = Environment.CurrentDirectory;
                pwd = Path.Combine(pwd, "..");
                pwd = Path.Combine(pwd, "..");
                if (IntPtr.Size == 4)
                    pwd = Path.Combine(pwd, "Win32");
                else
                    pwd = Path.Combine(pwd, "x64");
#if DEBUG
                pwd = Path.Combine(pwd, "Debug");
#else
                pwd = Path.Combine(pwd, "Release");
#endif
                pwd += ";" + Environment.GetEnvironmentVariable("PATH");
                Environment.SetEnvironmentVariable("PATH", pwd);
            } catch (Exception e) {
                Console.WriteLine(
                    "Unable to set the PATH environment variable.");
                Console.WriteLine(e.Message);
                return;
            }
            
            /*  Extract the command line parameters. */
            for (int i = 0; i < args.Length; i++)
            {
                isPeer = false;
                string s = args[i];
                if (s[0] != '-')
                    continue;
                switch (s[1])
                {
                    case 'a':
                        if (i == args.Length - 1)
                            usage();
                        i++;
                        if (args[i].Equals("all"))
                            config.ackPolicy = AckPolicy.ALL;
                        else if (!args[i].Equals("quorum"))
                            usage();
                        break;
                    case 'b':
                        config.bulk = true;
                        break;
                    case 'C':
                        config.startPolicy = StartPolicy.CLIENT;
                        break;
                    case 'h':
                        if (i == args.Length - 1)
                            usage();
                        i++;
                        config.home = args[i];
                        break;
                    case 'l':
                        if (i == args.Length - 1)
                            usage();
                        i++;
                        string[] words = args[i].Split(':');
                        if (words.Length != 2)
                        {
                            Console.Error.WriteLine("Invalid host " + 
                                "specification host:port needed.");
                            usage();
                        }
                        try 
                        {
                            tmpPort = uint.Parse(words[1]);
                        } catch (InvalidCastException)
                        {
                            Console.Error.WriteLine("Invalid host " + 
                                "specification, could not parse port number.");
                            usage();
                        }
                        config.host.Host = words[0];
                        config.host.Port = tmpPort;
                        break;
                    case 'M':
                        config.startPolicy = StartPolicy.MASTER;
                        break;
                    case 'n':
                        if (i == args.Length - 1)
                            usage();
                        i++;
                        try 
                        {
                            config.totalSites = uint.Parse(args[i]);
                        } catch (InvalidCastException) 
                        {
                            Console.Error.WriteLine(
                            "Unable to parse number of total sites.");
                            usage();
                        }
                        break;
                    case 'p':
                        if (i == args.Length - 1)
                            usage();
                        i++;
                        try 
                        {
                            config.priority = uint.Parse(args[i]);
                        } catch (InvalidCastException) 
                        {
                            Console.Error.WriteLine("Unable to parse priority.");
                            usage();
                        }
                        break;
                    case 'r':
                    case 'R':
                        if (i == args.Length - 1)
                            usage();
                        if (args[i].Equals("R"))
                            isPeer = true;
                        i++;
                        words = args[i].Split(':');
                        if (words.Length != 2)
                        {
                            Console.Error.WriteLine("Invalid host " + 
                                "specification host:port needed.");
                            usage();
                        }
                        try 
                        {
                            tmpPort = uint.Parse(words[1]);
                        } catch (InvalidCastException)
                        {
                            Console.Error.WriteLine("Invalid host " + 
                                "specification, could not parse port number.");
                            usage();
                        }
                        config.remote.Add(
                            new RemoteSite(words[0], tmpPort, isPeer));
                        break;
                    case 'v':
                        config.verbose = true;
                        break;
                    default:
                        Console.Error.WriteLine(
                            "Unrecognized option: " + args[i]);
                        usage();    
                        break;
                }
            }

            /* Error check command line. */
            if (config.host.Host == null || config.home.Length == 0)
                usage();

            RepQuoteExample runner = null;
            try 
            {
                runner = new RepQuoteExample();
                runner.init(config);
                runner.doloop();
                runner.terminate();
                runner = null;
            } catch (DatabaseException dbErr) 
            {
                Console.Error.WriteLine("Caught an exception during " +
                    "initialization or processing: " + dbErr);
                if (runner != null)
                    runner.terminate();
            }
        } /* End main. */
        
        public RepQuoteExample()
        {
            dbenv = null;
        }
        
        public int init(RepConfig config)
        {
            int ret = 0;
            
            DatabaseEnvironmentConfig envConfig = new DatabaseEnvironmentConfig();
            envConfig.ErrorPrefix = RepConfig.progname;
            envConfig.RepSystemCfg = new ReplicationConfig();
            envConfig.RepSystemCfg.RepMgrLocalSite = config.host;
            for (int i = 0; i < config.remote.Count; i++)
                envConfig.RepSystemCfg.AddRemoteSite(config.remote[i].Host, 
                    config.remote[i].IsPeer);

            if (config.totalSites > 0)
                envConfig.RepSystemCfg.NSites = config.totalSites;

            envConfig.RepSystemCfg.BulkTransfer = config.bulk;
            
            /*
             * Configure heartbeat timeouts so that repmgr monitors the
             * health of the TCP connection.  Master sites broadcast a heartbeat
             * at the frequency specified by the DB_REP_HEARTBEAT_SEND timeout.
             * Client sites wait for message activity the length of the 
             * DB_REP_HEARTBEAT_MONITOR timeout before concluding that the 
             * connection to the master is lost.  The DB_REP_HEARTBEAT_MONITOR 
             * timeout should be longer than the DB_REP_HEARTBEAT_SEND timeout.
             */
            envConfig.RepSystemCfg.HeartbeatMonitor = 10000000;
            envConfig.RepSystemCfg.HeartbeatSend = 5000000;

            /* 
             * Set replication group election priority for this environment.
             * An election first selects the site with the most recent log
             * records as the new master.  If multiple sites have the most
             * recent log records, the site with the highest priority value 
             * is selected as master.
             */
            envConfig.RepSystemCfg.Priority = config.priority;
            envConfig.MPoolSystemCfg = new MPoolConfig();
            envConfig.MPoolSystemCfg.CacheSize = RepConfig.CACHESIZE;
            envConfig.TxnNoSync = true;

            envConfig.EventNotify = new EventNotifyDelegate(RepQuoteEventHandler);

            /*
             * Set the policy that determines how master and client sites
             * handle acknowledgement of replication messages needed for
             * permanent records.  The default policy of "quorum" requires only
             * a quorum of electable peers sufficient to ensure a permanent
             * record remains durable if an election is held.  The "all" option
             * requires all clients to acknowledge a permanent replication
             * message instead.
             */
            envConfig.RepSystemCfg.RepMgrAckPolicy = config.ackPolicy;

            /*
             * Set the threshold for the minimum and maximum time the client
             * waits before requesting retransmission of a missing message.
             * Base these values on the performance and load characteristics
             * of the master and client host platforms as well as the round
             * trip message time.
             */
            envConfig.RepSystemCfg.RetransmissionRequest(20000, 500000);
            
            /*
             * Configure deadlock detection to ensure that any deadlocks
             * are broken by having one of the conflicting lock requests
             * rejected. DB_LOCK_DEFAULT uses the lock policy specified
             * at environment creation time or DB_LOCK_RANDOM if none was
             * specified.
             */
            envConfig.LockSystemCfg = new LockingConfig();
            envConfig.LockSystemCfg.DeadlockResolution = DeadlockPolicy.DEFAULT;

            envConfig.Create = true;
            envConfig.RunRecovery = true;
            envConfig.FreeThreaded = true;
            envConfig.UseReplication = true;
            envConfig.UseLocking = true;
            envConfig.UseLogging = true;
            envConfig.UseMPool = true;
            envConfig.UseTxns = true;
            envConfig.Verbosity = new VerboseMessages();
            envConfig.Verbosity.Replication = config.verbose;
            
            try 
            {
                dbenv = RepQuoteEnvironment.Open(config.home, envConfig);
            } catch(DatabaseException e) 
            {
                Console.WriteLine("Fail to open environment: " + e.Message);
                return 1;
            }

            
            /* The following base replication features may also be useful to your
             * application. See Berkeley DB documentation for more details.
             *   - Master leases: Provide stricter consistency for data reads
             *     on a master site.
             *   - Timeouts: Customize the amount of time Berkeley DB waits
             *     for such things as an election to be concluded or a master
             *     lease to be granted.
             *   - Delayed client synchronization: Manage the master site's
             *     resources by spreading out resource-intensive client 
             *     synchronizations.
             *   - Blocked client operations: Return immediately with an error
             *     instead of waiting indefinitely if a client operation is
             *     blocked by an ongoing client synchronization.
             *
             * The following repmgr features may also be useful to your
             * application.  See Berkeley DB documentation for more details.
             *  - Two-site strict majority rule - In a two-site replication
             *    group, require both sites to be available to elect a new
             *    master.
             *  - Timeouts - Customize the amount of time repmgr waits
             *    for such things as waiting for acknowledgements or attempting 
             *    to reconnect to other sites.
             *  - Site list - return a list of sites currently known to repmgr.
             */

            /* Start checkpoint and log archive support threads. */ 
            checkpointThread = new Thread(new ThreadStart(CheckPoint));
            checkpointThread.Start();
            logArchiveThread = new Thread(new ThreadStart(LogArchive));
            logArchiveThread.Start();

            /* Start replication manager. */
            if (config.startPolicy == StartPolicy.CLIENT)
                dbenv.env.RepMgrStartClient(3);
            else if (config.startPolicy == StartPolicy.ELECTION)
                dbenv.env.RepMgrStartClient(3, true);
            else if (config.startPolicy == StartPolicy.MASTER)
                dbenv.env.RepMgrStartMaster(3);
                
            return ret;
        }
        
        public int doloop()
        {
            BTreeDatabase db = null;
                
            for (;;)
            {
                if (db == null) 
                {
                    BTreeDatabaseConfig dbConfig = new BTreeDatabaseConfig();
                    dbConfig.Env = dbenv.env;

                    if (dbenv.IsMaster) 
                    {
                        /*
                         * Open database allowing create only if this is a master
                         * database.  A client database uses polling to attempt
                         * to open the database, without creating it, until the
                         * open succeeds.
                         *
                         * This polling logic for allowing create can be 
                         * simplified under some circumstances.  For example, if
                         * the application can be sure a database is already
                         * there, it would never need to open it allowing create.
                         */
                            dbConfig.Creation = CreatePolicy.IF_NEEDED;
                    }

                    dbConfig.AutoCommit = true;

                    try 
                    {
                        db = BTreeDatabase.Open(RepConfig.progname, dbConfig);
                    } catch (DatabaseException) 
                    {
                        Console.WriteLine("no stock database available yet.");
                        if (db != null) 
                        {
                            db.Close(true);
                            db = null;
                        }

                        Thread.Sleep(RepConfig.SLEEPTIME);
                        continue;
                    }
                }

                /* Listen for input, and add it to the database. */
                Console.Write("QUOTESERVER");
                if (dbenv.IsMaster == false)
                    Console.Write("(read-only)");
                Console.Write("> ");
                string nextLine = null;
                try
                {
                    nextLine = Console.ReadLine();
                } catch(System.IO.IOException)
                {
                    Console.WriteLine("Unable to get data");
                    break;
                }

                 /* A blank line causes the DB to be dumped. */
                string[] words = nextLine.Split(' ');
                if (words.Length == 0 ||
                    words.Length == 1 && words[0].Length == 0) 
                {
                    try
                    {
                        if (dbenv.InClientSync)
                            Console.WriteLine("Cannot read data during " +
                                "client initialization - please try again.");
                        else
                            printStocks(db);
                    } catch (DeadlockException) 
                    {
                        continue;
                    } catch (DatabaseException e) 
                    {
                        /*
                         * This could be DB_REP_HANDLE_DEAD, which
                         * should close the database and continue.
                         */
                        Console.WriteLine("Got db exception reading replication"
                            + "DB: " + e);
                        Console.WriteLine("Expected if it was due to a dead " +
                            "replication handle, otherwise an unexpected error.");
                        db.Close(false);/* Close no sync. */
                        db = null;
                        continue;
                    }
                    continue;
                }

                if (words.Length == 1 &&
                    (words[0].ToLower().Equals("quit") || 
                    words[0].ToLower().Equals("exit"))) 
                {
                    dbenv.AppFinished = true;
                    break;
                } else if (words.Length != 2) 
                {
                    Console.WriteLine("Format: TICKER VALUE");
                    continue;
                }

                if (!dbenv.IsMaster)
                {
                    Console.WriteLine("Can't update client");
                    continue;
                }

                DatabaseEntry key = new DatabaseEntry(
                    ASCIIEncoding.ASCII.GetBytes(words[0]));
                DatabaseEntry data = new DatabaseEntry(
                    ASCIIEncoding.ASCII.GetBytes(words[1]));

                db.Put(key, data);
            }

            if (db != null)
                db.Close(true);

            return 0;
        }

        public void terminate()
        {
            /* Wait for checkpoint and log archive threads to finish. */
            try 
            {
                logArchiveThread.Join();
                checkpointThread.Join();
            } catch (Exception) 
            {
                Console.WriteLine("Support thread join failed.");
            }

            /*
             * We have used the DB_TXN_NOSYNC environment flag for improved
             * performance without the usual sacrifice of transactional durability,
             * as discussed in the "Transactional guarantees" page of the Reference
             * Guide: if one replication site crashes, we can expect the data to
             * exist at another site.  However, in case we shut down all sites
             * gracefully, we push out the end of the log here so that the most
             * recent transactions don't mysteriously disappear.
             */
            dbenv.env.LogFlush();
            dbenv.env.Close();
        }

        /*
         * void return type since error conditions are propogated
         * via exceptions.
         */
        private void printStocks(Database db)
        {
            Cursor dbc = db.Cursor();
            Console.WriteLine("\tSymbol\t\tPrice");
            Console.WriteLine("\t======\t\t=====");
            DatabaseEntry key = new DatabaseEntry();
            DatabaseEntry data = new DatabaseEntry();

            foreach (KeyValuePair<DatabaseEntry, DatabaseEntry> pair in dbc)
            {
                string keyStr = ASCIIEncoding.ASCII.GetString(pair.Key.Data);
                string dataStr = ASCIIEncoding.ASCII.GetString(pair.Value.Data);
                Console.WriteLine("\t"+keyStr+"\t\t"+dataStr);
            }
            dbc.Close();
        }

        /*
         * This is a very simple thread that performs checkpoints at a fixed
         * time interval.  For a master site, the time interval is one minute
         * plus the duration of the checkpoint_delay timeout (30 seconds by
         * default.)  For a client site, the time interval is one minute.
         */
        public void CheckPoint()
        {
            for (; ; )
            {
                /*
                 * Wait for one minute, polling once per second to see if
                 * application has finished.  When application has finished,
                 * terminate this thread.
                 */
                for (int i = 0; i < 60; i++)
                {
                    Thread.Sleep(1000);
                    if (dbenv.AppFinished)
                        return;
                }

                /* Perform a checkpoint. */
                try
                {
                    dbenv.env.Checkpoint();
                } catch (DatabaseException)
                {
                    Console.WriteLine("Could not perform checkpoint.");
                }
            }
        }


        /*
         * This is a simple log archive thread.  Once per minute, it removes all but
         * the most recent 3 logs that are safe to remove according to a call to
         * DBENV->log_archive().
         *
         * Log cleanup is needed to conserve disk space, but aggressive log cleanup
         * can cause more frequent client initializations if a client lags too far
         * behind the current master.  This can happen in the event of a slow client,
         * a network partition, or a new master that has not kept as many logs as the
         * previous master.
         *
         * The approach in this routine balances the need to mitigate against a
         * lagging client by keeping a few more of the most recent unneeded logs
         * with the need to conserve disk space by regularly cleaning up log files.
         * Use of automatic log removal (DBENV->log_set_config() DB_LOG_AUTO_REMOVE 
         * flag) is not recommended for replication due to the risk of frequent 
         * client initializations.
         */
        public void LogArchive()
        {
            int logKeep = 3;
            int minLog;
            List<string> logFileList;

            for (; ; )
            {
                /*
                 * Wait for one minute, polling once per second to see if
                 * application has finished.  When application has finished,
                 * terminate this thread.
                 */
                for (int i = 0; i < 60; i++)
                {
                    Thread.Sleep(1000);
                    if (dbenv.AppFinished)
                        return;
                }

                try
                {
                    /* Get the list of unneeded log files. */
                    logFileList = dbenv.env.ArchivableLogFiles(true);
                    /* 
                     * Remove all but the logs_to_keep most recent unneeded
                     * log files. 
                     */
                    minLog = logFileList.Count - logKeep;
                    for (int i = 0; i < minLog; i++)
                    {
                        FileInfo logFile = new FileInfo(logFileList[i]);
                        logFile.Delete();
                    }
                } catch (DatabaseException)
                {
                    Console.WriteLine("Problem deleting log archive files.");
                }
            }
        }


        
        /*
         * Implemention of EventHandler interface to handle the Berkeley DB events
         * we are interested in receiving.
         */
        private void RepQuoteEventHandler(NotificationEvent eventCode, byte[] event_info)
        {
            switch (eventCode) 
            {
                case NotificationEvent.REP_CLIENT:
                    dbenv.IsMaster = false;
                    dbenv.InClientSync = true;
                    break;
                case NotificationEvent.REP_MASTER:
                    dbenv.IsMaster = true;
                    dbenv.InClientSync = false;
                    break;
                case NotificationEvent.REP_NEWMASTER:
                    dbenv.InClientSync = true;
                    break;
                case NotificationEvent.REP_STARTUPDONE:
                    dbenv.InClientSync = false;
                    break;
                /*
                 * Did not get enough acks to guarantee transaction
                 * durability based on the configured ack policy.  This 
                 * transaction will be flushed to the master site's
                 * local disk storage for durability.
                 */ 
                case NotificationEvent.REP_PERM_FAILED:
                    Console.WriteLine("Insufficient acknowledgements " +
                        "to guarantee transaction durability.");
                    break;
                default:
                    Console.WriteLine("Ignoring event: {0}", eventCode);
                    break;
            }
        }
    }
}

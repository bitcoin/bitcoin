/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package db.repquote;

import java.io.FileNotFoundException;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.IOException;
import java.lang.Thread;
import java.lang.InterruptedException;

import com.sleepycat.db.*;
import db.repquote.RepConfig;

/**
 * RepQuoteExample is a simple but complete demonstration of a replicated
 * application. The application is a mock stock ticker. The master accepts a
 * stock symbol and an numerical value as input, and stores this information
 * into a replicated database; either master or clients can display the
 * contents of the database.
 * <p>
 * The options to start a given replication node are:
 * <pre>
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
 * </pre>
 * <p>
 * A typical session begins with a command such as the following to start a
 * master:
 *
 * <pre>
 *   java db.repquote.RepQuoteExample -M -h dir1 -l localhost:6000
 * </pre>
 *
 * and several clients:
 *
 * <pre>
 *   java db.repquote.RepQuoteExample -C -h dir2
 *               -l localhost:6001 -r localhost:6000
 *   java db.repquote.RepQuoteExample -C -h dir3
 *               -l localhost:6002 -r localhost:6000
 *   java db.repquote.RepQuoteExample -C -h dir4
 *               -l localhost:6003 -r localhost:6000
 * </pre>
 *
 * <p>
 * Each process is a member of a DB replication group. The sample application
 * expects the following commands to stdin:
 * <ul>
 * <li>NEWLINE -- print all the stocks held in the database</li>
 * <li>quit -- shutdown this node</li>
 * <li>exit -- shutdown this node</li>
 * <li>stock_symbol number -- enter this stock and number into the
 * database</li>
 * </ul>
 */

public class RepQuoteExample
{
    private RepConfig appConfig;
    private RepQuoteEnvironment dbenv;
    private CheckpointThread ckpThr;
    private LogArchiveThread lgaThr;

    public static void usage()
    {
        System.err.println("usage: " + RepConfig.progname +
            " -h home -l host:port [-CM][-r host:port][-R host:port]\n" +
            "  [-a all|quorum][-b][-n nsites][-p priority][-v]");

        System.err.println(
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

        System.exit(1);
    }

    public static void main(String[] argv)
        throws Exception
    {
        RepConfig config = new RepConfig();
        boolean isPeer;
        String tmpHost;
        int tmpPort = 0;

        /*  Extract the command line parameters. */
        for (int i = 0; i < argv.length; i++)
        {
            isPeer = false;
            if (argv[i].compareTo("-a") == 0) {
                if (i == argv.length - 1)
                    usage();
                i++;
                if (argv[i].equals("all"))
                    config.ackPolicy = ReplicationManagerAckPolicy.ALL;
                else if (!argv[i].equals("quorum"))
                    usage();
            } else if (argv[i].compareTo("-b") == 0)
                config.bulk = true;
            else if (argv[i].compareTo("-C") == 0) {
                config.startPolicy = ReplicationManagerStartPolicy.REP_CLIENT;
            } else if (argv[i].compareTo("-h") == 0) {
                if (i == argv.length - 1)
                    usage();
                /* home - a string arg. */
                i++;
                config.home = argv[i];
            } else if (argv[i].compareTo("-l") == 0) {
                if (i == argv.length - 1)
                    usage();
                /* "local" should be host:port. */
                i++;
                String[] words = argv[i].split(":");
                if (words.length != 2) {
                    System.err.println(
                        "Invalid host specification host:port needed.");
                    usage();
                }
                try {
                    tmpPort = Integer.parseInt(words[1]);
                } catch (NumberFormatException nfe) {
                    System.err.println("Invalid host specification, " +
                        "could not parse port number.");
                    usage();
                }
                config.setThisHost(words[0], tmpPort);
            } else if (argv[i].compareTo("-M") == 0) {
                config.startPolicy = ReplicationManagerStartPolicy.REP_MASTER;
            } else if (argv[i].compareTo("-n") == 0) {
                if (i == argv.length - 1)
                    usage();
                i++;
                config.totalSites = Integer.parseInt(argv[i]);
            } else if (argv[i].compareTo("-p") == 0) {
                if (i == argv.length - 1)
                    usage();
                i++;
                config.priority = Integer.parseInt(argv[i]);
            } else if (argv[i].compareTo("-R") == 0 ||
                argv[i].compareTo("-r") == 0) {
                if (i == argv.length - 1)
                    usage();
                if (argv[i].equals("-R"))
                    isPeer = true;
                i++;
                String[] words = argv[i].split(":");
                if (words.length != 2) {
                    System.err.println(
                        "Invalid host specification host:port needed.");
                    usage();
                }
                try {
                    tmpPort = Integer.parseInt(words[1]);
                } catch (NumberFormatException nfe) {
                    System.err.println("Invalid host specification, " +
                        "could not parse port number.");
                    usage();
                }
                config.addOtherHost(words[0], tmpPort, isPeer);
            } else if (argv[i].compareTo("-v") == 0) {
                config.verbose = true;
            } else {
                System.err.println("Unrecognized option: " + argv[i]);
                usage();
            }

        }

        /* Error check command line. */
        if ((!config.gotListenAddress()) || config.home.length() == 0)
            usage();

        RepQuoteExample runner = null;
        try {
            runner = new RepQuoteExample();
            runner.init(config);

            /* Sleep to give ourselves time to find a master. */
            //try {
            //    Thread.sleep(5000);
            //} catch (InterruptedException e) {}

            runner.doloop();
            runner.terminate();
        } catch (DatabaseException dbe) {
            System.err.println("Caught an exception during " +
                "initialization or processing: " + dbe);
            if (runner != null)
                runner.terminate();
        }
    } /* End main. */

    public RepQuoteExample()
        throws DatabaseException
    {
        appConfig = null;
        dbenv = null;
    }

    public int init(RepConfig config)
        throws DatabaseException
    {
        int ret = 0;
        appConfig = config;
        EnvironmentConfig envConfig = new EnvironmentConfig();
        envConfig.setErrorStream(System.err);
        envConfig.setErrorPrefix(RepConfig.progname);

        envConfig.setReplicationManagerLocalSite(appConfig.getThisHost());
        for (RepRemoteHost host = appConfig.getFirstOtherHost();
            host != null; host = appConfig.getNextOtherHost()){
            envConfig.replicationManagerAddRemoteSite(
                host.getAddress(), host.isPeer());
        }
        if (appConfig.totalSites > 0)
            envConfig.setReplicationNumSites(appConfig.totalSites);

        /* 
         * Set replication group election priority for this environment.
         * An election first selects the site with the most recent log
         * records as the new master.  If multiple sites have the most
         * recent log records, the site with the highest priority value 
         * is selected as master.
         */
        envConfig.setReplicationPriority(appConfig.priority);

        envConfig.setCacheSize(RepConfig.CACHESIZE);
        envConfig.setTxnNoSync(true);

        envConfig.setEventHandler(new RepQuoteEventHandler());

        /*
         * Set the policy that determines how master and client sites
         * handle acknowledgement of replication messages needed for
         * permanent records.  The default policy of "quorum" requires only
         * a quorum of electable peers sufficient to ensure a permanent
         * record remains durable if an election is held.  The "all" option
         * requires all clients to acknowledge a permanent replication
         * message instead.
         */
        envConfig.setReplicationManagerAckPolicy(appConfig.ackPolicy);

        /*
         * Set the threshold for the minimum and maximum time the client
         * waits before requesting retransmission of a missing message.
         * Base these values on the performance and load characteristics
         * of the master and client host platforms as well as the round
         * trip message time.
         */
        envConfig.setReplicationRequestMin(20000);
        envConfig.setReplicationRequestMax(500000);

        /*
         * Configure deadlock detection to ensure that any deadlocks
         * are broken by having one of the conflicting lock requests
         * rejected. DB_LOCK_DEFAULT uses the lock policy specified
         * at environment creation time or DB_LOCK_RANDOM if none was
         * specified.
         */
        envConfig.setLockDetectMode(LockDetectMode.DEFAULT);

        envConfig.setAllowCreate(true);
        envConfig.setRunRecovery(true);
        envConfig.setThreaded(true);
        envConfig.setInitializeReplication(true);
        envConfig.setInitializeLocking(true);
        envConfig.setInitializeLogging(true);
        envConfig.setInitializeCache(true);
        envConfig.setTransactional(true);
        envConfig.setVerboseReplication(appConfig.verbose);
        try {
            dbenv = new RepQuoteEnvironment(appConfig.getHome(), envConfig);
        } catch(FileNotFoundException e) {
            System.err.println("FileNotFound exception: " + e);
            System.err.println(
                "Ensure that the environment directory is pre-created.");
            ret = 1;
        }
        
        if (appConfig.bulk)
            dbenv.setReplicationConfig(ReplicationConfig.BULK, true);

        /*
         * Configure heartbeat timeouts so that repmgr monitors the
         * health of the TCP connection.  Master sites broadcast a heartbeat
         * at the frequency specified by the DB_REP_HEARTBEAT_SEND timeout.
         * Client sites wait for message activity the length of the 
         * DB_REP_HEARTBEAT_MONITOR timeout before concluding that the 
         * connection to the master is lost.  The DB_REP_HEARTBEAT_MONITOR 
         * timeout should be longer than the DB_REP_HEARTBEAT_SEND timeout.
         */
        dbenv.setReplicationTimeout(ReplicationTimeoutType.HEARTBEAT_SEND, 
            5000000);
        dbenv.setReplicationTimeout(ReplicationTimeoutType.HEARTBEAT_MONITOR, 
            10000000);

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
        ckpThr = new CheckpointThread(dbenv);
        ckpThr.start();
        lgaThr = new LogArchiveThread(dbenv, envConfig);
        lgaThr.start();

        /* Start replication manager. */
        dbenv.replicationManagerStart(3, appConfig.startPolicy);
        
        return ret;
    }

    public int doloop()
        throws DatabaseException
    {
        Database db = null;

        for (;;)
        {
            if (db == null) {
                DatabaseConfig dbconf = new DatabaseConfig();
                dbconf.setType(DatabaseType.BTREE);
                if (dbenv.getIsMaster()) {
                    /*
                     * Open database allowing create only if this is a master
                     * database.  A client database uses polling to attempt
                     * to open the database without allowing create until
                     * it is successful.
                     *
                     * This polling logic for allowing create can be 
                     * simplified under some circumstances.  For example, if
                     * the application can be sure a database is already
                     * there, it would never need to open it allowing create.
                     */
                    dbconf.setAllowCreate(true);
                }
                dbconf.setTransactional(true);

                try {
                    db = dbenv.openDatabase
                        (null, RepConfig.progname, null, dbconf);
                } catch (java.io.FileNotFoundException e) {
                    System.err.println("no stock database available yet.");
                    if (db != null) {
                        db.close(true);
                        db = null;
                    }
                    try {
                        Thread.sleep(RepConfig.SLEEPTIME);
                    } catch (InterruptedException ie) {}
                    continue;
                }
            }

            BufferedReader stdin =
                new BufferedReader(new InputStreamReader(System.in));

            /* Listen for input, and add it to the database. */
            System.out.print("QUOTESERVER");
            if (!dbenv.getIsMaster())
                System.out.print("(read-only)");
            System.out.print("> ");
            System.out.flush();
            String nextline = null;
            try {
                nextline = stdin.readLine();
            } catch (IOException ioe) {
                System.err.println("Unable to get data from stdin");
                break;
            }
            String[] words = nextline.split("\\s");

            /* A blank line causes the DB to be dumped to stdout. */
            if (words.length == 0 ||
                (words.length == 1 && words[0].length() == 0)) {
                try {
                    if (dbenv.getInClientSync())
                        System.err.println(
    "Cannot read data during client initialization - please try again.");
                    else
                        printStocks(db);
                } catch (DeadlockException de) {
                    continue;
                } catch (DatabaseException e) {
                    /*
                     * This could be DB_REP_HANDLE_DEAD, which
                     * should close the database and continue.
                     */
                    System.err.println("Got db exception reading replication" +
                        "DB: " + e);
                    System.err.println("Expected if it was due to a dead " +
                        "replication handle, otherwise an unexpected error.");
                    db.close(true); /* Close no sync. */
                    db = null;
                    continue;
                }
                continue;
            }

            if (words.length == 1 &&
                (words[0].compareToIgnoreCase("quit") == 0 ||
                words[0].compareToIgnoreCase("exit") == 0)) {
                    dbenv.setAppFinished(true);
                    break;
            } else if (words.length != 2) {
                System.err.println("Format: TICKER VALUE");
                continue;
            }

            if (!dbenv.getIsMaster()) {
                System.err.println("Can't update client.");
                continue;
            }

            DatabaseEntry key = new DatabaseEntry(words[0].getBytes());
            DatabaseEntry data = new DatabaseEntry(words[1].getBytes());

            db.put(null, key, data);
        }
        if (db != null)
            db.close(true);
        return 0;
    }

    public void terminate()
        throws DatabaseException
    {
        /* Wait for checkpoint and log archive threads to finish. */
        try {
            lgaThr.join();
            ckpThr.join();
        } catch (Exception e1) {
            System.err.println("Support thread join failed.");
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
        dbenv.logFlush(null);
        
        dbenv.close();
    }

    /*
     * void return type since error conditions are propogated
     * via exceptions.
     */
    private void printStocks(Database db)
        throws DeadlockException, DatabaseException
    {
        Cursor dbc = db.openCursor(null, null);

        System.out.println("\tSymbol\tPrice");
        System.out.println("\t======\t=====");

        DatabaseEntry key = new DatabaseEntry();
        DatabaseEntry data = new DatabaseEntry();
        OperationStatus ret;
        for (ret = dbc.getFirst(key, data, LockMode.DEFAULT);
            ret == OperationStatus.SUCCESS;
            ret = dbc.getNext(key, data, LockMode.DEFAULT)) {
            String keystr = new String
                (key.getData(), key.getOffset(), key.getSize());
            String datastr = new String
                (data.getData(), data.getOffset(), data.getSize());
            System.out.println("\t"+keystr+"\t"+datastr);

        }
        dbc.close();
    }

    /*
     * Implemention of EventHandler interface to handle the Berkeley DB events
     * we are interested in receiving.
     */
    private /* internal */
    class RepQuoteEventHandler extends EventHandlerAdapter {
        public void handleRepClientEvent()
        {
            dbenv.setIsMaster(false);
            dbenv.setInClientSync(true);
        }
        public void handleRepMasterEvent()
        {
            dbenv.setIsMaster(true);
            dbenv.setInClientSync(false);
        }
        public void handleRepNewMasterEvent()
        {
            dbenv.setInClientSync(true);
        }
        public void handleRepPermFailedEvent()
        {
            /*
             * Did not get enough acks to guarantee transaction
             * durability based on the configured ack policy.  This 
             * transaction will be flushed to the master site's
             * local disk storage for durability.
             */
            System.err.println(
    "Insufficient acknowledgements to guarantee transaction durability.");
        }
        public void handleRepStartupDoneEvent()
        {
            dbenv.setInClientSync(false);
        }
    }
} /* End RepQuoteEventHandler class. */

/*
 * This is a very simple thread that performs checkpoints at a fixed
 * time interval.  For a master site, the time interval is one minute
 * plus the duration of the checkpoint_delay timeout (30 seconds by
 * default.)  For a client site, the time interval is one minute.
 */
class CheckpointThread extends Thread {
    private RepQuoteEnvironment myEnv = null;

    public CheckpointThread(RepQuoteEnvironment env) {
        myEnv = env;
    }

    public void run() {
        for (;;) {
            /*
             * Wait for one minute, polling once per second to see if
             * application has finished.  When application has finished,
             * terminate this thread.
             */
            for (int i = 0; i < 60; i++) {
                try {
                    Thread.sleep(1000);
                } catch (InterruptedException ie) {}
                if (myEnv.getAppFinished())
                    return;
            }

            /* Perform a checkpoint. */
            try {
                myEnv.checkpoint(null);
            } catch (DatabaseException de) {
                System.err.println("Could not perform checkpoint.");
            }
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
class LogArchiveThread extends Thread {
    private RepQuoteEnvironment myEnv = null;
    private EnvironmentConfig myEnvConfig = null;

    public LogArchiveThread(RepQuoteEnvironment env, 
        EnvironmentConfig envConfig) {
        myEnv = env;
        myEnvConfig = envConfig;
    }

    public void run() {
        java.io.File[] logFileList;
        int logs_to_keep = 3;
        int minlog;

        for (;;) {
            /*
             * Wait for one minute, polling once per second to see if
             * application has finished.  When application has finished,
             * terminate this thread.
             */
            for (int i = 0; i < 60; i++) {
                try {
                    Thread.sleep(1000);
                } catch (InterruptedException ie) {}
                if (myEnv.getAppFinished())
                    return;
            }

            try {
                /* Get the list of unneeded log files. */
                logFileList = myEnv.getArchiveLogFiles(false);
                /* 
                 * Remove all but the logs_to_keep most recent unneeded
                 * log files. 
                 */
                minlog = logFileList.length - logs_to_keep;
                for (int i = 0; i < minlog; i++) {
                    logFileList[i].delete();
                }
            } catch (DatabaseException de) {
                System.err.println("Problem deleting log archive files.");
            }
        }
    }
}

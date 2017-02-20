/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

// NOTE: This example is a simplified version of the RepQuoteExample.java
// example that can be found in the db/examples_java/src/db/repquote directory.
//
// This example is intended only as an aid in learning Replication Manager
// concepts. It is not complete in that many features are not exercised 
// in it, nor are many error conditions properly handled.

package db.repquote_gsg;

import java.io.FileNotFoundException;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.lang.Thread;
import java.lang.InterruptedException;

import com.sleepycat.db.Cursor;
import com.sleepycat.db.Database;
import com.sleepycat.db.DatabaseConfig;
import com.sleepycat.db.DatabaseEntry;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.DeadlockException;
import com.sleepycat.db.DatabaseType;
import com.sleepycat.db.EnvironmentConfig;
import com.sleepycat.db.EventHandler;
import com.sleepycat.db.LockMode;
import com.sleepycat.db.OperationStatus;
import com.sleepycat.db.ReplicationHandleDeadException;
import com.sleepycat.db.ReplicationHostAddress;
import com.sleepycat.db.ReplicationManagerStartPolicy;
import com.sleepycat.db.ReplicationManagerAckPolicy;
import db.repquote_gsg.RepConfig;

public class RepQuoteExampleGSG implements EventHandler
{
    private RepConfig repConfig;
    private RepQuoteEnvironment dbenv;

    public static void usage()
    {
        System.err.println("usage: " + RepConfig.progname);
        System.err.println("-h home -l host:port [-r host:port]" +
            "[-n nsites][-p priority]");

        System.err.println("\t -h home directory (required)\n" +
             "\t -l host:port (required; l stands for local)\n" +
             "\t -r host:port (optional; r stands for remote; any " +
             "number of these\n" +
             "\t    may be specified)\n" +
             "\t -n nsites (optional; number of sites in replication " +
             "group; defaults\n" +
             "\t    to 0 to try to dynamically compute nsites)\n" +
             "\t -p priority (optional; defaults to 100)\n");

        System.exit(1);
    }

    public static void main(String[] argv)
        throws Exception
    {
        RepConfig config = new RepConfig();
        String tmpHost;
        int tmpPort = 0;
        // Extract the command line parameters.
        for (int i = 0; i < argv.length; i++)
        {
            if (argv[i].compareTo("-h") == 0) {
                // home is a string arg.
                i++;
                config.home = argv[i];
            } else if (argv[i].compareTo("-l") == 0) {
                // "local" should be host:port.
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
            } else if (argv[i].compareTo("-n") == 0) {
                i++;
                config.totalSites = Integer.parseInt(argv[i]);
            } else if (argv[i].compareTo("-p") == 0) {
                i++;
                config.priority = Integer.parseInt(argv[i]);
            } else if (argv[i].compareTo("-r") == 0) {
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
                config.addOtherHost(words[0], tmpPort);
            } else {
                System.err.println("Unrecognized option: " + argv[i]);
                usage();
            }

        }

        // Error check command line.
        if ((!config.gotListenAddress()) || config.home.length() == 0)
            usage();

        RepQuoteExampleGSG runner = null;
        try {
            runner = new RepQuoteExampleGSG();
            runner.init(config);

            runner.doloop();
            runner.terminate();
        } catch (DatabaseException dbe) {
            System.err.println("Caught an exception during " +
                "initialization or processing: " + dbe.toString());
            if (runner != null)
                runner.terminate();
        }
            System.exit(0);
    } // end main

    public RepQuoteExampleGSG()
        throws DatabaseException
    {
        repConfig = null;
        dbenv = null;
    }

    public int init(RepConfig config)
        throws DatabaseException
    {
        int ret = 0;
        repConfig = config;
        EnvironmentConfig envConfig = new EnvironmentConfig();
        envConfig.setErrorStream(System.err);
        envConfig.setErrorPrefix(RepConfig.progname);

        envConfig.setReplicationManagerLocalSite(repConfig.getThisHost());
        for (ReplicationHostAddress host = repConfig.getFirstOtherHost();
            host != null; host = repConfig.getNextOtherHost())
            envConfig.replicationManagerAddRemoteSite(host, false);

        if (repConfig.totalSites > 0)
            envConfig.setReplicationNumSites(repConfig.totalSites);
        envConfig.setReplicationPriority(repConfig.priority);

        envConfig.setReplicationManagerAckPolicy(
            ReplicationManagerAckPolicy.ALL);
        envConfig.setCacheSize(RepConfig.CACHESIZE);
        envConfig.setTxnNoSync(true);

        envConfig.setEventHandler(this);

        envConfig.setAllowCreate(true);
        envConfig.setRunRecovery(true);
        envConfig.setThreaded(true);
        envConfig.setInitializeReplication(true);
        envConfig.setInitializeLocking(true);
        envConfig.setInitializeLogging(true);
        envConfig.setInitializeCache(true);
        envConfig.setTransactional(true);
        try {
            dbenv = new RepQuoteEnvironment(repConfig.getHome(), envConfig);
        } catch(FileNotFoundException e) {
            System.err.println("FileNotFound exception: " + e.toString());
            System.err.println(
                "Ensure that the environment directory is pre-created.");
            ret = 1;
        }

        // Start Replication Manager.
        dbenv.replicationManagerStart(3, repConfig.startPolicy);
        return ret;
    }

    // Provides the main data processing function for our application.
    // This function provides a command line prompt to which the user
    // can provide a ticker string and a stock price.  Once a value is
    // entered to the application, the application writes the value to
    // the database and then displays the entire database.
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
                    dbconf.setAllowCreate(true);
                }
                dbconf.setTransactional(true);

                try {
                    db = dbenv.openDatabase
                        (null, RepConfig.progname, null, dbconf);
                } catch (java.io.FileNotFoundException e) {
                    System.err.println("No stock database available yet.");
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

            // Listen for input, and add it to the database.
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

            // A blank line causes the DB to be dumped to stdout.
            if (words.length == 0 || 
                (words.length == 1 && words[0].length() == 0)) {
                try {
                    printStocks(db);
                } catch (DeadlockException de) {
                    continue;
                // Dead replication handles are cased by an election
                // resulting in a previously committing read becoming
                // invalid.  Close the db handle and reopen.
                } catch (ReplicationHandleDeadException rhde) {
                    db.close(true); // close no sync.
                    db = null;
                    continue;
                } catch (DatabaseException e) {
                    System.err.println("Got db exception reading replication" +
                        "DB: " + e.toString());
                    break;
                }
                continue;
            }

            if (words.length == 1 &&
                (words[0].compareToIgnoreCase("quit") == 0 ||
                words[0].compareToIgnoreCase("exit") == 0)) {
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
            dbenv.close();
    }

    public void handleRepClientEvent()
    {
        dbenv.setIsMaster(false);
    }

    public void handleRepMasterEvent()
    {
        dbenv.setIsMaster(true);
    }

    public void handleRepNewMasterEvent(int envId)
    {
        // Ignored for now.
    }

    public void handleWriteFailedEvent(int errorCode)
    {
        System.err.println("Write to stable storage failed!" +
            "Operating system error code:" + errorCode);
        System.err.println("Continuing....");
    }

    public void handleRepStartupDoneEvent()
    {
        // Ignored for now.
    }

    public void handleRepPermFailedEvent()
    {
	// Ignored for now.
    }

    public void handleRepElectedEvent()
    {
        // Safely ignored for Replication Manager applications.
    }

    public void handlePanicEvent()
    {
        System.err.println("Panic encountered!");
        System.err.println("Shutting down.");
        System.err.println("You should restart, running recovery.");
        try {
            terminate();
        } catch (DatabaseException dbe) {
            System.err.println("Caught an exception during " +
                "termination in handlePanicEvent: " + dbe.toString());
        }
        System.exit(-1);
    }

    // Display all the stock quote information in the database.
    // Return type is void because error conditions are propagated
    // via exceptions.
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
} // end class


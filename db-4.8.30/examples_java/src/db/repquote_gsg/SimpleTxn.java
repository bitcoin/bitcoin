/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package db.repquote_gsg;

import java.io.FileNotFoundException;
// BufferedReader and InputStreamReader needed for our command line 
// prompt.
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.IOException;
import java.io.UnsupportedEncodingException;

import com.sleepycat.db.Cursor;
import com.sleepycat.db.Database;
import com.sleepycat.db.DatabaseConfig;
import com.sleepycat.db.DatabaseEntry;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.DatabaseType;
import com.sleepycat.db.Environment;
import com.sleepycat.db.EnvironmentConfig;
import com.sleepycat.db.LockMode;
import com.sleepycat.db.OperationStatus;
import db.repquote_gsg.SimpleConfig;

public class SimpleTxn
{
    private SimpleConfig simpleConfig;
    private Environment dbenv;

    public SimpleTxn()
        throws DatabaseException
    {
        simpleConfig = null;
        dbenv = null;
    }

    public static void usage()
    {
        System.err.println("usage: " + SimpleConfig.progname + " -h home");
        System.exit(1);
    }

    public static void main(String[] argv)
        throws Exception
    {
        SimpleConfig config = new SimpleConfig();
        // Extract the command line parameters.
        for (int i = 0; i < argv.length; i++)
        {
            if (argv[i].compareTo("-h") == 0) {
                // home - a string arg.
                i++;
                config.home = argv[i];
            } else {
                System.err.println("Unrecognized option: " + argv[i]);
                usage();
            }
        }

        // Error check command line.
        if (config.home.length() == 0)
            usage();

        SimpleTxn runner = null;
        try {
            runner = new SimpleTxn();
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

    public int init(SimpleConfig config)
        throws DatabaseException
    {
        int ret = 0;
        simpleConfig = config;
        EnvironmentConfig envConfig = new EnvironmentConfig();
        envConfig.setErrorStream(System.err);
        envConfig.setErrorPrefix(SimpleConfig.progname);

        envConfig.setCacheSize(SimpleConfig.CACHESIZE);
        envConfig.setTxnNoSync(true);

        envConfig.setAllowCreate(true);
        envConfig.setRunRecovery(true);
        envConfig.setInitializeLocking(true);
        envConfig.setInitializeLogging(true);
        envConfig.setInitializeCache(true);
        envConfig.setTransactional(true);
        try {
            dbenv = new Environment(simpleConfig.getHome(), envConfig);
        } catch(FileNotFoundException e) {
            System.err.println("FileNotFound exception: " + e.toString());
            System.err.println(
                "Ensure that the environment directory is pre-created.");
            ret = 1;
        }

        return ret;
    }

    // Provides the main data processing function for our application.
    // This function provides a command line prompt to which the user
    // can provide a ticker string and a stock price.  Once a value is
    // entered to the application, the application writes the value to
    // the database and then displays the entire database.
    public int doloop()
        throws DatabaseException, UnsupportedEncodingException
    {
        Database db = null;

        for (;;)
        {
            if (db == null) {
                DatabaseConfig dbconf = new DatabaseConfig();
                dbconf.setType(DatabaseType.BTREE);
                dbconf.setAllowCreate(true);
                dbconf.setTransactional(true);

                try {
                    db = dbenv.openDatabase(null,              // txn handle
                                        SimpleConfig.progname, // db filename
                                        null,                  // db name
                                        dbconf);
                } catch (FileNotFoundException fnfe) {
                    System.err.println("File not found exception" + 
                        fnfe.toString());
                    // Get here only if the environment home directory
                    // somehow does not exist.
               }
            }

            BufferedReader stdin =
                new BufferedReader(new InputStreamReader(System.in));

            // Listen for input, and add it to the database.
            System.out.print("QUOTESERVER> ");
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
                } catch (DatabaseException e) {
                    System.err.println("Got db exception reading " +
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

            DatabaseEntry key = 
                    new DatabaseEntry(words[0].getBytes("UTF-8"));
            DatabaseEntry data = 
                    new DatabaseEntry(words[1].getBytes("UTF-8"));

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

    // Display all the stock quote information in the database.
    // Return type is void because error conditions are propagated
    // via exceptions.
    private void printStocks(Database db)
        throws DatabaseException
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


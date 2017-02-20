/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package collections.access;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintStream;
import java.util.Iterator;
import java.util.Map;
import java.util.SortedMap;

import com.sleepycat.bind.ByteArrayBinding;
import com.sleepycat.collections.StoredSortedMap;
import com.sleepycat.collections.TransactionRunner;
import com.sleepycat.collections.TransactionWorker;
import com.sleepycat.db.Database;
import com.sleepycat.db.DatabaseConfig;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.DatabaseType;
import com.sleepycat.db.Environment;
import com.sleepycat.db.EnvironmentConfig;

/**
 *  AccesssExample mirrors the functionality of a class by the same name
 * used to demonstrate the com.sleepycat.je Java API. This version makes
 * use of the new com.sleepycat.collections.* collections style classes to make
 * life easier.
 *
 *@author     Gregory Burd
 *@created    October 22, 2002
 */
public class AccessExample
         implements Runnable {

    // Class Variables of AccessExample class
    private static boolean create = true;
    private static final int EXIT_SUCCESS = 0;
    private static final int EXIT_FAILURE = 1;

    public static void usage() {

	System.out.println("usage: java " + AccessExample.class.getName() +
            " [-r] [database]\n");
	System.exit(EXIT_FAILURE);
    }

    /**
     *  The main program for the AccessExample class
     *
     *@param  argv  The command line arguments
     */
    public static void main(String[] argv) {

	boolean removeExistingDatabase = false;
	String databaseName = "access.db";

	for (int i = 0; i < argv.length; i++) {
	    if (argv[i].equals("-r")) {
		removeExistingDatabase = true;
	    } else if (argv[i].equals("-?")) {
		usage();
	    } else if (argv[i].startsWith("-")) {
		usage();
	    } else {
		if ((argv.length - i) != 1)
		    usage();
		databaseName = argv[i];
		break;
	    }
	}

        try {

            EnvironmentConfig envConfig = new EnvironmentConfig();
            envConfig.setTransactional(true);
            envConfig.setInitializeCache(true);
            envConfig.setInitializeLocking(true);
            if (create) {
                envConfig.setAllowCreate(true);
            }
            Environment env = new Environment(new File("."), envConfig);
	    // Remove the previous database.
	    if (removeExistingDatabase) {
                env.removeDatabase(null, databaseName, null);
            }

            // create the app and run it
            AccessExample app = new AccessExample(env, databaseName);
            app.run();
            app.close();
        } catch (DatabaseException e) {
            e.printStackTrace();
            System.exit(1);
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            System.exit(1);
        } catch (Exception e) {
            e.printStackTrace();
            System.exit(1);
        }
        System.exit(0);
    }


    private Database db;
    private SortedMap map;
    private Environment env;


    /**
     *  Constructor for the AccessExample object
     *
     *@param  env            Description of the Parameter
     *@exception  Exception  Description of the Exception
     */
    public AccessExample(Environment env, String databaseName)
	throws Exception {

        this.env = env;

        //
        // Lets mimic the db.AccessExample 100%
        // and use plain old byte arrays to store the key and data strings.
        //
        ByteArrayBinding keyBinding = new ByteArrayBinding();
        ByteArrayBinding dataBinding = new ByteArrayBinding();

        //
        // Open a data store.
        //
        DatabaseConfig dbConfig = new DatabaseConfig();
        if (create) {
            dbConfig.setAllowCreate(true);
            dbConfig.setType(DatabaseType.BTREE);
        }
        this.db = env.openDatabase(null, databaseName, null, dbConfig);

        //
        // Now create a collection style map view of the data store
        // so that it is easy to work with the data in the database.
        //
        this.map = new StoredSortedMap(db, keyBinding, dataBinding, true);
    }

    /**
     * Close the database and environment.
     */
    void close()
	throws DatabaseException {

        db.close();
        env.close();
    }

    /**
     *  Main processing method for the AccessExample object
     */
    public void run() {
        //
        // Insert records into a Stored Sorted Map DatabaseImpl, where
        // the key is the user input and the data is the user input
        // in reverse order.
        //
        final InputStreamReader reader = new InputStreamReader(System.in);

        for (; ; ) {
            final String line = askForLine(reader, System.out, "input> ");
            if (line == null) {
                break;
            }

            final String reversed =
		(new StringBuffer(line)).reverse().toString();

            log("adding: \"" +
		line + "\" : \"" +
		reversed + "\"");

            // Do the work to add the key/data to the HashMap here.
            TransactionRunner tr = new TransactionRunner(env);
            try {
                tr.run(
		       new TransactionWorker() {
			   public void doWork() {
			       if (!map.containsKey(line.getBytes()))
				   map.put(line.getBytes(),
                                           reversed.getBytes());
			       else
				   System.out.println("Key " + line +
						      " already exists.");
			   }
		       });
            } catch (com.sleepycat.db.DatabaseException e) {
                System.err.println("AccessExample: " + e.toString());
                System.exit(1);
            } catch (java.lang.Exception e) {
                System.err.println("AccessExample: " + e.toString());
                System.exit(1);
            }
        }
        System.out.println("");

        // Do the work to traverse and print the HashMap key/data
        // pairs here get iterator over map entries.
        Iterator iter = map.entrySet().iterator();
        System.out.println("Reading data");
        while (iter.hasNext()) {
            Map.Entry entry = (Map.Entry) iter.next();
            log("found \"" +
                new String((byte[]) entry.getKey()) +
                "\" key with data \"" +
                new String((byte[]) entry.getValue()) + "\"");
        }
    }


    /**
     *  Prompts for a line, and keeps prompting until a non blank line is
     *  returned. Returns null on error.
     *
     *@param  reader  stream from which to read user input
     *@param  out     stream on which to prompt for user input
     *@param  prompt  prompt to use to solicit input
     *@return         the string supplied by the user
     */
    String askForLine(InputStreamReader reader, PrintStream out,
                      String prompt) {

        String result = "";
        while (result != null && result.length() == 0) {
            out.print(prompt);
            out.flush();
            result = getLine(reader);
        }
        return result;
    }


    /**
     *  Read a single line. Gets the line attribute of the AccessExample object
     *  Not terribly efficient, but does the job. Works for reading a line from
     *  stdin or a file.
     *
     *@param  reader  stream from which to read the line
     *@return         either a String or null on EOF, if EOF appears in the
     *      middle of a line, returns that line, then null on next call.
     */
    String getLine(InputStreamReader reader) {

        StringBuffer b = new StringBuffer();
        int c;
        try {
            while ((c = reader.read()) != -1 && c != '\n') {
                if (c != '\r') {
                    b.append((char) c);
                }
            }
        } catch (IOException ioe) {
            c = -1;
        }

        if (c == -1 && b.length() == 0) {
            return null;
        } else {
            return b.toString();
        }
    }


    /**
     *  A simple log method.
     *
     *@param  s  The string to be logged.
     */
    private void log(String s) {

        System.out.println(s);
        System.out.flush();
    }
}

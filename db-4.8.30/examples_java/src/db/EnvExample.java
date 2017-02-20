/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package db;

import com.sleepycat.db.*;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.OutputStream;

/*
 * An example of a program configuring a database environment.
 *
 * For comparison purposes, this example uses a similar structure
 * as examples/ex_env.c and examples_cxx/EnvExample.cpp.
 */
public class EnvExample {
    private static final String progname = "EnvExample";

    private static void runApplication(Environment dbenv)
        throws DatabaseException, FileNotFoundException {
         
        // Open a database in the environment to verify the data_dir
        // has been set correctly.
        DatabaseConfig dbconfig = new DatabaseConfig();

        // The database is DB_BTREE.
        dbconfig.setAllowCreate(true);
        dbconfig.setMode(0644);
        dbconfig.setType(DatabaseType.BTREE);
        Database db=dbenv.openDatabase(null, 
            "jEnvExample_db1.db", null, dbconfig);

        // Close the database.
        db.close();
    }

    private static void setupEnvironment(File home,
                                         File dataDir,
                                         OutputStream errs)
        throws DatabaseException, FileNotFoundException {

        // Create an environment object and initialize it for error reporting.
        EnvironmentConfig config = new EnvironmentConfig();
        config.setErrorStream(errs);
        config.setErrorPrefix(progname);

        //
        // We want to specify the shared memory buffer pool cachesize,
        // but everything else is the default.
        //
        config.setCacheSize(64 * 1024);

        // Databases are in a separate directory.
        config.addDataDir(dataDir);

        // Open the environment with full transactional support.
        config.setAllowCreate(true);
        config.setInitializeCache(true);
        config.setTransactional(true);
        config.setInitializeLocking(true);

        //
        // open is declared to throw a FileNotFoundException, which normally
        // shouldn't occur when allowCreate is set.
        //
        Environment dbenv = new Environment(home, config);

        try {
            // Start your application.
            runApplication(dbenv);
        } finally {
            // Close the environment.  Doing this in the finally block ensures
            // it is done, even if an error is thrown.
            dbenv.close();
        }
    }

    private static void teardownEnvironment(File home,
                                            File dataDir,
                                            OutputStream errs)
        throws DatabaseException, FileNotFoundException {

        // Remove the shared database regions.
        EnvironmentConfig config = new EnvironmentConfig();

        config.setErrorStream(errs);
        config.setErrorPrefix(progname);
        config.addDataDir(dataDir);
        Environment.remove(home, true, config);
    }

    private static void usage() {
        System.err.println("usage: java db.EnvExample [-h home] [-d data_dir]");
        System.exit(1);
    }

    public static void main(String[] argv) {
        //
        // All of the shared database files live in home,
        // but data files live in dataDir.
        //
        // Using Berkeley DB in C/C++, we need to allocate two elements
        // in the array and set config[1] to NULL.  This is not
        // necessary in Java.
        //
        File home = new File("TESTDIR");
        File dataDir = new File("data");

        for (int i = 0; i < argv.length; ++i) {
            if (argv[i].equals("-h")) {
                if (++i >= argv.length)
                    usage();
                home = new File(argv[i]);
            } else if (argv[i].equals("-d")) {
                if (++i >= argv.length)
                    usage();
                dataDir = new File(argv[i]);
            } else if (argv[i].equals("-u")) {
                usage();
            }
        }

        try {
            System.out.println("Setup env");
            setupEnvironment(home, dataDir, System.err);

            System.out.println("Teardown env");
            teardownEnvironment(home, dataDir, System.err);
        } catch (DatabaseException dbe) {
            System.err.println(progname + ": environment open: " + dbe.toString());
            System.exit (1);
        } catch (FileNotFoundException fnfe) {
            System.err.println(progname + ": unexpected open environment error  " + fnfe);
            System.exit (1);
        }
    }

}

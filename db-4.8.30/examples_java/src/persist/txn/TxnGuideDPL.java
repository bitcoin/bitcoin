/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2008-2009 Oracle.  All rights reserved.
 *
 * $Id$ 
 */

// File TxnGuideDPL.java

package persist.txn;

import com.sleepycat.db.DatabaseConfig;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.DatabaseType;
import com.sleepycat.db.LockDetectMode;

import com.sleepycat.db.Environment;
import com.sleepycat.db.EnvironmentConfig;

import com.sleepycat.persist.EntityStore;
import com.sleepycat.persist.StoreConfig;

import java.io.File;
import java.io.FileNotFoundException;

public class TxnGuideDPL {

    private static String myEnvPath = "./";
    private static String storeName = "exampleStore";

    // Handles
    private static EntityStore myStore = null;
    private static Environment myEnv = null;

    private static final int NUMTHREADS = 5;

    private static void usage() {
        System.out.println("TxnGuideDPL [-h <env directory>]");
        System.exit(-1);
    }

    public static void main(String args[]) {
        try {
            // Parse the arguments list
            parseArgs(args);
            // Open the environment and store
            openEnv();

            // Start the threads
            StoreWriter[] threadArray;
            threadArray = new StoreWriter[NUMTHREADS];
            for (int i = 0; i < NUMTHREADS; i++) {
                threadArray[i] = new StoreWriter(myEnv, myStore);
                threadArray[i].start();
            }

            for (int i = 0; i < NUMTHREADS; i++) {
                threadArray[i].join();
            }
        } catch (Exception e) {
            System.err.println("TxnGuideDPL: " + e.toString());
            e.printStackTrace();
        } finally {
            closeEnv();
        }
        System.out.println("All done.");
    }


    private static void openEnv() throws DatabaseException {
        System.out.println("opening env and store");

        // Set up the environment.
        EnvironmentConfig myEnvConfig = new EnvironmentConfig();
        myEnvConfig.setAllowCreate(true);
        myEnvConfig.setInitializeCache(true);
        myEnvConfig.setInitializeLocking(true);
        myEnvConfig.setInitializeLogging(true);
        myEnvConfig.setRunRecovery(true);
        myEnvConfig.setTransactional(true);
        // EnvironmentConfig.setThreaded(true) is the default behavior 
        // in Java, so we do not have to do anything to cause the
        // environment handle to be free-threaded.

        // Indicate that we want db to internally perform deadlock 
        // detection. Also indicate that the transaction that has
        // performed the least amount of write activity to
        // receive the deadlock notification, if any.
        myEnvConfig.setLockDetectMode(LockDetectMode.MINWRITE);

        // Set up the entity store
        StoreConfig myStoreConfig = new StoreConfig();
        myStoreConfig.setAllowCreate(true);
        myStoreConfig.setTransactional(true);

        // Need a DatabaseConfig object so as to set uncommitted read
        // support.
        DatabaseConfig myDbConfig = new DatabaseConfig();
        myDbConfig.setType(DatabaseType.BTREE);
        myDbConfig.setAllowCreate(true);
        myDbConfig.setTransactional(true);
        myDbConfig.setReadUncommitted(true);

        try {
            // Open the environment
            myEnv = new Environment(new File(myEnvPath),    // Env home
                                    myEnvConfig);

            // Open the store
            myStore = new EntityStore(myEnv, storeName, myStoreConfig);

            // Set the DatabaseConfig object, so that the underlying
            // database is configured for uncommitted reads.
            myStore.setPrimaryConfig(PayloadDataEntity.class, myDbConfig);
        } catch (FileNotFoundException fnfe) {
            System.err.println("openEnv: " + fnfe.toString());
            System.exit(-1);
        }
    }

    private static void closeEnv() {
        System.out.println("Closing env and store");
        if (myStore != null ) {
            try {
                myStore.close();
            } catch (DatabaseException e) {
                System.err.println("closeEnv: myStore: " + 
                    e.toString());
                e.printStackTrace();
            }
        }

        if (myEnv != null ) {
            try {
                myEnv.close();
            } catch (DatabaseException e) {
                System.err.println("closeEnv: " + e.toString());
                e.printStackTrace();
            }
        }
    }

    private TxnGuideDPL() {}

    private static void parseArgs(String args[]) {
        int nArgs = args.length;
        for(int i = 0; i < args.length; ++i) {
            if (args[i].startsWith("-")) {
                switch(args[i].charAt(1)) {
                    case 'h':
                        if (i < nArgs - 1) {
                            myEnvPath = new String(args[++i]);
                        }
                    break;
                    default:
                        usage();
                }
            }
        }
    }
}

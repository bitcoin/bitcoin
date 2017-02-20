/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005-2009 Oracle.  All rights reserved.
 *
 * $Id$ 
 */

package db.txn;

import com.sleepycat.bind.EntryBinding;
import com.sleepycat.bind.serial.StoredClassCatalog;
import com.sleepycat.bind.serial.SerialBinding;
import com.sleepycat.bind.tuple.StringBinding;

import com.sleepycat.db.Cursor;
import com.sleepycat.db.CursorConfig;
import com.sleepycat.db.Database;
import com.sleepycat.db.DatabaseEntry;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.DeadlockException;
import com.sleepycat.db.Environment;
import com.sleepycat.db.LockMode;
import com.sleepycat.db.OperationStatus;
import com.sleepycat.db.Transaction;

import java.io.UnsupportedEncodingException;
import java.util.Random;

public class DBWriter extends Thread
{
    private Database myDb = null;
    private Environment myEnv = null;
    private EntryBinding dataBinding = null;
    private Random generator = new Random();
    private boolean passTxn = false;


    private static final int MAX_RETRY = 20;

    private static String[] keys = {"key 1", "key 2", "key 3",
                                    "key 4", "key 5", "key 6",
                                    "key 7", "key 8", "key 9",
                                    "key 10"};


    // Constructor. Get our DB handles from here
    // This consturctor allows us to indicate whether the
    // txn handle should be handed to countRecords()
    DBWriter(Environment env, Database db, StoredClassCatalog scc,
        boolean passtxn)

        throws DatabaseException {
        myDb = db;
        myEnv = env;
        dataBinding = new SerialBinding(scc, PayloadData.class);

        passTxn = passtxn;
    }

    // Constructor. Get our DB handles from here
    DBWriter(Environment env, Database db, StoredClassCatalog scc)

        throws DatabaseException {
        myDb = db;
        myEnv = env;
        dataBinding = new SerialBinding(scc, PayloadData.class);
    }

    // Thread method that writes a series of records
    // to the database using transaction protection.
    // Deadlock handling is demonstrated here.
    public void run () {
        Transaction txn = null;

        // Perform 50 transactions
        for (int i=0; i<50; i++) {

           boolean retry = true;
           int retry_count = 0;
           // while loop is used for deadlock retries
           while (retry) {
                // try block used for deadlock detection and
                // general db exception handling
                try {

                    // Get a transaction
                    txn = myEnv.beginTransaction(null, null);

                    // Write 10 records to the db
                    // for each transaction
                    for (int j = 0; j < 10; j++) {
                        // Get the key
                        DatabaseEntry key = new DatabaseEntry();
                        StringBinding.stringToEntry(keys[j], key);

                        // Get the data
                        PayloadData pd = new PayloadData(i+j, getName(),
                            generator.nextDouble());
                        DatabaseEntry data = new DatabaseEntry();
                        dataBinding.objectToEntry(pd, data);

                        // Do the put
                        myDb.put(txn, key, data);
                    }

                    // commit
                    System.out.println(getName() + " : committing txn : " + i);

                    // This code block allows us to decide if txn handle is
                    // passed to countRecords()
                    //
                    // TxnGuideInMemory requires a txn handle be handed to
                    // countRecords(). The code self deadlocks if you don't.
                    // TxnGuide has no such requirement because it supports
                    // uncommitted reads.
                    Transaction txnHandle = null;
                    if (passTxn) { txnHandle = txn; }

                    System.out.println(getName() + " : Found " +
                        countRecords(txnHandle) + " records in the database.");
                    try {
                        txn.commit();
                        txn = null;
                    } catch (DatabaseException e) {
                        System.err.println("Error on txn commit: " +
                            e.toString());
                    }
                    retry = false;

                } catch (DeadlockException de) {
                    System.out.println("################# " + getName() +
                        " : caught deadlock");
                    // retry if necessary
                    if (retry_count < MAX_RETRY) {
                        System.err.println(getName() +
                            " : Retrying operation.");
                        retry = true;
                        retry_count++;
                    } else {
                        System.err.println(getName() +
                            " : out of retries. Giving up.");
                        retry = false;
                    }
                } catch (DatabaseException e) {
                    // abort and don't retry
                    retry = false;
                    System.err.println(getName() +
                        " : caught exception: " + e.toString());
                    System.err.println(getName() +
                        " : errno: " + e.getErrno());
                    e.printStackTrace();
                } finally {
                    if (txn != null) {
                        try {
                            txn.abort();
                        } catch (Exception e) {
                            System.err.println("Error aborting transaction: " +
                                e.toString());
                            e.printStackTrace();
                        }
                    }
                }
            }
        }
    }

    // This simply counts the number of records contained in the
    // database and returns the result. You can use this method
    // in three ways:
    //
    // First call it with an active txn handle.
    //
    // Secondly, configure the cursor for dirty reads
    //
    // Third, call countRecords AFTER the writer has committed
    //    its transaction.
    //
    // If you do none of these things, the writer thread will
    // self-deadlock.
    //
    // Note that this method exists only for illustrative purposes.
    // A more straight-forward way to count the number of records in
    // a database is to use the Database.getStats() method.
    private int countRecords(Transaction txn)  throws DatabaseException {
        DatabaseEntry key = new DatabaseEntry();
        DatabaseEntry data = new DatabaseEntry();
        int count = 0;
        Cursor cursor = null;

        try {
            // Get the cursor
            CursorConfig cc = new CursorConfig();
            // setReadUncommitted is ignored if the database was not
            // opened for uncommitted read support. TxnGuide opens
            // its database in this way, TxnGuideInMemory does not.
            cc.setReadUncommitted(true);
            cursor = myDb.openCursor(txn, cc);
            while (cursor.getNext(key, data, LockMode.DEFAULT) ==
                    OperationStatus.SUCCESS) {

                    count++;
            }
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }

        return count;

    }
}

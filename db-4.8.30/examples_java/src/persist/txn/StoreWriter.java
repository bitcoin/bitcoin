/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2008-2009 Oracle.  All rights reserved.
 *
 * $Id$ 
 */

package persist.txn;

import com.sleepycat.db.CursorConfig;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.DeadlockException;
import com.sleepycat.db.Environment;
import com.sleepycat.db.Transaction;

import com.sleepycat.persist.EntityCursor;
import com.sleepycat.persist.EntityStore;
import com.sleepycat.persist.PrimaryIndex;

import java.util.Iterator;
import java.util.Random;
import java.io.UnsupportedEncodingException;

public class StoreWriter extends Thread 
{
    private EntityStore myStore = null;
    private Environment myEnv = null;
    private PrimaryIndex<Integer,PayloadDataEntity> pdKey;
    private Random generator = new Random();
    private boolean passTxn = false;


    private static final int MAX_RETRY = 20;

    // Constructor. Get our handles from here
    StoreWriter(Environment env, EntityStore store) 

        throws DatabaseException {
        myStore = store;
        myEnv = env;
        
        // Open the data accessor. This is used to store persistent
        // objects.
        pdKey = myStore.getPrimaryIndex(Integer.class, 
                        PayloadDataEntity.class);
    }

    // Thread method that writes a series of objects
    // to the store using transaction protection.
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
                // general exception handling
                try {

                    // Get a transaction
                    txn = myEnv.beginTransaction(null, null);

                    // Write 10 PayloadDataEntity objects to the 
                    // store for each transaction
                    for (int j = 0; j < 10; j++) {
                        // Instantiate an object
                        PayloadDataEntity pd = new PayloadDataEntity();

                        // Set the Object ID. This is used as the primary key.
                        pd.setID(i + j);

                        // The thread name is used as a secondary key, and
                        // it is retrieved by this class's getName() method.
                        pd.setThreadName(getName());
                        
                        // The last bit of data that we use is a double
                        // that we generate randomly. This data is not
                        // indexed.
                        pd.setDoubleData(generator.nextDouble());

                        // Do the put
                        pdKey.put(txn, pd);
                    }

                    // commit
                    System.out.println(getName() + " : committing txn : " + i);
                    System.out.println(getName() + " : Found " +
                        countObjects(txn) + " objects in the store.");
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

    // This simply counts the number of objects contained in the
    // store and returns the result. You can use this method
    // in three ways:
    //
    // First call it with an active txn handle.
    //
    // Secondly, configure the cursor for dirty reads
    //
    // Third, call countObjects AFTER the writer has committed
    //    its transaction.
    //
    // If you do none of these things, the writer thread will 
    // self-deadlock.
    private int countObjects(Transaction txn)  throws DatabaseException {
        int count = 0;

        CursorConfig cc = new CursorConfig();
        // This is ignored if the store is not opened with uncommitted read
        // support.
        cc.setReadUncommitted(true);
        EntityCursor<PayloadDataEntity> cursor = pdKey.entities(txn, cc);

        try {
            for (PayloadDataEntity pdi : cursor) {
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

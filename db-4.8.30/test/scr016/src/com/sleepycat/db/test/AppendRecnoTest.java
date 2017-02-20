/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 */

/*
 * Alexg 23-4-06
 * Based on scr016 TestAppendRecno test application.
 */

package com.sleepycat.db.test;

import org.junit.Before;
import org.junit.After;
import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import java.io.File;
import java.io.FileNotFoundException;
import com.sleepycat.db.*;

import com.sleepycat.db.DatabaseException;

import com.sleepycat.db.test.TestUtils;

public class AppendRecnoTest implements RecordNumberAppender {

    public static final String RECNOTEST_DBNAME  =       "appendrecnotest.db";

    public static final String[] EXPECTED_ENTRIES  =       { "data0_xyz", "data1_xy", "ata4_xyz", "ata5_xy",
                                                                  "abc", "", "data9_xyz" };

    int callback_count = 0;
    boolean callback_throws = false;

    @BeforeClass public static void ClassInit() {
	    TestUtils.loadConfig(null);
        TestUtils.check_file_removed(TestUtils.getDBFileName(RECNOTEST_DBNAME), true, true);
    }

    @AfterClass public static void ClassShutdown() {
        TestUtils.check_file_removed(TestUtils.getDBFileName(RECNOTEST_DBNAME), true, true);
    }

    @Before public void PerTestInit()
        throws Exception {
	    TestUtils.removeall(true, false, TestUtils.BASETEST_DBDIR, TestUtils.getDBFileName(RECNOTEST_DBNAME));
    }

    @After public void PerTestShutdown()
        throws Exception {
    }

    /*
     * Test creating a new database.
     */
    @Test public void test1()
        throws DatabaseException, FileNotFoundException
    {
        DatabaseConfig dbConfig = new DatabaseConfig();
        dbConfig.setErrorStream(TestUtils.getErrorStream());
        dbConfig.setErrorPrefix("AppendRecnoTest");
        dbConfig.setType(DatabaseType.RECNO);
        dbConfig.setPageSize(1024);
        dbConfig.setAllowCreate(true);
        dbConfig.setRecordNumberAppender(this);

        Database db = new Database(TestUtils.getDBFileName(RECNOTEST_DBNAME), null, dbConfig);

        for (int i=0; i<10; i++) {
            TestUtils.DEBUGOUT("\n*** Iteration " + i );
            boolean gotExcept = false;
            try {
                RecnoEntry key = new RecnoEntry(77+i);
                DatabaseEntry data = new DatabaseEntry((new String("data" + i + "_xyz")).getBytes());
                db.append(null, key, data);
            }
            catch (DatabaseException dbe) {
                gotExcept = true;
                // Can be expected since testing throwing from the appendRecordNumber callback.
                TestUtils.DEBUGOUT("dbe: " + dbe);
            } catch (ArrayIndexOutOfBoundsException e) {
                gotExcept = true;
                TestUtils.DEBUGOUT("ArrayIndex: " + e);
            }
            if((gotExcept && callback_throws == false ) || (!gotExcept && callback_throws == true))
                TestUtils.DEBUGOUT(3, "appendRecordNumber callback exception or non-exception condition dealt with incorrectly. Case " + callback_count);
        }

        Cursor dbcp = db.openCursor(null, CursorConfig.DEFAULT);

        // Walk through the table, validating the key/data pairs.
        RecnoEntry readkey = new RecnoEntry();
        DatabaseEntry readdata = new DatabaseEntry();
    	TestUtils.DEBUGOUT("Dbc.get");

    	int itemcount = 0;
    	while (dbcp.getNext(readkey, readdata, LockMode.DEFAULT) == OperationStatus.SUCCESS) {
    	    String gotString = new String(readdata.getData(), readdata.getOffset(), readdata.getSize());
    	    TestUtils.DEBUGOUT(1, readkey.getRecno() + " : " + gotString);

    	    if(readkey.getRecno() != ++itemcount)
    	        TestUtils.DEBUGOUT(3, "Recno iteration out of order. key: " + readkey.getRecno() + " item: " + itemcount);

    	    if(itemcount > EXPECTED_ENTRIES.length)
    	        TestUtils.ERR("More entries in recno DB than expected.");
    	     

    	    if(gotString.compareTo(EXPECTED_ENTRIES[itemcount-1]) != 0)
    	        TestUtils.DEBUGOUT(3, "Recno - stored data mismatch. Expected: " + EXPECTED_ENTRIES[itemcount-1] + " received: " + gotString);
    	}

    	dbcp.close();
    	db.close(false);

    }

    public void appendRecordNumber(Database db, DatabaseEntry data, int recno)
        throws DatabaseException
    {
        callback_throws = false;
        TestUtils.DEBUGOUT("AppendRecnoTest::appendRecordNumber. data " + new String(data.getData()) + " recno: " + recno);

        switch (callback_count++) {
            case 0:
                // nothing
                break;

            case 1:
                data.setSize(data.getSize() - 1);
                break;

            case 2:
                // Should result in an error.
                callback_throws = true;
                TestUtils.DEBUGOUT("throwing...");
                throw new DatabaseException("appendRecordNumber thrown");
                //not reached

            case 3:
                // Should result in an error (size unchanged).
                callback_throws = true;
                data.setOffset(1);
                break;

            case 4:
                data.setOffset(1);
                data.setSize(data.getSize() - 1);
                break;

            case 5:
                data.setOffset(1);
                data.setSize(data.getSize() - 2);
                break;

            case 6:
                data.setData(new String("abc").getBytes());
                data.setSize(3);
                break;

            case 7:
                // Should result in an error.
                callback_throws = true;
                data.setData(new String("abc").getBytes());
                data.setSize(4);
                break;
// TODO: Broken - does not throw an exception.
            case 8:
                // TODO: Should this result in an error?
                data.setData(null);
                break;

            default:
                break;
        }
    }

    static class RecnoEntry extends DatabaseEntry
    {
        public RecnoEntry() {
            super();
        }

        public RecnoEntry(int value)
        {
            setReuseBuffer(false);
            arr = new byte[4];
            setData(arr);                // use our local array for data
            setRecno(value);
        }

        public void setRecno(int value)
        {
            setRecordNumber(value);
         }

        public int getRecno()
        {
            return getRecordNumber();
        }
        byte arr[];
    } // end of RecnoEntry sub-class.

}

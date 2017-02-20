/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 */

/*
 * Alexg 23-4-06
 * Based on scr016 TestAssociate test application.
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

public class AssociateTest {

    public static final String ASSOCTEST_DBNAME  =       "associatetest.db";

    public static final String[] DATABASETEST_SAMPLE_DATA =  {"abc", "def", "ghi", "JKL", "MNO", "pqrst", "UVW", "y", "Z"};

    public static Database savedPriDb = null;
    public static Database savedSecDb = null;

    int callback_count = 0;
    boolean callback_throws = false;
 
    @BeforeClass public static void ClassInit() {
	    TestUtils.loadConfig(null);
        TestUtils.check_file_removed(TestUtils.getDBFileName(ASSOCTEST_DBNAME), true, true);
	    TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR, TestUtils.getDBFileName(ASSOCTEST_DBNAME));
    }

    @AfterClass public static void ClassShutdown() {
        TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR, TestUtils.getDBFileName(ASSOCTEST_DBNAME));
    }

    @Before public void PerTestInit()
        throws Exception {
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
        int i;
        EnvironmentConfig envc = new EnvironmentConfig();
        envc.setAllowCreate(true);
        envc.setInitializeCache(true);
        Environment dbEnv = new Environment(TestUtils.BASETEST_DBFILE, envc);

        DatabaseConfig dbConfig = new DatabaseConfig();
        dbConfig.setErrorStream(TestUtils.getErrorStream());
        dbConfig.setErrorPrefix("AssociateTest");
        dbConfig.setType(DatabaseType.BTREE);
        dbConfig.setAllowCreate(true);

        Database priDb = dbEnv.openDatabase(null, ASSOCTEST_DBNAME, null, dbConfig);

        SecondaryConfig secConfig = new SecondaryConfig();
        secConfig.setAllowCreate(true);
        secConfig.setType(DatabaseType.BTREE);
        secConfig.setSortedDuplicates(true);
        secConfig.setKeyCreator(new Capitalize());
        SecondaryDatabase secDb = dbEnv.openSecondaryDatabase(null, ASSOCTEST_DBNAME+"2", null,
                                                  priDb, secConfig);
        savedPriDb = priDb;
        savedSecDb = secDb;

        // Insert records into the database
        for(i =0; i < DATABASETEST_SAMPLE_DATA.length; i++)
        {
            String curdata = DATABASETEST_SAMPLE_DATA[i];
            String reversed = (new StringBuffer(curdata)).reverse().toString();

            DatabaseEntry key = new DatabaseEntry(curdata.getBytes());
            key.setReuseBuffer(false);
            DatabaseEntry data = new DatabaseEntry(reversed.getBytes());
            data.setReuseBuffer(false);
            try {
                if (priDb.putNoOverwrite(null, key, data) == OperationStatus.KEYEXIST) {
                    // should not in this - since we control the data.
                    TestUtils.DEBUGOUT(2, "Key: " + curdata + " already exists\n");
                }
            } catch(DatabaseException dbe) {
                TestUtils.ERR("Caught DatabaseException: " + dbe);
            }
        }

        DatabaseEntry readkey = new DatabaseEntry();
        readkey.setReuseBuffer(false);
        DatabaseEntry readdata = new DatabaseEntry();
        readdata.setReuseBuffer(false);
        Cursor dbcp = priDb.openCursor(null, CursorConfig.DEFAULT);
    	while (dbcp.getNext(readkey, readdata, LockMode.DEFAULT) == OperationStatus.SUCCESS) {
    	    String keystring = new String(readkey.getData());
    	    String datastring = new String(readdata.getData());
    	    String expecteddata = (new StringBuffer(keystring)).reverse().toString();

    	    boolean found = false;
            for(i = 0; i < DATABASETEST_SAMPLE_DATA.length; i++)
            {
                if(DATABASETEST_SAMPLE_DATA[i].compareTo(keystring) == 0)
                    found = true;
            }
            if(!found)
                TestUtils.ERR("Key: " + keystring + " retrieved from DB, but never added!");
            if(datastring.compareTo(expecteddata) != 0)
                TestUtils.ERR("Data: " + datastring + " does not match expected data: " + expecteddata);
    	}

        // Test secondary get functionality.
        DatabaseEntry seckey = new DatabaseEntry();
        seckey.setReuseBuffer(false);
        DatabaseEntry secpkey = new DatabaseEntry();
        secpkey.setReuseBuffer(false);
        DatabaseEntry secdata = new DatabaseEntry();
        secdata.setReuseBuffer(false);

        seckey.setData("BC".getBytes());
        if(secDb.get(null, seckey, secdata, null) == OperationStatus.SUCCESS)
        {
            TestUtils.DEBUGOUT(2, "seckey: " + new String(seckey.getData()) + " secdata: " +
                new String(secdata.getData()));
        } else {
            TestUtils.ERR("Secondary get of key: " + new String(seckey.getData()) + " did not succeed.");
        }
        // pget
        if(secDb.get(null, seckey, secpkey, secdata, null) == OperationStatus.SUCCESS)
        {
            TestUtils.DEBUGOUT(2, "seckey: " + new String(seckey.getData()) + " secdata: " +
                new String(secdata.getData()) + " pkey: " + new String(secpkey.getData()));

            // ensure that the retrievals are consistent using both primary and secondary keys.
            DatabaseEntry tmpdata = new DatabaseEntry();
            priDb.get(null, secpkey, tmpdata, null);
            String tmpstr = new String(tmpdata.getData());
            if(tmpstr.compareTo(new String(secdata.getData())) != 0)
                TestUtils.ERR("Data retrieved from matching primary secondary keys is not consistent. secdata: " + new String(secdata.getData()) +
                    " pridata: " + new String(tmpdata.getData()));
        } else {
            TestUtils.ERR("Secondary pget of key: " + new String(seckey.getData()) + " did not succeed.");
        }
        seckey.setData("KL".getBytes());
        if(secDb.get(null, seckey, secdata, null) == OperationStatus.SUCCESS)
        {
            TestUtils.DEBUGOUT(2, "seckey: " + new String(seckey.getData()) + " secdata: " +
                new String(secdata.getData()));
        } else {
            TestUtils.ERR("Secondary get of key: " + new String(seckey.getData()) + " did not succeed.");
        }
        // pget
        if(secDb.get(null, seckey, secpkey, secdata, null) == OperationStatus.SUCCESS)
        {
            TestUtils.DEBUGOUT(2, "seckey: " + new String(seckey.getData()) + " secdata: " +
                new String(secdata.getData()) + " pkey: " + new String(secpkey.getData()));

            // ensure that the retrievals are consistent using both primary and secondary keys.
            DatabaseEntry tmpdata = new DatabaseEntry();
            priDb.get(null, secpkey, tmpdata, null);
            String tmpstr = new String(tmpdata.getData());
            if(tmpstr.compareTo(new String(secdata.getData())) != 0)
                TestUtils.ERR("Data retrieved from matching primary secondary keys is not consistent. secdata: " + new String(secdata.getData()) +
                    " pridata: " + new String(tmpdata.getData()));
        } else {
            TestUtils.ERR("Secondary pget of key: " + new String(seckey.getData()) + " did not succeed.");
        }

    }

/**************************************************************************************
 **************************************************************************************
 **************************************************************************************/
    /* creates a stupid secondary index as follows:
     For an N letter key, we use N-1 letters starting at
     position 1.  If the new letters are already capitalized,
     we return the old array, but with offset set to 1.
     If the letters are not capitalized, we create a new,
     capitalized array.  This is pretty stupid for
     an application, but it tests all the paths in the runtime.
     */
    public static class Capitalize implements SecondaryKeyCreator
    {
        public boolean createSecondaryKey(SecondaryDatabase secondary,
                                      DatabaseEntry key,
                                      DatabaseEntry data,
                                      DatabaseEntry result)
                throws DatabaseException
        {
            key.setReuseBuffer(false);
            data.setReuseBuffer(false);
            result.setReuseBuffer(false);
            String which = "unknown db";
            if (savedPriDb.equals(secondary)) {
                which = "primary";
            }
            else if (savedSecDb.equals(secondary)) {
                which = "secondary";
            }
    	    String keystring = new String(key.getData());
    	    String datastring = new String(data.getData());
            TestUtils.DEBUGOUT(2, "secondaryKeyCreate, Db: " + TestUtils.shownull(secondary) + "(" + which + "), key: " + keystring + ", data: " + datastring);
            int len = key.getSize();
            byte[] arr = key.getData();
            boolean capped = true;

            if (len < 1)
                throw new DatabaseException("bad key");

            result.setSize(len - 1);
            for (int i=1; capped && i<len; i++) {
                if (!Character.isUpperCase((char)arr[i]))
                    capped = false;
            }
            if (capped) {
                TestUtils.DEBUGOUT(2, "  creating key(1): " + new String(arr, 1, len-1));
                result.setData(arr);
                result.setOffset(1);
                result.setSize(result.getSize() -1);
            }
            else {
                TestUtils.DEBUGOUT(2, "  creating key(2): " + (new String(arr)).substring(1).
                                   toUpperCase());
                result.setData((new String(arr)).substring(1).
                                toUpperCase().getBytes());
            }
            return true;
        }
    }

}

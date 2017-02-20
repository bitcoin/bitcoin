/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 */

package com.sleepycat.db.test;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Ignore;
import org.junit.Test;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import com.sleepycat.db.*;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;

import com.sleepycat.db.test.TestUtils;
public class PartialGetTest {
    public static final String PARTIALGETTEST_DBNAME = "partialgettest.db";
    public static final byte[] data_64chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890<>".getBytes();
    @BeforeClass public static void ClassInit() {
        TestUtils.loadConfig(null);
        TestUtils.check_file_removed(TestUtils.getDBFileName(PARTIALGETTEST_DBNAME), true, true);
        TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR, TestUtils.getDBFileName(PARTIALGETTEST_DBNAME));
    }

    @AfterClass public static void ClassShutdown() {
        TestUtils.check_file_removed(TestUtils.getDBFileName(PARTIALGETTEST_DBNAME), true, true);
        TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR, TestUtils.getDBFileName(PARTIALGETTEST_DBNAME));
    }

    @Before public void PerTestInit()
        throws Exception {
    }

    @After public void PerTestShutdown()
        throws Exception {
        TestUtils.check_file_removed(TestUtils.getDBFileName(PARTIALGETTEST_DBNAME), true, true);
    }
    /*
     * Test case implementations.
     * To disable a test mark it with @Ignore
     * To set a timeout(ms) notate like: @Test(timeout=1000)
     * To indicate an expected exception notate like: (expected=Exception)
     */

    /*
     * Simple partial gets on a record which is on a single page.
     */
    @Test public void test1()
        throws DatabaseException, FileNotFoundException
    {
        DatabaseEntry key = new DatabaseEntry("key".getBytes());
        Database db = setupDb1(key, data_64chars);
     
        StringEntry partialData = new StringEntry();
        partialData.setPartial(true);
        partialData.setPartial(0, 12, true);
     
        if (db.get(null, key, partialData, LockMode.DEFAULT) !=
            OperationStatus.SUCCESS)
            fail("Failed doing partial retrieval, first part of entry on single page.");
        // Validate the data.
        if (!MatchData(data_64chars, partialData.getString(), 12))
            fail("Data mismatch from partial get.");
     
        partialData.setPartial(12, 12, true);
        if (db.get(null, key, partialData, LockMode.DEFAULT) !=
            OperationStatus.SUCCESS)
            fail("Failed doing partial retrieval, second part of entry on single page.");
        // Validate the data.
        if (!MatchData(new String(data_64chars, 12, 12), partialData.getString(), 12))
            fail("Data mismatch from partial get.");
         
        db.close(false);
    }
 
    /*
     * Retrieve entry using different DB_DBT_alloc flags.
     * Verify results.
     */
    @Test public void test2()
        throws DatabaseException, FileNotFoundException
    {
        DatabaseEntry key = new DatabaseEntry("key".getBytes());
        Database db = setupDb1(key, data_64chars);
        StringEntry partialData = new StringEntry();
        partialData.setPartial(true);
        partialData.setPartial(0, 12, true);
     
        if (db.get(null, key, partialData, LockMode.DEFAULT) !=
            OperationStatus.SUCCESS)
            fail("Failed doing partial retrieval.");
        // Validate the data.
        if (!MatchData(data_64chars, partialData.getString(), 12))
            fail("Data mismatch from partial get.");
     
        partialData.setReuseBuffer(true);
        if (db.get(null, key, partialData, LockMode.DEFAULT) !=
            OperationStatus.SUCCESS)
        if (!MatchData(data_64chars, partialData.getString(), 12))
            fail("Data mismatch from partial get.");
     
        partialData.setReuseBuffer(false);
        partialData.setUserBuffer(64, true);
        partialData.setData(new byte[64]);
        if (db.get(null, key, partialData, LockMode.DEFAULT) !=
            OperationStatus.SUCCESS)
        if (!MatchData(data_64chars, partialData.getString(), 12))
            fail("Data mismatch from partial get.");
     
        partialData.setPartial(12, 12, true);
        if (db.get(null, key, partialData, LockMode.DEFAULT) !=
            OperationStatus.SUCCESS)
            fail("Failed doing partial retrieval.");
        // Validate the data.
        if (!MatchData(new String(data_64chars, 12, 12), partialData.getString(), 12))
            fail("Data mismatch from partial get.");

        db.close(false);
    }
 
    /* Retrieve entry that spans multiple pages. */
 
    @Test public void test3()
        throws DatabaseException, FileNotFoundException
    {
        DatabaseEntry key = new DatabaseEntry("key".getBytes());
        StringBuffer sb = new StringBuffer(1024*100);
        for(int i = 0; i < 1024; i++) {
            sb.append("abcdefghijklmnopqrstuvwxyz1234567890!@#$%^&*()_+-=");
            sb.append("abcdefghijklmnopqrstuvwxyz1234567890!@#$%^&*()_+-=");
        }
        Database db = setupDb1(key, sb.toString().getBytes());
     
        StringEntry partialData = new StringEntry();
        partialData.setPartial(true);
        partialData.setPartial(0, 12, true);
     
        if (db.get(null, key, partialData, LockMode.DEFAULT) !=
            OperationStatus.SUCCESS)
            fail("Failed doing partial retrieval.");
        // Validate the data.
        if (!MatchData(data_64chars, partialData.getString(), 12))
            fail("Data mismatch from partial get.");

        // retrieve a chunk larger than a page size, starting at offset 0.
        partialData.setPartial(0, 2048, true);
        if (db.get(null, key, partialData, LockMode.DEFAULT) !=
            OperationStatus.SUCCESS)
            fail("Failed doing partial retrieval.");
        // Validate the data.
        if (!MatchData(sb.substring(0, 2048), partialData.getString(), 2048))
            fail("Data mismatch from partial get.");

        // retrieve a chunk larger than a page size, starting at offset greater than 0.
        partialData.setPartial(10, 2048, true);
        if (db.get(null, key, partialData, LockMode.DEFAULT) !=
            OperationStatus.SUCCESS)
            fail("Failed doing partial retrieval.");
        // Validate the data.
        if (!MatchData(sb.substring(10, 2048+10), partialData.getString(), 12))
            fail("Data mismatch from partial get.");

        db.close(false);
    }
 
    /*
     * Test partial retrieval using a cursor.
     */
    @Test public void test4()
        throws DatabaseException, FileNotFoundException
    {
    }
 
    /*
     * Test partial retrieval using different DB types.
     */
    @Test public void test5()
        throws DatabaseException, FileNotFoundException
    {
    }
 
    /*
     * Test partial retrieval .
     */
    @Test public void test6()
        throws DatabaseException, FileNotFoundException
    {
    }

    /*
     * Helper methods and classes follow.
     */
 
    private Database setupDb1(DatabaseEntry key, byte[] dataData)
        throws DatabaseException, FileNotFoundException
    {
        DatabaseConfig dbConfig = new DatabaseConfig();
        dbConfig.setErrorStream(TestUtils.getErrorStream());
        dbConfig.setErrorPrefix("PartialGetTest");
        dbConfig.setType(DatabaseType.BTREE);
        dbConfig.setPageSize(1024);
        dbConfig.setAllowCreate(true);
        Database db = new Database(TestUtils.getDBFileName(PARTIALGETTEST_DBNAME), null, dbConfig);
     
        DatabaseEntry data = new DatabaseEntry(dataData);
     
        if(db.putNoOverwrite(null, key, data) != OperationStatus.SUCCESS)
            TestUtils.ERR("Failed to create standard entry in database.");
         
        return db;
    }
 
    /* Save converting String to do data comparisons. */
    private boolean MatchData(byte[] data1, byte[] data2, int len)
    {
        return MatchData(new String(data1), new String(data2), len);
    }
    private boolean MatchData(String data1, byte[] data2, int len)
    {
        return MatchData(data1, new String(data2), len);
    }
    private boolean MatchData(byte[] data1, String data2, int len)
    {
        return MatchData(new String(data1), data2, len);
    }
    private boolean MatchData(String data1, String data2, int len)
    {
        if(data1.length() < len || data2.length() < len)
            return false;
        TestUtils.DEBUGOUT(0, "data1: " +data1.substring(0, 12));
        TestUtils.DEBUGOUT(0, "data2: " +data2.substring(0, 12));
        return data1.regionMatches(0, data2, 0, len);
    }
 
    static /*inner*/
    class StringEntry extends DatabaseEntry {
        StringEntry() {
        }
     
        StringEntry (String value) {
            setString(value);
        }
     
        void setString(String value) {
            byte[] data = value.getBytes();
            setData(data);
            setSize(data.length);
        }
     
        String getString() {
            return new String(getData(), getOffset(), getSize());
        }
    }
}

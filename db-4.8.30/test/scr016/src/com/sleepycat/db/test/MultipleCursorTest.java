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
import org.junit.Test;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import com.sleepycat.db.*;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;

import com.sleepycat.db.test.TestUtils;

public class MultipleCursorTest {
    public static final String MULTIPLECURSORTEST_DBNAME = "multiplecursortest.db";

    /* The data used by this test. */
    private static final String[] Key_Strings = {
    "abc",
    "def",
    "ghi",
    "jkl",
    "mno",
    "pqr",
    "stu",
    "vwx",
    "yza",
    "bcd",
    "efg",
    "hij",
    "klm",
    "nop",
    "qrs",
    "tuv",
    "wxy",
    };
    private static boolean verbose = false;
    @BeforeClass public static void ClassInit() {
        TestUtils.loadConfig(null);
        TestUtils.check_file_removed(TestUtils.getDBFileName(MULTIPLECURSORTEST_DBNAME), true, true);
        TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR, TestUtils.getDBFileName(MULTIPLECURSORTEST_DBNAME));
    }

    @AfterClass public static void ClassShutdown() {
        TestUtils.check_file_removed(TestUtils.getDBFileName(MULTIPLECURSORTEST_DBNAME), true, true);
        TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR, TestUtils.getDBFileName(MULTIPLECURSORTEST_DBNAME));
    }

    @Before public void PerTestInit()
        throws Exception {
    }

    @After public void PerTestShutdown()
        throws Exception {
    }
    public static void main(String []argv) {
        verbose = true;
        if (argv.length > 0 && argv[0].equals("-s")) {
	    try {
	    java.lang.Thread.sleep(15*1000);
	    } catch (InterruptedException e) {
	    }
	}
        try {
            MultipleCursorTest mpt = new MultipleCursorTest();
            mpt.testMultiplePut();
            mpt.testMultipleDelete();
	} catch (DatabaseException dbe) {
	    System.out.println("MultipleCursorTest threw DatabaseException");
	} catch (FileNotFoundException fnfe) {
	    System.out.println("MultipleCursorTest threw FileNotFound");
	}
    }
	/*
     * Test case implementations.
     * To disable a test mark it with @Ignore
     * To set a timeout(ms) notate like: @Test(timeout=1000)
     * To indicate an expected exception notate like: (expected=Exception)
     */

    @Test public void testMultiplePut()
        throws DatabaseException, FileNotFoundException
    {
        Database db = createDatabase();
	byte [] buffer = new byte[1024];
	byte [] buffer2 = new byte[1024];
	int i;

	/* Build up a bulk key/data pair. */
	MultipleKeyDataEntry kd = new MultipleKeyDataEntry(buffer);
	DatabaseEntry key = new DatabaseEntry();
	DatabaseEntry data = new DatabaseEntry();
	/* Put 3 in the first round. */
	for (i = 0; i < 3; i++) {
            key.setData(Key_Strings[i].getBytes());
	    data.setData(Key_Strings[i].getBytes());
	    kd.append(key, data);
        }
	if (verbose)
	    System.out.println("Built up a multi-key/data buffer.");
	db.putMultipleKey(null, kd, false);
	if (verbose)
	    System.out.println("Put a multi-key/data buffer.");

	/* Build up separate bulk key/data DatabaseEntries */
	MultipleDataEntry keys = new MultipleDataEntry(buffer);
	MultipleDataEntry datas = new MultipleDataEntry(buffer2);
	/* Put 3 in the second round. */
	for (; i < 6; i++) {
            key.setData(Key_Strings[i].getBytes());
	    data.setData(Key_Strings[i].getBytes());
	    keys.append(key);
	    datas.append(data);
        }
	if (verbose)
	    System.out.println("Built up multi-key and data buffers.");
	db.putMultiple(null, keys, datas, false);
	if (verbose)
	    System.out.println("Put multi-key and data buffers.");

	// Bulk cursor, adding single items.
	Cursor dbc = db.openCursor(null, CursorConfig.BULK_CURSOR);
	for (; i < 12; i++) {
            key.setData(Key_Strings[i].getBytes());
	    data.setData(Key_Strings[i].getBytes());
	    dbc.put(key, data);
        }
	dbc.close();

	if (verbose)
	    dumpDatabase(db);
    }
    @Test public void testMultipleDelete()
        throws DatabaseException, FileNotFoundException
    {
	byte [] buffer = new byte[1024];
	int i;
        Database db = createDatabase();
	populateDatabase(db, 0);

	/* Build up separate bulk key/data DatabaseEntries */
	MultipleDataEntry keys = new MultipleDataEntry(buffer);
	DatabaseEntry key = new DatabaseEntry();
	/* Put 3 in the second round. */
	for (i = 0; i < 6; i++) {
            key.setData(Key_Strings[i].getBytes());
	    keys.append(key);
        }
	db.deleteMultiple(null, keys);
	// Bulk cursor, adding single items.
	DatabaseEntry data = new DatabaseEntry();
	Cursor dbc = db.openCursor(null, CursorConfig.BULK_CURSOR);
	for (; i < 12; i++) {
            key.setData(Key_Strings[i].getBytes());
	    dbc.getSearchKey(key, data, LockMode.DEFAULT);
	    dbc.delete();
        }
	dbc.close();

	// Should have about 3 entries left.
	if (verbose)
	    dumpDatabase(db);
    }
    	
    /* Not implemented yet.
    @Test public void testMultipleGet()
        throws DatabaseException, FileNotFoundException
    {
        Database db = createDatabase();
	populateDatabase(db, 0);
    }
    */

    private Database createDatabase()
        throws DatabaseException, FileNotFoundException
    {
        /* Create database. */
        Database db;
	DatabaseConfig db_config = new DatabaseConfig();
        String name = TestUtils.getDBFileName(MULTIPLECURSORTEST_DBNAME);

        db_config.setAllowCreate(true);
	db_config.setType(DatabaseType.BTREE);
	db_config.setSortedDuplicates(true);

        db = new Database(name, null, db_config);
	return db;
    }
        
    private void populateDatabase(Database db, int duplicates)
        throws DatabaseException, FileNotFoundException
    {
        DatabaseEntry key = new DatabaseEntry();
	DatabaseEntry data = new DatabaseEntry();
	for (int i = 0; i < Key_Strings.length; i++) {
	    String datastr = new Integer(i).toString() + Key_Strings[i] + Key_Strings[i];
	    key.setData(Key_Strings[i].getBytes());
	    data.setData(datastr.getBytes());
	    db.put(null, key, data);
	    for (int j = 0; j < duplicates; j++) {
	        datastr = new Integer(j).toString() + datastr + Key_Strings[i];
	        data.setData(datastr.getBytes());
		db.put(null, key, data);

	    }
	}
    }

    private void dumpDatabase(Database db) {
        try {
	    Cursor dbc = db.openCursor(null, CursorConfig.DEFAULT);
	    DatabaseEntry key = new DatabaseEntry();
	    DatabaseEntry data = new DatabaseEntry();

	    System.out.println("Dumping database contents:");
	    while (dbc.getNext(key, data, LockMode.DEFAULT) != OperationStatus.NOTFOUND) {
		System.out.println("\tGot key : " + new String(key.getData()));
		System.out.println("\t    data: " + new String(data.getData()));
	    }
	    System.out.println("Finished dumping database contents.");
        } catch (DatabaseException dbe) {
	    System.err.println("dumpDatabase caught an exception.");
	}
    }

}

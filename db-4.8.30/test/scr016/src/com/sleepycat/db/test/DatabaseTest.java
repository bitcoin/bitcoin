/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 */

/*
 * Alexg 23-4-06
 * Based on scr016 TestConstruct01 and TestConstruct02
 * test applications.
 */

package com.sleepycat.db.test;

import org.junit.Before;
import org.junit.After;
import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import com.sleepycat.db.Cursor;
import com.sleepycat.db.CursorConfig;
import com.sleepycat.db.Database;
import com.sleepycat.db.DatabaseConfig;
import com.sleepycat.db.DatabaseEntry;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.DatabaseType;
import com.sleepycat.db.Environment;
import com.sleepycat.db.EnvironmentConfig;
import com.sleepycat.db.LockMode;
import com.sleepycat.db.OperationStatus;

import java.io.File;
import java.io.IOException;
import java.io.FileNotFoundException;
import com.sleepycat.db.test.TestUtils;

public class DatabaseTest {

    public static final String DATABASETEST_DBNAME  =       "databasetest.db";

    private static int itemcount;	// count the number of items in the database
 
    @BeforeClass public static void ClassInit() {
	    TestUtils.loadConfig(null);
        TestUtils.check_file_removed(TestUtils.getDBFileName(DATABASETEST_DBNAME), true, true);
	    TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR, TestUtils.getDBFileName(DATABASETEST_DBNAME));
        itemcount = 0;
    }

    @AfterClass public static void ClassShutdown() {
        TestUtils.check_file_removed(TestUtils.getDBFileName(DATABASETEST_DBNAME), true, true);
	    TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR, TestUtils.getDBFileName(DATABASETEST_DBNAME));
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
        TestOptions options = new TestOptions();
        options.db_config.setErrorPrefix("DatabaseTest::test1 ");

        rundb(itemcount++, options);
    }

    /*
     * Test opening and adding to an existing database.
     */
    @Test public void test2()
        throws DatabaseException, FileNotFoundException
    {
        TestOptions options = new TestOptions();
        options.db_config.setErrorPrefix("DatabaseTest::test2 ");

        rundb(itemcount++, options);
    }

    /*
     * Test modifying the error prefix multiple times ?
     */
    @Test public void test3()
        throws DatabaseException, FileNotFoundException
    {
        TestOptions options = new TestOptions();
        options.db_config.setErrorPrefix("DatabaseTest::test3 ");

        for (int i=0; i<100; i++)
            options.db_config.setErrorPrefix("str" + i);

        rundb(itemcount++, options);
    }

    /*
     * Test opening a database with an env.
     */
    @Test public void test4()
        throws DatabaseException, FileNotFoundException
    {
        TestOptions options = new TestOptions();
        options.db_config.setErrorPrefix("DatabaseTest::test4 ");

        EnvironmentConfig envc = new EnvironmentConfig();
        envc.setAllowCreate(true);
        envc.setInitializeCache(true);
    	options.db_env = new Environment(TestUtils.BASETEST_DBFILE, envc);

        rundb(itemcount++, options);
        options.db_env.close();
    }

    /*
     * Test opening multiple databases using the same env.
     */
    @Test public void test5()
        throws DatabaseException, FileNotFoundException
    {
        TestOptions options = new TestOptions();
        options.db_config.setErrorPrefix("DatabaseTest::test5 ");

        EnvironmentConfig envc = new EnvironmentConfig();
        envc.setAllowCreate(true);
        envc.setInitializeCache(true);
    	options.db_env = new Environment(TestUtils.BASETEST_DBFILE, envc);

        rundb(itemcount++, options);

        rundb(itemcount++, options);

        options.db_env.close();
    }

    /*
     * Test just opening and closing a DB and an Env without doing any operations.
     */
    @Test public void test6()
        throws DatabaseException, FileNotFoundException
    {
        TestOptions options = new TestOptions();
        options.db_config.setErrorPrefix("DatabaseTest::test6 ");

        Database db = new Database(TestUtils.getDBFileName(DATABASETEST_DBNAME), null, options.db_config);

        EnvironmentConfig envc = new EnvironmentConfig();
        envc.setAllowCreate(true);
        envc.setInitializeCache(true);
    	Environment db_env = new Environment(TestUtils.BASETEST_DBFILE, envc);

    	db.close();
    	db_env.close();

    	System.gc();
    	System.runFinalization();
    }

    /*
     * test7 leaves a db and dbenv open; it should be detected.
     */
    /* Not sure if relevant with current API.
    @Test public void test7()
        throws DatabaseException, FileNotFoundException
    {
        TestOptions options = new TestOptions();
        options.db_config.setErrorPrefix("DatabaseTest::test7 ");

        Database db = new Database(TestUtils.getDBFileName(DATABASETEST_DBNAME), null, options.db_config);

        EnvironmentConfig envc = new EnvironmentConfig();
        envc.setAllowCreate(true);
        envc.setInitializeCache(true);
    	Environment db_env = new Environment(TestUtils.BASETEST_DBFILE, envc);

	    System.gc();
	    System.runFinalization();
    }
    */

    /*
     * Test creating a new database.
     */
    @Test public void test8()
        throws DatabaseException, FileNotFoundException
    {
	    TestUtils.removeall(true, false, TestUtils.BASETEST_DBDIR, TestUtils.getDBFileName(DATABASETEST_DBNAME));
	    itemcount = 0;
        TestOptions options = new TestOptions();
        options.save_db = true;
        options.db_config.setErrorPrefix("DatabaseTest::test8 ");

        EnvironmentConfig envc = new EnvironmentConfig();
        envc.setAllowCreate(true);
        envc.setInitializeCache(true);
    	options.db_env = new Environment(TestUtils.BASETEST_DBFILE, envc);

        // stop rundb from closing the database, and pass in one created.
    	rundb(itemcount++, options);
    	rundb(itemcount++, options);
    	rundb(itemcount++, options);
    	rundb(itemcount++, options);
    	rundb(itemcount++, options);
    	rundb(itemcount++, options);

    	options.database.close();
    	options.database = null;

    	// reopen the same database.
    	rundb(itemcount++, options);
    	rundb(itemcount++, options);
    	rundb(itemcount++, options);
    	rundb(itemcount++, options);
    	rundb(itemcount++, options);
    	rundb(itemcount++, options);

        options.database.close();
    	options.database = null;

    }

    // Check that key/data for 0 - count-1 are already present,
    // and write a key/data for count.  The key and data are
    // both "0123...N" where N == count-1.
    //
    // For some reason on Windows, we need to open using the full pathname
    // of the file when there is no environment, thus the 'has_env'
    // variable.
    //
    void rundb(int count, TestOptions options)
            throws DatabaseException, FileNotFoundException
    {
    	String name;

        Database db;
        if(options.database == null)
        {
        	if (options.db_env != null)
        	    name = DATABASETEST_DBNAME;
        	else
        	    name = TestUtils.getDBFileName(DATABASETEST_DBNAME);

            if(count == 0)
                options.db_config.setAllowCreate(true);

            if(options.db_env == null)
                db = new Database(name, null, options.db_config);
            else
                db = options.db_env.openDatabase(null, name, null, options.db_config);
        } else {
            db = options.database;
        }

    	// The bit map of keys we've seen
    	long bitmap = 0;

    	// The bit map of keys we expect to see
    	long expected = (1 << (count+1)) - 1;

    	byte outbuf[] = new byte[count+1];
    	int i;
    	for (i=0; i<count; i++) {
    		outbuf[i] = (byte)('0' + i);
    	}
    	outbuf[i++] = (byte)'x';

    	DatabaseEntry key = new DatabaseEntry(outbuf, 0, i);
    	DatabaseEntry data = new DatabaseEntry(outbuf, 0, i);

    	TestUtils.DEBUGOUT("Put: " + (char)outbuf[0] + ": " + new String(outbuf, 0, i));
    	db.putNoOverwrite(null, key, data);

    	// Acquire a cursor for the table.
    	Cursor dbcp = db.openCursor(null, CursorConfig.DEFAULT);

    	// Walk through the table, checking
    	DatabaseEntry readkey = new DatabaseEntry();
    	DatabaseEntry readdata = new DatabaseEntry();
    	DatabaseEntry whoknows = new DatabaseEntry();

    	/*
    	 * NOTE: Maybe want to change from user-buffer to DB buffer
    	 *       depending on the flag options.user_buffer (setReuseBuffer)
    	 * The old version set MALLOC/REALLOC here - not sure if it is the same.
    	 */

    	TestUtils.DEBUGOUT("Dbc.get");
    	while (dbcp.getNext(readkey, readdata, LockMode.DEFAULT) == OperationStatus.SUCCESS) {
    		String key_string =
    		new String(readkey.getData(), 0, readkey.getSize());
    		String data_string =
    		new String(readdata.getData(), 0, readkey.getSize());
    		TestUtils.DEBUGOUT("Got: " + key_string + ": " + data_string);
    		int len = key_string.length();
    		if (len <= 0 || key_string.charAt(len-1) != 'x') {
    			TestUtils.ERR("reread terminator is bad");
    		}
    		len--;
    		long bit = (1 << len);
    		if (len > count) {
    			TestUtils.ERR("reread length is bad: expect " + count + " got "+ len + " (" + key_string + ")" );
    		}
    		else if (!data_string.equals(key_string)) {
    			TestUtils.ERR("key/data don't match");
    		}
    		else if ((bitmap & bit) != 0) {
    			TestUtils.ERR("key already seen");
    		}
    		else if ((expected & bit) == 0) {
    			TestUtils.ERR("key was not expected");
    		}
    		else {
    			bitmap |= bit;
    			expected &= ~(bit);
    			for (i=0; i<len; i++) {
    				if (key_string.charAt(i) != ('0' + i)) {
    					System.out.print(" got " + key_string
    					+ " (" + (int)key_string.charAt(i)
    					+ "), wanted " + i
    					+ " (" + (int)('0' + i)
    					+ ") at position " + i + "\n");
    					TestUtils.ERR("key is corrupt");
    				}
    			}
    		}
    	}
    	if (expected != 0) {
    		System.out.print(" expected more keys, bitmap is: " + expected + "\n");
    		TestUtils.ERR("missing keys in database");
    	}

    	dbcp.close();
    	TestUtils.DEBUGOUT("options.save_db " + options.save_db + " options.database " + options.database);
    	if(options.save_db == false)
    	    db.close(false);
    	else if (options.database == null)
    	    options.database = db;
    }
}


class TestOptions
{
    int testmask = 0;           // which tests to run
    int user_buffer = 0;    // use DB_DBT_USERMEM or DB_DBT_MALLOC
    int successcounter =0;
    boolean save_db = false;
    Environment db_env = null;
    DatabaseConfig db_config;
    Database database = null; // db is saved here by rundb if save_db is true.

    public TestOptions()
    {
        this.testmask = 0;
        this.user_buffer = 0;
        this.successcounter = 0;
        this.db_env = null;

        db_config = new DatabaseConfig();
        db_config.setErrorStream(TestUtils.getErrorStream());
        db_config.setErrorPrefix("DatabaseTest");
        db_config.setType(DatabaseType.BTREE);
    	// We don't really care about the pagesize
        db_config.setPageSize(1024);

    }

}

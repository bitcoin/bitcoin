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
public class HashCompareTest {
    public static final String HASHCOMPARETEST_DBNAME = "hashcomparetest.db";
    @BeforeClass public static void ClassInit() {
        TestUtils.loadConfig(null);
        TestUtils.check_file_removed(TestUtils.getDBFileName(HASHCOMPARETEST_DBNAME), true, true);
        TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR, TestUtils.getDBFileName(HASHCOMPARETEST_DBNAME));
    }

    @AfterClass public static void ClassShutdown() {
        TestUtils.check_file_removed(TestUtils.getDBFileName(HASHCOMPARETEST_DBNAME), true, true);
        TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR, TestUtils.getDBFileName(HASHCOMPARETEST_DBNAME));
    }

    @Before public void PerTestInit()
        throws Exception {
        TestUtils.check_file_removed(TestUtils.getDBFileName(HASHCOMPARETEST_DBNAME), true, true);
		java.io.File dbfile = new File(HASHCOMPARETEST_DBNAME);
		dbfile.delete();
    }

    @After public void PerTestShutdown()
        throws Exception {
    }
    /*
     * Test case implementations.
     * To disable a test mark it with @Ignore
     * To set a timeout(ms) notate like: @Test(timeout=1000)
     * To indicate an expected exception notate like: (expected=Exception)
     */

    @Test public void test1()
        throws DatabaseException, FileNotFoundException
    {
		runTest(DatabaseType.HASH);
	}

    @Test public void test2()
        throws DatabaseException, FileNotFoundException
    {
		runTest(DatabaseType.BTREE);
	}

	public void runTest(DatabaseType type)
	    throws DatabaseException, FileNotFoundException
	{
		int i;
		DatabaseConfig conf = new DatabaseConfig();
        conf.setErrorStream(TestUtils.getErrorStream());
        conf.setErrorPrefix("HashCompareTest");
        conf.setType(type);
		if (type == DatabaseType.HASH) {
			conf.setHashComparator(new HashComparator());
		} else
			conf.setBtreeComparator(new BtreeComparator());
        conf.setAllowCreate(true);

        Database db = new Database(HASHCOMPARETEST_DBNAME, null, conf);

		DatabaseEntry key = new DatabaseEntry();
		DatabaseEntry data = new DatabaseEntry("world".getBytes());
		for (i = 0; i < 100; i++) {
			key.setData((new String("key"+i)).getBytes());
			db.put(null, key, data);
		}
		i = 0;
		Cursor dbc;
		dbc = db.openCursor(null, CursorConfig.DEFAULT);
		while (dbc.getNext(key, data, LockMode.DEFAULT) ==
		    OperationStatus.SUCCESS) {
			++i;
		}
//		System.out.println("retrieved " + i + " entries");
		dbc.close();
		db.close();

    }
}

class HashComparator implements java.util.Comparator
{
	public int compare(Object o1, Object o2) {
//		System.out.println("Comparing: " + o1 + ":"+o2);
		String s1, s2;
		s1 = new String((byte[])o1);
		s2 = new String((byte[])o2);
		return s1.compareToIgnoreCase(s2);
	}
}

class BtreeComparator implements java.util.Comparator
{
	public int compare(Object o1, Object o2) {
		//System.out.println("Comparing: " + o1 + ":"+o2);
		String s1, s2;
		s1 = new String((byte[])o1);
		s2 = new String((byte[])o2);
		return s1.compareToIgnoreCase(s2);
	}
}

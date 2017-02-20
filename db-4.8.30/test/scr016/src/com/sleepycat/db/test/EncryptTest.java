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
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.UnsupportedEncodingException;

import com.sleepycat.db.test.TestUtils;
public class EncryptTest {
    public static final String ENCRYPTTEST_DBNAME = "encrypttest.db";
    @BeforeClass public static void ClassInit() {
        TestUtils.loadConfig(null);
        TestUtils.check_file_removed(TestUtils.getDBFileName(ENCRYPTTEST_DBNAME), true, true);
    }

    @AfterClass public static void ClassShutdown() {
        TestUtils.check_file_removed(TestUtils.getDBFileName(ENCRYPTTEST_DBNAME), true, true);
    }

    @Before public void PerTestInit()
        throws Exception {
        TestUtils.check_file_removed(TestUtils.getDBFileName(ENCRYPTTEST_DBNAME), true, true);
    }

    @After() public void PerTestShutdown()
        throws Exception {
        TestUtils.check_file_removed(TestUtils.getDBFileName(ENCRYPTTEST_DBNAME), true, true);
    }
    /*
     * Test case implementations.
     * To disable a test mark it with @Ignore
     * To set a timeout(ms) notate like: @Test(timeout=1000)
     * To indicate an expected exception notate like: (expected=Exception)
     */

    /*
     * Test the basic db no env and encryption disabled.
     */
    @Test public void test1()
        throws DatabaseException, FileNotFoundException, UnsupportedEncodingException
    {
        DatabaseConfig dbConf = new DatabaseConfig();
        dbConf.setType(DatabaseType.BTREE);
        dbConf.setAllowCreate(true);
     
        Database db = new Database(TestUtils.getDBFileName(ENCRYPTTEST_DBNAME), null, dbConf);

        DatabaseEntry key = new DatabaseEntry("key".getBytes("UTF-8"));
        DatabaseEntry data = new DatabaseEntry("data".getBytes("UTF-8"));
     
        db.put(null, key, data);
     
        db.close();
        //try { Thread.sleep(10000); } catch(InterruptedException e) {}
     
        if(!findData("key"))
            fail("Did not find the un-encrypted value in the database file after close");
    }

    /*
     * Test database with encryption, no env.
     */
    @Test public void test2()
        throws DatabaseException, FileNotFoundException, UnsupportedEncodingException
    {
        DatabaseConfig dbConf = new DatabaseConfig();
        dbConf.setType(DatabaseType.BTREE);
        dbConf.setAllowCreate(true);
        dbConf.setEncrypted("password");
        dbConf.setErrorStream(System.err);
     
        Database db = new Database(TestUtils.getDBFileName(ENCRYPTTEST_DBNAME), null, dbConf);
     
        DatabaseEntry key = new DatabaseEntry("key".getBytes("UTF-8"));
        DatabaseEntry data = new DatabaseEntry("data".getBytes("UTF-8"));
     
        db.put(null, key, data);
     
        db.sync();
        db.close();
     
        if (findData("key"))
            fail("Found the un-encrypted value in an encrypted database file after close");
    }

    private boolean findData(String toFind)
    {
        boolean found = false;
        try {
            String dbname = TestUtils.getDBFileName(ENCRYPTTEST_DBNAME);
            File f = new File(dbname);
            if (!f.exists() || f.isDirectory()) {
                TestUtils.ERR("Could not find database file: " + dbname + " to look for encrypted string.");
                return false;
            }
            FileInputStream fin = new FileInputStream(f);
            byte[] buf  = new byte[(int)f.length()];
            fin.read(buf, 0, (int)f.length());
            fin.close();
         
            TestUtils.DEBUGOUT(1, "EncryptTest findData file length: " + buf.length);
            byte firstbyte = (toFind.getBytes("UTF-8"))[0];
            // buf can contain non-ascii characters, so no easy string search
            for (int i = 0; i < buf.length - toFind.length(); i++)
            {
                if (buf[i] == firstbyte)
                {
                    if(toFind.compareTo(new String(buf, i, toFind.length())) == 0)
                    {
                        found = true;
                        break;
                    }
                }
            }
        } catch(Exception e) {
        }
        return found;
    }
}

/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */
package com.sleepycat.collections.test.serial;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;

import com.sleepycat.bind.serial.StoredClassCatalog;
import com.sleepycat.compat.DbCompat;
import com.sleepycat.db.Database;
import com.sleepycat.db.DatabaseConfig;
import com.sleepycat.db.Environment;
import com.sleepycat.util.test.SharedTestUtils;
import com.sleepycat.util.test.TestEnv;

/**
 * @author Mark Hayes
 */
public class CatalogCornerCaseTest extends TestCase {

    public static void main(String[] args) {
        junit.framework.TestResult tr =
            junit.textui.TestRunner.run(suite());
        if (tr.errorCount() > 0 ||
            tr.failureCount() > 0) {
            System.exit(1);
        } else {
            System.exit(0);
        }
    }

    public static Test suite() {
        return new TestSuite(CatalogCornerCaseTest.class);
    }

    private Environment env;

    public CatalogCornerCaseTest(String name) {

        super(name);
    }

    @Override
    public void setUp()
        throws Exception {

        SharedTestUtils.printTestName(getName());
        env = TestEnv.BDB.open(getName());
    }

    @Override
    public void tearDown() {

        try {
            if (env != null) {
                env.close();
            }
        } catch (Exception e) {
            System.out.println("Ignored exception during tearDown: " + e);
        } finally {
            /* Ensure that GC can cleanup. */
            env = null;
        }
    }

    public void testReadOnlyEmptyCatalog()
        throws Exception {

        String file = "catalog.db";

        /* Create an empty database. */
        DatabaseConfig config = new DatabaseConfig();
        config.setAllowCreate(true);
        DbCompat.setTypeBtree(config);
        Database db =
            DbCompat.testOpenDatabase(env, null, file, null, config);
        db.close();

        /* Open the empty database read-only. */
        config.setAllowCreate(false);
        config.setReadOnly(true);
        db = DbCompat.testOpenDatabase(env, null, file, null, config);

        /* Expect exception when creating the catalog. */
        try {
            new StoredClassCatalog(db);
            fail();
        } catch (IllegalStateException e) { }
        db.close();
    }
}

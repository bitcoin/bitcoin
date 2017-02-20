/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002,2009 Oracle.  All rights reserved.
 *
 * $Id: DualTestCase.java,v 1.6 2009/01/13 10:41:22 cwl Exp $
 */

package com.sleepycat.db.util;

import java.io.File;
import java.io.FileNotFoundException;

import junit.framework.TestCase;

import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.Environment;
import com.sleepycat.db.EnvironmentConfig;

public class DualTestCase extends TestCase {

    private Environment env;
    private boolean setUpInvoked = false;

    public DualTestCase() {
        super();
    }

    protected DualTestCase(String name) {
        super(name);
    }

    @Override
    protected void setUp()
        throws Exception {

        setUpInvoked = true;
        super.setUp();
    }

    @Override
    protected void tearDown()
        throws Exception {

        if (!setUpInvoked) {
            throw new IllegalStateException
                ("tearDown was invoked without a corresponding setUp() call");
        }
        destroy();
        super.tearDown();
    }

    protected Environment create(File envHome, EnvironmentConfig envConfig)
        throws DatabaseException {

        try {
            env = new Environment(envHome, envConfig);
        } catch (FileNotFoundException e) {
            throw new RuntimeException(e);
        }
        return env;
    }

    protected void close(Environment environment)
        throws DatabaseException {

        env.close();
        env = null;
    }

    protected void destroy()
        throws Exception {

        if (env != null) {
            try {
                /* Close in case we hit an exception and didn't close */
                env.close();
            } catch (RuntimeException e) {
                /* OK if already closed */
            }
            env = null;
        }
    }

    public static boolean isReplicatedTest(Class<?> testCaseClass) {
        return false;
    }
}

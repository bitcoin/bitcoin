/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.util.test;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;

import com.sleepycat.util.FastOutputStream;

/**
 * @author Mark Hayes
 */
public class FastOutputStreamTest extends TestCase {

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
        TestSuite suite = new TestSuite(FastOutputStreamTest.class);
        return suite;
    }

    public FastOutputStreamTest(String name) {

        super(name);
    }

    @Override
    public void setUp() {

        SharedTestUtils.printTestName("FastOutputStreamTest." + getName());
    }

    public void testBufferSizing() {
        FastOutputStream fos = new FastOutputStream();
        assertEquals
            (FastOutputStream.DEFAULT_INIT_SIZE, fos.getBufferBytes().length);

        /* Write X+1 bytes, expect array size 2X+1 */
        fos.write(new byte[FastOutputStream.DEFAULT_INIT_SIZE + 1]);
        assertEquals
            ((FastOutputStream.DEFAULT_INIT_SIZE * 2) + 1,
             fos.getBufferBytes().length);

        /* Write X+1 bytes, expect array size 4X+3 = (2(2X+1) + 1) */
        fos.write(new byte[FastOutputStream.DEFAULT_INIT_SIZE + 1]);
        assertEquals
            ((FastOutputStream.DEFAULT_INIT_SIZE * 4) + 3,
             fos.getBufferBytes().length);
    }
}

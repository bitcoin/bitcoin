/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */
package com.sleepycat.persist.test;

import junit.framework.Test;

import com.sleepycat.util.test.SharedTestUtils;

/**
 * Runs part one of the EvolveTest.  This part is run with the old/original
 * version of EvolveClasses in the classpath.  It creates a fresh environment
 * and store containing instances of the original class.  When EvolveTest is
 * run, it will read/write/evolve these objects from the store created here.
 *
 * @author Mark Hayes
 */
public class EvolveTestInit extends EvolveTestBase {

    public static Test suite()
        throws Exception {

        return getSuite(EvolveTestInit.class);
    }

    @Override
    boolean useEvolvedClass() {
        return false;
    }

    @Override
    public void setUp() {
        envHome = getTestInitHome(false /*evolved*/);
        envHome.mkdirs();
        SharedTestUtils.emptyDir(envHome);
    }

    public void testInit()
        throws Exception {

        openEnv();
        if (!openStoreReadWrite()) {
            fail();
        }
        caseObj.writeObjects(store);
        caseObj.checkUnevolvedModel(store.getModel(), env);
        closeAll();
    }
}

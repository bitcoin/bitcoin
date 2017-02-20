/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */
package com.sleepycat.persist.test;

import java.util.Iterator;
import java.util.List;

import junit.framework.TestCase;

import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.Environment;
import com.sleepycat.persist.EntityStore;
import com.sleepycat.persist.StoreConfig;
import com.sleepycat.persist.evolve.Mutations;
import com.sleepycat.persist.model.ClassMetadata;
import com.sleepycat.persist.model.EntityModel;
import com.sleepycat.persist.model.Persistent;
import com.sleepycat.persist.raw.RawStore;
import com.sleepycat.persist.raw.RawType;

@Persistent
abstract class EvolveCase {

    static final String STORE_NAME = "foo";

    transient boolean updated;

    Mutations getMutations() {
        return null;
    }

    void configure(EntityModel model, StoreConfig config) {
    }

    String getStoreOpenException() {
        return null;
    }

    int getNRecordsExpected() {
        return 1;
    }

    void checkUnevolvedModel(EntityModel model, Environment env) {
    }

    void checkEvolvedModel(EntityModel model,
                           Environment env,
                           boolean oldTypesExist) {
    }

    /**
     * @throws DatabaseException from subclasses.
     */
    void writeObjects(EntityStore store)
        throws DatabaseException {
    }

    /**
     * @throws DatabaseException from subclasses.
     */
    void readObjects(EntityStore store, boolean doUpdate)
        throws DatabaseException {
    }

    /**
     * @throws DatabaseException from subclasses.
     */
    void readRawObjects(RawStore store,
                        boolean expectEvolved,
                        boolean expectUpdated)
        throws DatabaseException {
    }

    /**
     * @throws DatabaseException from subclasses.
     */
    void copyRawObjects(RawStore rawStore, EntityStore newStore)
        throws DatabaseException {
    }

    /**
     * Checks for equality and prints the entire values rather than
     * abbreviated values like TestCase.assertEquals does.
     */
    static void checkEquals(Object expected, Object got) {
        if ((expected != null) ? (!expected.equals(got)) : (got != null)) {
            TestCase.fail("Expected:\n" + expected + "\nBut got:\n" + got);
        }
    }

    /**
     * Asserts than an entity database exists or does not exist.
     */
    static void assertDbExists(boolean expectExists,
                               Environment env,
                               String entityClassName) {
        assertDbExists(expectExists, env, entityClassName, null);
    }

    /**
     * Checks that an entity class exists or does not exist.
     */
    static void checkEntity(boolean exists,
                            EntityModel model,
                            Environment env,
                            String className,
                            int version,
                            String secKeyName) {
        if (exists) {
            TestCase.assertNotNull(model.getEntityMetadata(className));
            ClassMetadata meta = model.getClassMetadata(className);
            TestCase.assertNotNull(meta);
            TestCase.assertEquals(version, meta.getVersion());
            TestCase.assertTrue(meta.isEntityClass());

            RawType raw = model.getRawType(className);
            TestCase.assertNotNull(raw);
            TestCase.assertEquals(version, raw.getVersion());
        } else {
            TestCase.assertNull(model.getEntityMetadata(className));
            TestCase.assertNull(model.getClassMetadata(className));
            TestCase.assertNull(model.getRawType(className));
        }

        assertDbExists(exists, env, className);
        if (secKeyName != null) {
            assertDbExists(exists, env, className, secKeyName);
        }
    }

    /**
     * Checks that a non-entity class exists or does not exist.
     */
    static void checkNonEntity(boolean exists,
                               EntityModel model,
                               Environment env,
                               String className,
                               int version) {
        if (exists) {
            ClassMetadata meta = model.getClassMetadata(className);
            TestCase.assertNotNull(meta);
            TestCase.assertEquals(version, meta.getVersion());
            TestCase.assertTrue(!meta.isEntityClass());

            RawType raw = model.getRawType(className);
            TestCase.assertNotNull(raw);
            TestCase.assertEquals(version, raw.getVersion());
        } else {
            TestCase.assertNull(model.getClassMetadata(className));
            TestCase.assertNull(model.getRawType(className));
        }

        TestCase.assertNull(model.getEntityMetadata(className));
        assertDbExists(false, env, className);
    }

    /**
     * Asserts than a database expectExists or does not exist. If keyName is
     * null, checks an entity database.  If keyName is non-null, checks a
     * secondary database.
     */
    static void assertDbExists(boolean expectExists,
                               Environment env,
                               String entityClassName,
                               String keyName) {
        PersistTestUtils.assertDbExists
            (expectExists, env, STORE_NAME, entityClassName, keyName);
    }

    static void checkVersions(EntityModel model, String name, int version) {
        checkVersions(model, new String[] {name}, new int[] {version});
    }

    static void checkVersions(EntityModel model,
                              String name1,
                              int version1,
                              String name2,
                              int version2) {
        checkVersions
            (model, new String[] {name1, name2},
             new int[] {version1, version2});
    }

    private static void checkVersions(EntityModel model,
                                      String[] names,
                                      int[] versions) {
        List<RawType> all = model.getAllRawTypeVersions(names[0]);
        TestCase.assertNotNull(all);

        assert names.length == versions.length;
        TestCase.assertEquals(all.toString(), names.length, all.size());

        Iterator<RawType> iter = all.iterator();
        for (int i = 0; i < names.length; i += 1) {
            RawType type = iter.next();
            TestCase.assertEquals(versions[i], type.getVersion());
            TestCase.assertEquals(names[i], type.getClassName());
        }
    }
}

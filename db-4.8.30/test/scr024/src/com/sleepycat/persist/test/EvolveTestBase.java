/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */
package com.sleepycat.persist.test;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.Enumeration;

import junit.framework.TestCase;
import junit.framework.TestSuite;

import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.Environment;
import com.sleepycat.db.EnvironmentConfig;
import com.sleepycat.persist.EntityStore;
import com.sleepycat.persist.StoreConfig;
import com.sleepycat.persist.model.AnnotationModel;
import com.sleepycat.persist.model.EntityModel;
import com.sleepycat.persist.raw.RawStore;
import com.sleepycat.util.test.TestEnv;

/**
 * Base class for EvolveTest and EvolveTestInit.
 *
 * @author Mark Hayes
 */
public abstract class EvolveTestBase extends TestCase {

    /*
     * When adding a evolve test class, three places need to be changed:
     * 1) Add the unmodified class to EvolveClass.java.original.
     * 2) Add the modified class to EvolveClass.java.
     * 3) Add the class name to the ALL list below as a pair of strings.  The
     * first string in each pair is the name of the original class, and the
     * second string is the name of the evolved class or null if the evolved
     * name is the same as the original.  The index in the list identifies a
     * test case, and the class at that position identifies the old and new
     * class to use for the test.
     */
    private static final String[] ALL = {
//*
        "DeletedEntity1_ClassRemoved",
        "DeletedEntity1_ClassRemoved_NoMutation",
        "DeletedEntity2_ClassRemoved",
        "DeletedEntity2_ClassRemoved_WithDeleter",
        "DeletedEntity3_AnnotRemoved_NoMutation",
        null,
        "DeletedEntity4_AnnotRemoved_WithDeleter",
        null,
        "DeletedEntity5_EntityToPersist_NoMutation",
        null,
        "DeletedEntity6_EntityToPersist_WithDeleter",
        null,
        "DeletedPersist1_ClassRemoved_NoMutation",
        null,
        "DeletedPersist2_ClassRemoved_WithDeleter",
        null,
        "DeletedPersist3_AnnotRemoved_NoMutation",
        null,
        "DeletedPersist4_AnnotRemoved_WithDeleter",
        null,
        "DeletedPersist5_PersistToEntity_NoMutation",
        null,
        "DeletedPersist6_PersistToEntity_WithDeleter",
        null,
        "RenamedEntity1_NewEntityName",
        "RenamedEntity1_NewEntityName_NoMutation",
        "RenamedEntity2_NewEntityName",
        "RenamedEntity2_NewEntityName_WithRenamer",
        "DeleteSuperclass1_NoMutation",
        null,
        "DeleteSuperclass2_WithConverter",
        null,
        "DeleteSuperclass3_WithDeleter",
        null,
        "DeleteSuperclass4_NoFields",
        null,
        "DeleteSuperclass5_Top",
        null,
        "InsertSuperclass1_Between",
        null,
        "InsertSuperclass2_Top",
        null,
        "DisallowNonKeyField_PrimitiveToObject",
        null,
        "DisallowNonKeyField_ObjectToPrimitive",
        null,
        "DisallowNonKeyField_ObjectToSubtype",
        null,
        "DisallowNonKeyField_ObjectToUnrelatedSimple",
        null,
        "DisallowNonKeyField_ObjectToUnrelatedOther",
        null,
        "DisallowNonKeyField_byte2boolean",
        null,
        "DisallowNonKeyField_short2byte",
        null,
        "DisallowNonKeyField_int2short",
        null,
        "DisallowNonKeyField_long2int",
        null,
        "DisallowNonKeyField_float2long",
        null,
        "DisallowNonKeyField_double2float",
        null,
        "DisallowNonKeyField_Byte2byte",
        null,
        "DisallowNonKeyField_Character2char",
        null,
        "DisallowNonKeyField_Short2short",
        null,
        "DisallowNonKeyField_Integer2int",
        null,
        "DisallowNonKeyField_Long2long",
        null,
        "DisallowNonKeyField_Float2float",
        null,
        "DisallowNonKeyField_Double2double",
        null,
        "DisallowNonKeyField_float2BigInt",
        null,
        "DisallowNonKeyField_BigInt2long",
        null,
        "DisallowSecKeyField_byte2short",
        null,
        "DisallowSecKeyField_char2int",
        null,
        "DisallowSecKeyField_short2int",
        null,
        "DisallowSecKeyField_int2long",
        null,
        "DisallowSecKeyField_long2float",
        null,
        "DisallowSecKeyField_float2double",
        null,
        "DisallowSecKeyField_Byte2short2",
        null,
        "DisallowSecKeyField_Character2int",
        null,
        "DisallowSecKeyField_Short2int2",
        null,
        "DisallowSecKeyField_Integer2long",
        null,
        "DisallowSecKeyField_Long2float2",
        null,
        "DisallowSecKeyField_Float2double2",
        null,
        "DisallowSecKeyField_int2BigInt",
        null,
        "DisallowPriKeyField_byte2short",
        null,
        "DisallowPriKeyField_char2int",
        null,
        "DisallowPriKeyField_short2int",
        null,
        "DisallowPriKeyField_int2long",
        null,
        "DisallowPriKeyField_long2float",
        null,
        "DisallowPriKeyField_float2double",
        null,
        "DisallowPriKeyField_Byte2short2",
        null,
        "DisallowPriKeyField_Character2int",
        null,
        "DisallowPriKeyField_Short2int2",
        null,
        "DisallowPriKeyField_Integer2long",
        null,
        "DisallowPriKeyField_Long2float2",
        null,
        "DisallowPriKeyField_Float2double2",
        null,
        "DisallowPriKeyField_Long2BigInt",
        null,
        "DisallowCompositeKeyField_byte2short",
        null,
        "AllowPriKeyField_Byte2byte2",
        null,
        "AllowPriKeyField_byte2Byte",
        null,
        "AllowFieldTypeChanges",
        null,
        "ConvertFieldContent_Entity",
        null,
        "ConvertExample1_Entity",
        null,
        "ConvertExample2_Person",
        null,
        "ConvertExample3_Person",
        null,
        "ConvertExample3Reverse_Person",
        null,
        "ConvertExample4_Entity",
        null,
        "ConvertExample5_Entity",
        null,
        "AllowFieldAddDelete",
        null,
        "ProxiedClass_Entity",
        null,
        "DisallowChangeProxyFor",
        null,
        "DisallowDeleteProxyFor",
        null,
        "ArrayNameChange_Entity",
        null,
        "AddEnumConstant_Entity",
        null,
        "InsertEnumConstant_Entity",
        null,
        "DeleteEnumConstant_NoMutation",
        null,
        "DisallowChangeKeyRelate",
        null,
        "AllowChangeKeyMetadata",
        null,
        "AllowChangeKeyMetadataInSubclass",
        null,
        "AllowAddSecondary",
        null,
        "FieldAddAndConvert",
        null,
//*/
    };

    File envHome;
    Environment env;
    EntityStore store;
    RawStore rawStore;
    EntityStore newStore;
    String caseClsName;
    Class caseCls;
    EvolveCase caseObj;
    String caseLabel;

    static TestSuite getSuite(Class testClass)
        throws Exception {

        TestSuite suite = new TestSuite();
        for (int i = 0; i < ALL.length; i += 2) {
            String originalClsName = ALL[i];
            String evolvedClsName = ALL[i + 1];
            if (evolvedClsName == null) {
                evolvedClsName = originalClsName;
            }
            TestSuite baseSuite = new TestSuite(testClass);
            Enumeration e = baseSuite.tests();
            while (e.hasMoreElements()) {
                EvolveTestBase test = (EvolveTestBase) e.nextElement();
                test.init(originalClsName, evolvedClsName);
                suite.addTest(test);
            }
        }
        return suite;
    }

    private void init(String originalClsName,
                      String evolvedClsName) 
        throws Exception {

        String caseClsName = useEvolvedClass() ?
            evolvedClsName : originalClsName;
        caseClsName = "com.sleepycat.persist.test.EvolveClasses$" +
                      caseClsName;

        this.caseClsName = caseClsName;
        this.caseCls = Class.forName(caseClsName);
        this.caseObj = (EvolveCase) caseCls.newInstance();
        this.caseLabel = evolvedClsName;
    }

    abstract boolean useEvolvedClass();

    File getTestInitHome(boolean evolved) {
        return new File
            (System.getProperty("testevolvedir"),
             (evolved ? "evolved" : "original") + '/' + caseLabel);
    }

    @Override
    public void tearDown() {

        /* Set test name for reporting; cannot be done in the ctor or setUp. */
        setName(caseLabel + '-' + getName());

        if (env != null) {
            try {
                closeAll();
            } catch (Throwable e) {
                System.out.println("During tearDown: " + e);
            }
        }
        envHome = null;
        env = null;
        store = null;
        caseCls = null;
        caseObj = null;
        caseLabel = null;

        /* Do not delete log files so they can be used by 2nd phase of test. */
    }

    /**
     * @throws FileNotFoundException from DB core.
     */
    void openEnv()
        throws FileNotFoundException, DatabaseException {

        EnvironmentConfig config = TestEnv.TXN.getConfig();
        config.setAllowCreate(true);
        env = new Environment(envHome, config);
    }

    /**
     * Returns true if the store was opened successfully.  Returns false if the
     * store could not be opened because an exception was expected -- this is
     * not a test failure but no further tests for an EntityStore may be run.
     */
    private boolean openStore(StoreConfig config)
        throws Exception {

        config.setTransactional(true);
        config.setMutations(caseObj.getMutations());

        EntityModel model = new AnnotationModel();
        config.setModel(model);
        caseObj.configure(model, config);

        String expectException = caseObj.getStoreOpenException();
        try {
            store = new EntityStore(env, EvolveCase.STORE_NAME, config);
            if (expectException != null) {
                fail("Expected: " + expectException);
            }
        } catch (Exception e) {
            if (expectException != null) {
                //e.printStackTrace();
                String actualMsg = e.getMessage();
                EvolveCase.checkEquals
                    (expectException,
                     e.getClass().getName() + ": " + actualMsg);
                return false;
            } else {
                throw e;
            }
        }
        return true;
    }

    boolean openStoreReadOnly()
        throws Exception {

        StoreConfig config = new StoreConfig();
        config.setReadOnly(true);
        return openStore(config);
    }

    boolean openStoreReadWrite()
        throws Exception {

        StoreConfig config = new StoreConfig();
        config.setAllowCreate(true);
        return openStore(config);
    }

    void openRawStore()
        throws DatabaseException {

        StoreConfig config = new StoreConfig();
        config.setTransactional(true);
        rawStore = new RawStore(env, EvolveCase.STORE_NAME, config);
    }

    void closeStore()
        throws DatabaseException {

        if (store != null) {
            store.close();
            store = null;
        }
    }

    void openNewStore()
        throws Exception {

        StoreConfig config = new StoreConfig();
        config.setAllowCreate(true);
        config.setTransactional(true);

        EntityModel model = new AnnotationModel();
        config.setModel(model);
        caseObj.configure(model, config);

        newStore = new EntityStore(env, "new", config);
    }

    void closeNewStore()
        throws DatabaseException {

        if (newStore != null) {
            newStore.close();
            newStore = null;
        }
    }

    void closeRawStore()
        throws DatabaseException {

        if (rawStore != null) {
            rawStore.close();
            rawStore = null;
        }
    }

    void closeEnv()
        throws DatabaseException {

        if (env != null) {
            env.close();
            env = null;
        }
    }

    void closeAll()
        throws DatabaseException {

        closeStore();
        closeRawStore();
        closeNewStore();
        closeEnv();
    }
}

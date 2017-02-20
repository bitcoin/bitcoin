/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */
package com.sleepycat.persist.test;

import static com.sleepycat.persist.model.Relationship.MANY_TO_ONE;
import static com.sleepycat.persist.model.Relationship.ONE_TO_ONE;

import java.math.BigInteger;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.StringTokenizer;

import junit.framework.TestCase;

import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.Environment;
import com.sleepycat.persist.EntityStore;
import com.sleepycat.persist.PrimaryIndex;
import com.sleepycat.persist.SecondaryIndex;
import com.sleepycat.persist.StoreConfig;
import com.sleepycat.persist.evolve.Conversion;
import com.sleepycat.persist.evolve.Converter;
import com.sleepycat.persist.evolve.Deleter;
import com.sleepycat.persist.evolve.EntityConverter;
import com.sleepycat.persist.evolve.Mutations;
import com.sleepycat.persist.evolve.Renamer;
import com.sleepycat.persist.model.Entity;
import com.sleepycat.persist.model.EntityModel;
import com.sleepycat.persist.model.KeyField;
import com.sleepycat.persist.model.Persistent;
import com.sleepycat.persist.model.PersistentProxy;
import com.sleepycat.persist.model.PrimaryKey;
import com.sleepycat.persist.model.SecondaryKey;
import com.sleepycat.persist.raw.RawObject;
import com.sleepycat.persist.raw.RawStore;
import com.sleepycat.persist.raw.RawType;

/**
 * Nested classes are modified versions of classes of the same name in
 * EvolveClasses.java.original.  See EvolveTestBase.java for the steps that are
 * taken to add a new class (test case).
 *
 * @author Mark Hayes
 */
class EvolveClasses {

    private static final String PREFIX = EvolveClasses.class.getName() + '$';
    private static final String CASECLS = EvolveCase.class.getName();

    private static RawObject readRaw(RawStore store,
                                     Object key,
                                     Object... classVersionPairs)
        throws DatabaseException {

        return readRaw(store, null, key, classVersionPairs);
    }

    /**
     * Reads a raw object and checks its superclass names and versions.
     */
    private static RawObject readRaw(RawStore store,
                                     String entityClsName,
                                     Object key,
                                     Object... classVersionPairs)
        throws DatabaseException {

        TestCase.assertNotNull(store);
        TestCase.assertNotNull(key);

        if (entityClsName == null) {
            entityClsName = (String) classVersionPairs[0];
        }
        PrimaryIndex<Object,RawObject> index =
            store.getPrimaryIndex(entityClsName);
        TestCase.assertNotNull(index);

        RawObject obj = index.get(key);
        TestCase.assertNotNull(obj);

        checkRawType(obj.getType(), classVersionPairs);

        RawObject superObj = obj.getSuper();
        for (int i = 2; i < classVersionPairs.length; i += 2) {
            Object[] a = new Object[classVersionPairs.length - i];
            System.arraycopy(classVersionPairs, i, a, 0, a.length);
            TestCase.assertNotNull(superObj);
            checkRawType(superObj.getType(), a);
            superObj = superObj.getSuper();
        }

        return obj;
    }

    /**
     * Reads a raw object and checks its superclass names and versions.
     */
    private static void checkRawType(RawType type,
                                     Object... classVersionPairs) {
        TestCase.assertNotNull(type);
        TestCase.assertNotNull(classVersionPairs);
        TestCase.assertTrue(classVersionPairs.length % 2 == 0);

        for (int i = 0; i < classVersionPairs.length; i += 2) {
            String clsName = (String) classVersionPairs[i];
            int clsVersion = (Integer) classVersionPairs[i + 1];
            TestCase.assertEquals(clsName, type.getClassName());
            TestCase.assertEquals(clsVersion, type.getVersion());
            type = type.getSuperType();
        }
        TestCase.assertNull(type);
    }

    /**
     * Checks that a raw object contains the specified field values.  Does not
     * check superclass fields.
     */
    private static void checkRawFields(RawObject obj,
                                       Object... nameValuePairs) {
        TestCase.assertNotNull(obj);
        TestCase.assertNotNull(obj.getValues());
        TestCase.assertNotNull(nameValuePairs);
        TestCase.assertTrue(nameValuePairs.length % 2 == 0);

        Map<String,Object> values = obj.getValues();
        TestCase.assertEquals(nameValuePairs.length / 2, values.size());

        for (int i = 0; i < nameValuePairs.length; i += 2) {
            String name = (String) nameValuePairs[i];
            Object value = nameValuePairs[i + 1];
            TestCase.assertEquals(name, value, values.get(name));
        }
    }

    private static Map<String,Object> makeValues(Object... nameValuePairs) {
        TestCase.assertTrue(nameValuePairs.length % 2 == 0);
        Map<String,Object> values = new HashMap<String,Object>();
        for (int i = 0; i < nameValuePairs.length; i += 2) {
            values.put((String) nameValuePairs[i], nameValuePairs[i + 1]);
        }
        return values;
    }

    /**
     * Disallow removing an entity class when no Deleter mutation is specified.
     */
    static class DeletedEntity1_ClassRemoved_NoMutation extends EvolveCase {

        private static final String NAME =
            PREFIX + "DeletedEntity1_ClassRemoved";

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DeletedEntity1_ClassRemoved version: 0 Error: java.lang.ClassNotFoundException: com.sleepycat.persist.test.EvolveClasses$DeletedEntity1_ClassRemoved";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, "skey");
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "skey", 88);
        }
    }

    /**
     * Allow removing an entity class when a Deleter mutation is specified.
     */
    static class DeletedEntity2_ClassRemoved_WithDeleter extends EvolveCase {

        private static final String NAME =
            PREFIX + "DeletedEntity2_ClassRemoved";

        @Override
        int getNRecordsExpected() {
            return 0;
        }

        @Override
        Mutations getMutations() {
            Mutations m = new Mutations();
            m.addDeleter(new Deleter(NAME, 0));
            return m;
        }

        @Override
        void checkEvolvedModel(EntityModel model,
                               Environment env,
                               boolean oldTypesExist) {
            checkEntity(false, model, env, NAME, 0, "skey");
            if (oldTypesExist) {
                checkVersions(model, NAME, 0);
            }
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                return;
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "skey", 88);
        }
    }

    /**
     * Disallow removing the Entity annotation when no Deleter mutation is
     * specified.
     */
    static class DeletedEntity3_AnnotRemoved_NoMutation extends EvolveCase {

        private static final String NAME =
            DeletedEntity3_AnnotRemoved_NoMutation.class.getName();

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DeletedEntity3_AnnotRemoved_NoMutation version: 0 Error: java.lang.IllegalArgumentException: Class could not be loaded or is not persistent: com.sleepycat.persist.test.EvolveClasses$DeletedEntity3_AnnotRemoved_NoMutation";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, "skey");
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "skey", 88);
        }
    }

    /**
     * Allow removing the Entity annotation when a Deleter mutation is
     * specified.
     */
    static class DeletedEntity4_AnnotRemoved_WithDeleter extends EvolveCase {

        private static final String NAME =
            DeletedEntity4_AnnotRemoved_WithDeleter.class.getName();

        @Override
        int getNRecordsExpected() {
            return 0;
        }

        @Override
        Mutations getMutations() {
            Mutations m = new Mutations();
            m.addDeleter(new Deleter(NAME, 0));
            return m;
        }

        @Override
        void checkEvolvedModel(EntityModel model,
                               Environment env,
                               boolean oldTypesExist) {
            checkEntity(false, model, env, NAME, 0, "skey");
            if (oldTypesExist) {
                checkVersions(model, NAME, 0);
            }
        }

        @Override
        void readObjects(EntityStore store, boolean doUpdate) {
            try {
                store.getPrimaryIndex
                    (Integer.class,
                     DeletedEntity4_AnnotRemoved_WithDeleter.class);
                TestCase.fail();
            } catch (Exception e) {
                checkEquals
                    ("java.lang.IllegalArgumentException: Class could not be loaded or is not an entity class: com.sleepycat.persist.test.EvolveClasses$DeletedEntity4_AnnotRemoved_WithDeleter",
                     e.toString());
            }
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                return;
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "skey", 88);
        }
    }

    /**
     * Disallow changing the Entity annotation to Persistent when no Deleter
     * mutation is specified.
     */
    @Persistent(version=1)
    static class DeletedEntity5_EntityToPersist_NoMutation extends EvolveCase {

        private static final String NAME =
            DeletedEntity5_EntityToPersist_NoMutation.class.getName();

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DeletedEntity5_EntityToPersist_NoMutation version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DeletedEntity5_EntityToPersist_NoMutation version: 1 Error: @Entity switched to/from @Persistent";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, "skey");
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "skey", 88);
        }
    }

    /**
     * Allow changing the Entity annotation to Persistent when a Deleter
     * mutation is specified.
     */
    @Persistent(version=1)
    static class DeletedEntity6_EntityToPersist_WithDeleter extends EvolveCase {

        private static final String NAME =
            DeletedEntity6_EntityToPersist_WithDeleter.class.getName();
        private static final String NAME2 =
            Embed_DeletedEntity6_EntityToPersist_WithDeleter.class.getName();

        @Override
        int getNRecordsExpected() {
            return 0;
        }

        @Override
        Mutations getMutations() {
            Mutations m = new Mutations();
            m.addDeleter(new Deleter(NAME, 0));
            return m;
        }

        @Override
        void checkEvolvedModel(EntityModel model,
                               Environment env,
                               boolean oldTypesExist) {
            checkNonEntity(true, model, env, NAME, 1);
            if (oldTypesExist) {
                checkVersions(model, NAME, 1, NAME, 0);
            } else {
                checkVersions(model, NAME, 1);
            }
        }

        @Override
        void readObjects(EntityStore store, boolean doUpdate)
            throws DatabaseException {

            /* Cannot get the primary index for the former entity class. */
            try {
                store.getPrimaryIndex
                    (Integer.class,
                     DeletedEntity6_EntityToPersist_WithDeleter.class);
                TestCase.fail();
            } catch (Exception e) {
                checkEquals
                    ("java.lang.IllegalArgumentException: Class could not be loaded or is not an entity class: com.sleepycat.persist.test.EvolveClasses$DeletedEntity6_EntityToPersist_WithDeleter",
                     e.toString());
            }

            /* Can embed the now persistent class in another entity class. */
            PrimaryIndex<Long,
                         Embed_DeletedEntity6_EntityToPersist_WithDeleter>
                index = store.getPrimaryIndex
                    (Long.class,
                     Embed_DeletedEntity6_EntityToPersist_WithDeleter.class);

            if (doUpdate) {
                Embed_DeletedEntity6_EntityToPersist_WithDeleter embed =
                    new Embed_DeletedEntity6_EntityToPersist_WithDeleter();
                index.put(embed);
                embed = index.get(embed.key);
                /* This new type should exist only after update. */
                Environment env = store.getEnvironment();
                EntityModel model = store.getModel();
                checkEntity(true, model, env, NAME2, 0, null);
                checkVersions(model, NAME2, 0);
            }
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                return;
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "skey", 88);
        }
    }

    @Entity
    static class Embed_DeletedEntity6_EntityToPersist_WithDeleter {

        @PrimaryKey
        long key = 99;

        DeletedEntity6_EntityToPersist_WithDeleter embedded =
            new DeletedEntity6_EntityToPersist_WithDeleter();
    }

    /**
     * Disallow removing a Persistent class when no Deleter mutation is
     * specified, even when the Entity class that embedded the Persistent class
     * is deleted properly (by removing the Entity annotation in this case).
     */
    static class DeletedPersist1_ClassRemoved_NoMutation extends EvolveCase {

        private static final String NAME =
            PREFIX + "DeletedPersist1_ClassRemoved";

        private static final String NAME2 =
            DeletedPersist1_ClassRemoved_NoMutation.class.getName();

        @Override
        Mutations getMutations() {
            Mutations m = new Mutations();
            m.addDeleter(new Deleter(NAME2, 0));
            return m;
        }

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DeletedPersist1_ClassRemoved version: 0 Error: java.lang.ClassNotFoundException: com.sleepycat.persist.test.EvolveClasses$DeletedPersist1_ClassRemoved";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkNonEntity(true, model, env, NAME, 0);
            checkEntity(true, model, env, NAME2, 0, null);
            checkVersions(model, NAME, 0);
            checkVersions(model, NAME2, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }

            RawType embedType = store.getModel().getRawType(NAME);
            checkRawType(embedType, NAME, 0);

            RawObject embed =
                new RawObject(embedType, makeValues("f", 123), null);

            RawObject obj = readRaw(store, 99, NAME2, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "embed", embed);
        }
    }

    /**
     * Allow removing a Persistent class when a Deleter mutation is
     * specified, and the Entity class that embedded the Persistent class
     * is also deleted properly (by removing the Entity annotation in this
     * case).
     */
    static class DeletedPersist2_ClassRemoved_WithDeleter extends EvolveCase {

        private static final String NAME =
            PREFIX + "DeletedPersist2_ClassRemoved";
        private static final String NAME2 =
            DeletedPersist2_ClassRemoved_WithDeleter.class.getName();

        @Override
        int getNRecordsExpected() {
            return 0;
        }

        @Override
        Mutations getMutations() {
            Mutations m = new Mutations();
            m.addDeleter(new Deleter(NAME, 0));
            m.addDeleter(new Deleter(NAME2, 0));
            return m;
        }

        @Override
        void checkEvolvedModel(EntityModel model,
                               Environment env,
                               boolean oldTypesExist) {
            checkNonEntity(false, model, env, NAME, 0);
            checkEntity(false, model, env, NAME2, 0, null);
            if (oldTypesExist) {
                checkVersions(model, NAME, 0);
                checkVersions(model, NAME2, 0);
            }
        }

        @Override
        void readObjects(EntityStore store, boolean doUpdate) {
            try {
                store.getPrimaryIndex
                    (Integer.class,
                     DeletedPersist2_ClassRemoved_WithDeleter.class);
                TestCase.fail();
            } catch (Exception e) {
                checkEquals
                    ("java.lang.IllegalArgumentException: Class could not be loaded or is not an entity class: com.sleepycat.persist.test.EvolveClasses$DeletedPersist2_ClassRemoved_WithDeleter",
                     e.toString());
            }
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                return;
            }

            RawType embedType = store.getModel().getRawType(NAME);
            checkRawType(embedType, NAME, 0);

            RawObject embed =
                new RawObject(embedType, makeValues("f", 123), null);

            RawObject obj = readRaw(store, 99, NAME2, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "embed", embed);
        }
    }

    static class DeletedPersist3_AnnotRemoved {

        int f = 123;
    }

    /**
     * Disallow removing the Persistent annotation when no Deleter mutation is
     * specified, even when the Entity class that embedded the Persistent class
     * is deleted properly (by removing the Entity annotation in this case).
     */
    static class DeletedPersist3_AnnotRemoved_NoMutation extends EvolveCase {

        private static final String NAME =
            DeletedPersist3_AnnotRemoved.class.getName();
        private static final String NAME2 =
            DeletedPersist3_AnnotRemoved_NoMutation.class.getName();

        @Override
        Mutations getMutations() {
            Mutations m = new Mutations();
            m.addDeleter(new Deleter(NAME2, 0));
            return m;
        }

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DeletedPersist3_AnnotRemoved version: 0 Error: java.lang.IllegalArgumentException: Class could not be loaded or is not persistent: com.sleepycat.persist.test.EvolveClasses$DeletedPersist3_AnnotRemoved";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkNonEntity(true, model, env, NAME, 0);
            checkEntity(true, model, env, NAME2, 0, null);
            checkVersions(model, NAME, 0);
            checkVersions(model, NAME2, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }

            RawType embedType = store.getModel().getRawType(NAME);
            checkRawType(embedType, NAME, 0);

            RawObject embed =
                new RawObject(embedType, makeValues("f", 123), null);

            RawObject obj = readRaw(store, 99, NAME2, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "embed", embed);
        }
    }

    static class DeletedPersist4_AnnotRemoved {

        int f = 123;
    }

    /**
     * Allow removing the Persistent annotation when a Deleter mutation is
     * specified, and the Entity class that embedded the Persistent class
     * is also be deleted properly (by removing the Entity annotation in this
     * case).
     */
    static class DeletedPersist4_AnnotRemoved_WithDeleter extends EvolveCase {

        private static final String NAME =
            DeletedPersist4_AnnotRemoved.class.getName();
        private static final String NAME2 =
            DeletedPersist4_AnnotRemoved_WithDeleter.class.getName();

        @Override
        int getNRecordsExpected() {
            return 0;
        }

        @Override
        Mutations getMutations() {
            Mutations m = new Mutations();
            m.addDeleter(new Deleter(NAME, 0));
            m.addDeleter(new Deleter(NAME2, 0));
            return m;
        }

        @Override
        void checkEvolvedModel(EntityModel model,
                               Environment env,
                               boolean oldTypesExist) {
            checkNonEntity(false, model, env, NAME, 0);
            checkEntity(false, model, env, NAME2, 0, null);
            if (oldTypesExist) {
                checkVersions(model, NAME, 0);
                checkVersions(model, NAME2, 0);
            }
        }

        @Override
        void readObjects(EntityStore store, boolean doUpdate) {
            try {
                store.getPrimaryIndex
                    (Integer.class,
                     DeletedPersist4_AnnotRemoved_WithDeleter.class);
                TestCase.fail();
            } catch (Exception e) {
                checkEquals
                    ("java.lang.IllegalArgumentException: Class could not be loaded or is not an entity class: com.sleepycat.persist.test.EvolveClasses$DeletedPersist4_AnnotRemoved_WithDeleter",
                     e.toString());
            }
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                return;
            }

            RawType embedType = store.getModel().getRawType(NAME);
            checkRawType(embedType, NAME, 0);

            RawObject embed =
                new RawObject(embedType, makeValues("f", 123), null);

            RawObject obj = readRaw(store, 99, NAME2, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "embed", embed);
        }
    }

    @Entity(version=1)
    static class DeletedPersist5_PersistToEntity {

        @PrimaryKey
        int key = 99;

        int f = 123;
    }

    /**
     * Disallow changing the Entity annotation to Persistent when no Deleter
     * mutation is specified, even when the Entity class that embedded the
     * Persistent class is deleted properly (by removing the Entity annotation
     * in this case).
     */
    static class DeletedPersist5_PersistToEntity_NoMutation
        extends EvolveCase {

        private static final String NAME =
            DeletedPersist5_PersistToEntity.class.getName();
        private static final String NAME2 =
            DeletedPersist5_PersistToEntity_NoMutation.class.getName();

        @Override
        Mutations getMutations() {
            Mutations m = new Mutations();
            m.addDeleter(new Deleter(NAME2, 0));
            return m;
        }

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DeletedPersist5_PersistToEntity version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DeletedPersist5_PersistToEntity version: 1 Error: @Entity switched to/from @Persistent";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkNonEntity(true, model, env, NAME, 0);
            checkEntity(true, model, env, NAME2, 0, null);
            checkVersions(model, NAME, 0);
            checkVersions(model, NAME2, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }

            RawType embedType = store.getModel().getRawType(NAME);
            checkRawType(embedType, NAME, 0);

            RawObject embed =
                new RawObject(embedType, makeValues("f", 123), null);

            RawObject obj = readRaw(store, 99, NAME2, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "embed", embed);
        }
    }

    @Entity(version=1)
    static class DeletedPersist6_PersistToEntity {

        @PrimaryKey
        int key = 99;

        int f = 123;
    }

    /**
     * Allow changing the Entity annotation to Persistent when a Deleter
     * mutation is specified, and the Entity class that embedded the Persistent
     * class is also be deleted properly (by removing the Entity annotation in
     * this case).
     */
    static class DeletedPersist6_PersistToEntity_WithDeleter
        extends EvolveCase {

        private static final String NAME =
            DeletedPersist6_PersistToEntity.class.getName();
        private static final String NAME2 =
            DeletedPersist6_PersistToEntity_WithDeleter.class.getName();

        @Override
        int getNRecordsExpected() {
            return 0;
        }

        @Override
        Mutations getMutations() {
            Mutations m = new Mutations();
            m.addDeleter(new Deleter(NAME, 0));
            m.addDeleter(new Deleter(NAME2, 0));
            return m;
        }

        @Override
        void checkEvolvedModel(EntityModel model,
                               Environment env,
                               boolean oldTypesExist) {
            checkEntity(false, model, env, NAME2, 0, null);
            if (oldTypesExist) {
                checkVersions(model, NAME, 1, NAME, 0);
                checkVersions(model, NAME2, 0);
            } else {
                checkVersions(model, NAME, 1);
            }
        }

        @Override
        void readObjects(EntityStore store, boolean doUpdate)
            throws DatabaseException {

            /* Cannot get the primary index for the former entity class. */
            try {
                store.getPrimaryIndex
                    (Integer.class,
                     DeletedPersist6_PersistToEntity_WithDeleter.class);
                TestCase.fail();
            } catch (Exception e) {
                checkEquals
                    ("java.lang.IllegalArgumentException: Class could not be loaded or is not an entity class: com.sleepycat.persist.test.EvolveClasses$DeletedPersist6_PersistToEntity_WithDeleter",
                     e.toString());
            }

            /* Can use the primary index of the now entity class. */
            PrimaryIndex<Integer,
                         DeletedPersist6_PersistToEntity>
                index = store.getPrimaryIndex
                    (Integer.class,
                     DeletedPersist6_PersistToEntity.class);

            if (doUpdate) {
                DeletedPersist6_PersistToEntity obj =
                    new DeletedPersist6_PersistToEntity();
                index.put(obj);
                obj = index.get(obj.key);
                /* This new type should exist only after update. */
                Environment env = store.getEnvironment();
                EntityModel model = store.getModel();
                checkEntity(true, model, env, NAME, 1, null);
            }
        }

        @Override
        void copyRawObjects(RawStore rawStore, EntityStore newStore)
            throws DatabaseException {

            PrimaryIndex<Integer,
                         DeletedPersist6_PersistToEntity>
                index = newStore.getPrimaryIndex
                    (Integer.class,
                     DeletedPersist6_PersistToEntity.class);
            RawObject raw = rawStore.getPrimaryIndex(NAME).get(99);
            index.put((DeletedPersist6_PersistToEntity)
                      newStore.getModel().convertRawObject(raw));
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                return;
            }

            RawType embedType = store.getModel().getRawType(NAME);
            checkRawType(embedType, NAME, 0);

            RawObject embed =
                new RawObject(embedType, makeValues("f", 123), null);

            RawObject obj = readRaw(store, 99, NAME2, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "embed", embed);
        }
    }

    /**
     * Disallow renaming an entity class without a Renamer mutation.
     */
    @Entity(version=1)
    static class RenamedEntity1_NewEntityName_NoMutation
        extends EvolveCase {

        private static final String NAME =
            PREFIX + "RenamedEntity1_NewEntityName";
        private static final String NAME2 =
            RenamedEntity1_NewEntityName_NoMutation.class.getName();

        @PrimaryKey
        int key = 99;

        @SecondaryKey(relate=ONE_TO_ONE)
        int skey = 88;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$RenamedEntity1_NewEntityName version: 0 Error: java.lang.ClassNotFoundException: com.sleepycat.persist.test.EvolveClasses$RenamedEntity1_NewEntityName";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, "skey");
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "skey", 88);
        }
    }

    /**
     * Allow renaming an entity class with a Renamer mutation.
     */
    @Entity(version=1)
    static class RenamedEntity2_NewEntityName_WithRenamer
        extends EvolveCase {

        private static final String NAME =
            PREFIX + "RenamedEntity2_NewEntityName";
        private static final String NAME2 =
            RenamedEntity2_NewEntityName_WithRenamer.class.getName();

        @PrimaryKey
        int key = 99;

        @SecondaryKey(relate=ONE_TO_ONE)
        int skey = 88;

        @Override
        Mutations getMutations() {
            Mutations m = new Mutations();
            m.addRenamer(new Renamer(NAME, 0, NAME2));
            return m;
        }

        @Override
        void checkEvolvedModel(EntityModel model,
                               Environment env,
                               boolean oldTypesExist) {
            checkEntity(false, model, env, NAME, 0, null);
            checkEntity(true, model, env, NAME2, 1, null);
            if (oldTypesExist) {
                checkVersions(model, NAME2, 1, NAME, 0);
            } else {
                checkVersions(model, NAME2, 1);
            }
        }

        @Override
        void readObjects(EntityStore store, boolean doUpdate)
            throws DatabaseException {

            PrimaryIndex<Integer,RenamedEntity2_NewEntityName_WithRenamer>
                index = store.getPrimaryIndex
                    (Integer.class,
                     RenamedEntity2_NewEntityName_WithRenamer.class);
            RenamedEntity2_NewEntityName_WithRenamer obj = index.get(key);
            TestCase.assertNotNull(obj);
            TestCase.assertEquals(99, obj.key);
            TestCase.assertEquals(88, obj.skey);

            SecondaryIndex<Integer,Integer,
                           RenamedEntity2_NewEntityName_WithRenamer>
                sindex = store.getSecondaryIndex(index, Integer.class, "skey");
            obj = sindex.get(88);
            TestCase.assertNotNull(obj);
            TestCase.assertEquals(99, obj.key);
            TestCase.assertEquals(88, obj.skey);

            if (doUpdate) {
                index.put(obj);
            }
        }

        @Override
        void copyRawObjects(RawStore rawStore, EntityStore newStore)
            throws DatabaseException {

            PrimaryIndex<Integer,RenamedEntity2_NewEntityName_WithRenamer>
                index = newStore.getPrimaryIndex
                    (Integer.class,
                     RenamedEntity2_NewEntityName_WithRenamer.class);
            RawObject raw = rawStore.getPrimaryIndex(NAME2).get(99);
            index.put((RenamedEntity2_NewEntityName_WithRenamer)
                      newStore.getModel().convertRawObject(raw));
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            RawObject obj;
            if (expectEvolved) {
                obj = readRaw(store, 99, NAME2, 1, CASECLS, 0);
            } else {
                obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            }
            checkRawFields(obj, "key", 99, "skey", 88);
        }
    }

    @Persistent
    static class DeleteSuperclass1_BaseClass
        extends EvolveCase {

        int f = 123;
    }

    /**
     * Disallow deleting a superclass from the hierarchy when the superclass
     * has persistent fields and no Deleter or Converter is specified.
     */
    @Entity
    static class DeleteSuperclass1_NoMutation
        extends EvolveCase {

        private static final String NAME =
            DeleteSuperclass1_BaseClass.class.getName();
        private static final String NAME2 =
            DeleteSuperclass1_NoMutation.class.getName();

        @PrimaryKey
        int key = 99;

        int ff;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DeleteSuperclass1_NoMutation version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DeleteSuperclass1_NoMutation version: 0 Error: When a superclass is removed from the class hierarchy, the superclass or all of its persistent fields must be deleted with a Deleter: com.sleepycat.persist.test.EvolveClasses$DeleteSuperclass1_BaseClass";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkNonEntity(true, model, env, NAME, 0);
            checkEntity(true, model, env, NAME2, 0, null);
            checkVersions(model, NAME, 0);
            checkVersions(model, NAME2, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME2, 0, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", 88);
            checkRawFields(obj.getSuper(), "f", 123);
            checkRawFields(obj.getSuper().getSuper());
        }
    }

    @Persistent
    static class DeleteSuperclass2_BaseClass
        extends EvolveCase {

        int f;

        @SecondaryKey(relate=ONE_TO_ONE)
        int skey;
    }

    /**
     * Allow deleting a superclass from the hierarchy when the superclass has
     * persistent fields and a class Converter is specified.  Also check that
     * the secondary key field in the deleted base class is handled properly.
     */
    @Entity(version=1)
    static class DeleteSuperclass2_WithConverter extends EvolveCase {

        private static final String NAME =
            DeleteSuperclass2_BaseClass.class.getName();
        private static final String NAME2 =
            DeleteSuperclass2_WithConverter.class.getName();

        @PrimaryKey
        int key;

        int ff;

        @SecondaryKey(relate=ONE_TO_ONE)
        Integer skey2;

        @SecondaryKey(relate=ONE_TO_ONE)
        int skey3;

        @Override
        Mutations getMutations() {
            Mutations m = new Mutations();
            m.addConverter(new EntityConverter
                (NAME2, 0, new MyConversion(),
                 Collections.singleton("skey")));
            return m;
        }

        @SuppressWarnings("serial")
        static class MyConversion implements Conversion {

            transient RawType newType;

            public void initialize(EntityModel model) {
                newType = model.getRawType(NAME2);
                TestCase.assertNotNull(newType);
            }

            public Object convert(Object fromValue) {
                TestCase.assertNotNull(newType);
                RawObject obj = (RawObject) fromValue;
                RawObject newSuper = obj.getSuper().getSuper();
                return new RawObject(newType, obj.getValues(), newSuper);
            }

            @Override
            public boolean equals(Object other) {
                return other instanceof MyConversion;
            }
        }

        @Override
        void checkEvolvedModel(EntityModel model,
                               Environment env,
                               boolean oldTypesExist) {
            checkEntity(true, model, env, NAME2, 1, null);
            if (oldTypesExist) {
                checkVersions(model, NAME2, 1, NAME2, 0);
                checkNonEntity(true, model, env, NAME, 0);
                checkVersions(model, NAME, 0);
            } else {
                checkVersions(model, NAME2, 1);
            }
        }

        @Override
        void readObjects(EntityStore store, boolean doUpdate)
            throws DatabaseException {

            PrimaryIndex<Integer,DeleteSuperclass2_WithConverter>
                index = store.getPrimaryIndex
                    (Integer.class,
                     DeleteSuperclass2_WithConverter.class);
            DeleteSuperclass2_WithConverter obj = index.get(99);
            TestCase.assertNotNull(obj);
            TestCase.assertSame
                (EvolveCase.class, obj.getClass().getSuperclass());
            TestCase.assertEquals(99, obj.key);
            TestCase.assertEquals(88, obj.ff);
            TestCase.assertEquals(Integer.valueOf(77), obj.skey2);
            TestCase.assertEquals(66, obj.skey3);
            if (doUpdate) {
                index.put(obj);
            }
        }

        @Override
        void copyRawObjects(RawStore rawStore, EntityStore newStore)
            throws DatabaseException {

            PrimaryIndex<Integer,DeleteSuperclass2_WithConverter>
                index = newStore.getPrimaryIndex
                    (Integer.class,
                     DeleteSuperclass2_WithConverter.class);
            RawObject raw = rawStore.getPrimaryIndex(NAME2).get(99);
            index.put((DeleteSuperclass2_WithConverter)
                      newStore.getModel().convertRawObject(raw));
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            RawObject obj;
            if (expectEvolved) {
                obj = readRaw(store, 99, NAME2, 1, CASECLS, 0);
            } else {
                obj = readRaw(store, 99, NAME2, 0, NAME, 0, CASECLS, 0);
            }
            checkRawFields
                (obj, "key", 99, "ff", 88, "skey2", 77, "skey3", 66);
            if (expectEvolved) {
                checkRawFields(obj.getSuper());
            } else {
                checkRawFields(obj.getSuper(), "f", 123, "skey", 456);
                checkRawFields(obj.getSuper().getSuper());
            }
            Environment env = store.getEnvironment();
            assertDbExists(!expectEvolved, env, NAME2, "skey");
            assertDbExists(true, env, NAME2, "skey3");
        }
    }

    static class DeleteSuperclass3_BaseClass
        extends EvolveCase {

        int f;

        @SecondaryKey(relate=ONE_TO_ONE)
        int skey;
    }

    /**
     * Allow deleting a superclass from the hierarchy when the superclass
     * has persistent fields and a class Deleter is specified.  Also check that
     * the secondary key field in the deleted base class is handled properly.
     */
    @Entity(version=1)
    static class DeleteSuperclass3_WithDeleter extends EvolveCase {

        private static final String NAME =
            DeleteSuperclass3_BaseClass.class.getName();
        private static final String NAME2 =
            DeleteSuperclass3_WithDeleter.class.getName();

        @PrimaryKey
        int key;

        int ff;

        @Override
        Mutations getMutations() {
            Mutations m = new Mutations();
            m.addDeleter(new Deleter(NAME, 0));
            return m;
        }

        @Override
        void checkEvolvedModel(EntityModel model,
                               Environment env,
                               boolean oldTypesExist) {
            checkEntity(true, model, env, NAME2, 1, null);
            if (oldTypesExist) {
                checkVersions(model, NAME2, 1, NAME2, 0);
                checkNonEntity(false, model, env, NAME, 0);
                checkVersions(model, NAME, 0);
            } else {
                checkVersions(model, NAME2, 1);
            }
        }

        @Override
        void readObjects(EntityStore store, boolean doUpdate)
            throws DatabaseException {

            PrimaryIndex<Integer,DeleteSuperclass3_WithDeleter>
                index = store.getPrimaryIndex
                    (Integer.class,
                     DeleteSuperclass3_WithDeleter.class);
            DeleteSuperclass3_WithDeleter obj = index.get(99);
            TestCase.assertNotNull(obj);
            TestCase.assertSame
                (EvolveCase.class, obj.getClass().getSuperclass());
            TestCase.assertEquals(99, obj.key);
            TestCase.assertEquals(88, obj.ff);
            if (doUpdate) {
                index.put(obj);
            }
        }

        @Override
        void copyRawObjects(RawStore rawStore, EntityStore newStore)
            throws DatabaseException {

            PrimaryIndex<Integer,DeleteSuperclass3_WithDeleter>
                index = newStore.getPrimaryIndex
                    (Integer.class,
                     DeleteSuperclass3_WithDeleter.class);
            RawObject raw = rawStore.getPrimaryIndex(NAME2).get(99);
            index.put((DeleteSuperclass3_WithDeleter)
                      newStore.getModel().convertRawObject(raw));
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            RawObject obj;
            if (expectEvolved) {
                obj = readRaw(store, 99, NAME2, 1, CASECLS, 0);
            } else {
                obj = readRaw(store, 99, NAME2, 0, NAME, 0, CASECLS, 0);
            }
            checkRawFields(obj, "key", 99, "ff", 88);
            if (expectEvolved) {
                checkRawFields(obj.getSuper());
            } else {
                checkRawFields(obj.getSuper(), "f", 123, "skey", 456);
                checkRawFields(obj.getSuper().getSuper());
            }
            Environment env = store.getEnvironment();
            assertDbExists(!expectEvolved, env, NAME2, "skey");
        }
    }

    @Persistent
    static class DeleteSuperclass4_BaseClass
        extends EvolveCase {
    }

    /**
     * Allow deleting a superclass from the hierarchy when the superclass
     * has NO persistent fields.  No mutations are needed.
     */
    @Entity(version=1)
    static class DeleteSuperclass4_NoFields extends EvolveCase {

        private static final String NAME =
            DeleteSuperclass4_BaseClass.class.getName();
        private static final String NAME2 =
            DeleteSuperclass4_NoFields.class.getName();

        @PrimaryKey
        int key = 99;

        int ff;

        @Override
        void checkEvolvedModel(EntityModel model,
                               Environment env,
                               boolean oldTypesExist) {
            checkEntity(true, model, env, NAME2, 1, null);
            if (oldTypesExist) {
                checkVersions(model, NAME2, 1, NAME2, 0);
                checkNonEntity(true, model, env, NAME, 0);
                checkVersions(model, NAME, 0);
            } else {
                checkVersions(model, NAME2, 1);
            }
        }

        @Override
        void readObjects(EntityStore store, boolean doUpdate)
            throws DatabaseException {

            PrimaryIndex<Integer,DeleteSuperclass4_NoFields>
                index = store.getPrimaryIndex
                    (Integer.class,
                     DeleteSuperclass4_NoFields.class);
            DeleteSuperclass4_NoFields obj = index.get(key);
            TestCase.assertNotNull(obj);
            TestCase.assertSame
                (EvolveCase.class, obj.getClass().getSuperclass());
            TestCase.assertEquals(99, obj.key);
            TestCase.assertEquals(88, obj.ff);
            if (doUpdate) {
                index.put(obj);
            }
        }

        @Override
        void copyRawObjects(RawStore rawStore, EntityStore newStore)
            throws DatabaseException {

            PrimaryIndex<Integer,DeleteSuperclass4_NoFields>
                index = newStore.getPrimaryIndex
                    (Integer.class,
                     DeleteSuperclass4_NoFields.class);
            RawObject raw = rawStore.getPrimaryIndex(NAME2).get(99);
            index.put((DeleteSuperclass4_NoFields)
                      newStore.getModel().convertRawObject(raw));
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            RawObject obj;
            if (expectEvolved) {
                obj = readRaw(store, 99, NAME2, 1, CASECLS, 0);
            } else {
                obj = readRaw(store, 99, NAME2, 0, NAME, 0, CASECLS, 0);
            }
            checkRawFields(obj, "key", 99, "ff", 88);
            checkRawFields(obj.getSuper());
            if (expectEvolved) {
                TestCase.assertNull(obj.getSuper().getSuper());
            } else {
                checkRawFields(obj.getSuper().getSuper());
            }
        }
    }

    @Persistent(version=1)
    static class DeleteSuperclass5_Embedded {

        int f;

        @Override
        public String toString() {
            return "" + f;
        }
    }

    /**
     * Ensure that a superclass at the top of the hierarchy can be deleted.  A
     * class Deleter is used.
     */
    @Entity
    static class DeleteSuperclass5_Top
        extends EvolveCase {

        private static final String NAME =
            DeleteSuperclass5_Top.class.getName();
        private static final String NAME2 =
            DeleteSuperclass5_Embedded.class.getName();
        private static final String NAME3 =
            PREFIX + "DeleteSuperclass5_Embedded_Base";

        @PrimaryKey
        int key = 99;

        int ff;

        DeleteSuperclass5_Embedded embed =
            new DeleteSuperclass5_Embedded();

        @Override
        Mutations getMutations() {
            Mutations m = new Mutations();
            m.addDeleter(new Deleter(NAME3, 0));
            return m;
        }

        @Override
        void checkEvolvedModel(EntityModel model,
                               Environment env,
                               boolean oldTypesExist) {
            checkEntity(true, model, env, NAME, 0, null);
            checkNonEntity(true, model, env, NAME2, 1);
            checkNonEntity(false, model, env, NAME3, 0);
            checkVersions(model, NAME, 0);
            if (oldTypesExist) {
                checkVersions(model, NAME2, 1, NAME2, 0);
                checkVersions(model, NAME3, 0);
            } else {
                checkVersions(model, NAME2, 1);
            }
        }

        @Override
        void readObjects(EntityStore store, boolean doUpdate)
            throws DatabaseException {

            PrimaryIndex<Integer,DeleteSuperclass5_Top>
                index = store.getPrimaryIndex
                    (Integer.class,
                     DeleteSuperclass5_Top.class);
            DeleteSuperclass5_Top obj = index.get(key);
            TestCase.assertNotNull(obj);
            TestCase.assertNotNull(obj.embed);
            TestCase.assertEquals(99, obj.key);
            TestCase.assertEquals(88, obj.ff);
            TestCase.assertEquals(123, obj.embed.f);
            if (doUpdate) {
                index.put(obj);
            }
        }

        @Override
        void copyRawObjects(RawStore rawStore, EntityStore newStore)
            throws DatabaseException {

            PrimaryIndex<Integer,DeleteSuperclass5_Top>
                index = newStore.getPrimaryIndex
                    (Integer.class,
                     DeleteSuperclass5_Top.class);
            RawObject raw = rawStore.getPrimaryIndex(NAME).get(99);
            index.put((DeleteSuperclass5_Top)
                      newStore.getModel().convertRawObject(raw));
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            RawType embedType = store.getModel().getRawType(NAME2);
            RawObject embedSuper = null;
            if (!expectEvolved) {
                RawType embedSuperType = store.getModel().getRawType(NAME3);
                embedSuper = new RawObject
                    (embedSuperType, makeValues("g", 456), null);
            }
            RawObject embed =
                new RawObject(embedType, makeValues("f", 123), embedSuper);
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", 88, "embed", embed);
        }
    }

    @Persistent
    static class InsertSuperclass1_BaseClass
        extends EvolveCase {

        int f = 123;
    }

    /**
     * Allow inserting a superclass between two existing classes in the
     * hierarchy.  No mutations are needed.
     */
    @Entity(version=1)
    static class InsertSuperclass1_Between
        extends InsertSuperclass1_BaseClass {

        private static final String NAME =
            InsertSuperclass1_BaseClass.class.getName();
        private static final String NAME2 =
            InsertSuperclass1_Between.class.getName();

        @PrimaryKey
        int key = 99;

        int ff;

        @Override
        void checkEvolvedModel(EntityModel model,
                               Environment env,
                               boolean oldTypesExist) {
            checkNonEntity(true, model, env, NAME, 0);
            checkEntity(true, model, env, NAME2, 1, null);
            checkVersions(model, NAME, 0);
            if (oldTypesExist) {
                checkVersions(model, NAME2, 1, NAME2, 0);
            } else {
                checkVersions(model, NAME2, 1);
            }
        }

        @Override
        void readObjects(EntityStore store, boolean doUpdate)
            throws DatabaseException {

            PrimaryIndex<Integer,InsertSuperclass1_Between>
                index = store.getPrimaryIndex
                    (Integer.class,
                     InsertSuperclass1_Between.class);
            InsertSuperclass1_Between obj = index.get(key);
            TestCase.assertNotNull(obj);
            TestCase.assertSame
                (InsertSuperclass1_BaseClass.class,
                 obj.getClass().getSuperclass());
            TestCase.assertSame
                (EvolveCase.class,
                 obj.getClass().getSuperclass().getSuperclass());
            TestCase.assertEquals(99, obj.key);
            TestCase.assertEquals(88, obj.ff);
            TestCase.assertEquals(123, obj.f);
            if (doUpdate) {
                index.put(obj);
            }
        }

        @Override
        void copyRawObjects(RawStore rawStore, EntityStore newStore)
            throws DatabaseException {

            PrimaryIndex<Integer,InsertSuperclass1_Between>
                index = newStore.getPrimaryIndex
                    (Integer.class,
                     InsertSuperclass1_Between.class);
            RawObject raw = rawStore.getPrimaryIndex(NAME2).get(99);
            index.put((InsertSuperclass1_Between)
                      newStore.getModel().convertRawObject(raw));
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            RawObject obj;
            if (expectEvolved) {
                obj = readRaw(store, 99, NAME2, 1, NAME, 0, CASECLS, 0);
            } else {
                obj = readRaw(store, 99, NAME2, 0, CASECLS, 0);
            }
            checkRawFields(obj, "key", 99, "ff", 88);
            if (expectEvolved) {
                if (expectUpdated) {
                    checkRawFields(obj.getSuper(), "f", 123);
                } else {
                    checkRawFields(obj.getSuper());
                }
                checkRawFields(obj.getSuper().getSuper());
                TestCase.assertNull(obj.getSuper().getSuper().getSuper());
            } else {
                checkRawFields(obj.getSuper());
                TestCase.assertNull(obj.getSuper().getSuper());
            }
        }
    }

    @Persistent
    static class InsertSuperclass2_Embedded_Base {

        int g = 456;
    }

    @Persistent(version=1)
    static class InsertSuperclass2_Embedded
        extends InsertSuperclass2_Embedded_Base  {

        int f;
    }

    /**
     * Allow inserting a superclass at the top of the hierarchy.  No mutations
     * are needed.
     */
    @Entity
    static class InsertSuperclass2_Top
        extends EvolveCase {

        private static final String NAME =
            InsertSuperclass2_Top.class.getName();
        private static final String NAME2 =
            InsertSuperclass2_Embedded.class.getName();
        private static final String NAME3 =
            InsertSuperclass2_Embedded_Base.class.getName();

        @PrimaryKey
        int key = 99;

        int ff;

        InsertSuperclass2_Embedded embed =
            new InsertSuperclass2_Embedded();

        @Override
        void checkEvolvedModel(EntityModel model,
                               Environment env,
                               boolean oldTypesExist) {
            checkEntity(true, model, env, NAME, 0, null);
            checkNonEntity(true, model, env, NAME2, 1);
            checkNonEntity(true, model, env, NAME3, 0);
            checkVersions(model, NAME, 0);
            if (oldTypesExist) {
                checkVersions(model, NAME2, 1, NAME2, 0);
            } else {
                checkVersions(model, NAME2, 1);
            }
            checkVersions(model, NAME3, 0);
        }

        @Override
        void readObjects(EntityStore store, boolean doUpdate)
            throws DatabaseException {

            PrimaryIndex<Integer,InsertSuperclass2_Top>
                index = store.getPrimaryIndex
                    (Integer.class,
                     InsertSuperclass2_Top.class);
            InsertSuperclass2_Top obj = index.get(key);
            TestCase.assertNotNull(obj);
            TestCase.assertNotNull(obj.embed);
            TestCase.assertEquals(99, obj.key);
            TestCase.assertEquals(88, obj.ff);
            TestCase.assertEquals(123, obj.embed.f);
            TestCase.assertEquals(456, obj.embed.g);
            if (doUpdate) {
                index.put(obj);
            }
        }

        @Override
        void copyRawObjects(RawStore rawStore, EntityStore newStore)
            throws DatabaseException {

            PrimaryIndex<Integer,InsertSuperclass2_Top>
                index = newStore.getPrimaryIndex
                    (Integer.class,
                     InsertSuperclass2_Top.class);
            RawObject raw = rawStore.getPrimaryIndex(NAME).get(99);
            index.put((InsertSuperclass2_Top)
                      newStore.getModel().convertRawObject(raw));
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            RawType embedType = store.getModel().getRawType(NAME2);
            RawObject embedSuper = null;
            if (expectEvolved) {
                RawType embedSuperType = store.getModel().getRawType(NAME3);
                Map<String,Object> values =
                    expectUpdated ? makeValues("g", 456) : makeValues();
                embedSuper = new RawObject(embedSuperType, values, null);
            }
            RawObject embed =
                new RawObject(embedType, makeValues("f", 123), embedSuper);
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", 88, "embed", embed);
        }
    }

    @Entity(version=1)
    static class DisallowNonKeyField_PrimitiveToObject
        extends EvolveCase {

        private static final String NAME =
            DisallowNonKeyField_PrimitiveToObject.class.getName();

        @PrimaryKey
        int key = 99;

        String ff;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_PrimitiveToObject version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_PrimitiveToObject version: 1 Error: Old field type: int is not compatible with the new type: java.lang.String for field: ff";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", 88);
        }
    }

    @Entity(version=1)
    static class DisallowNonKeyField_ObjectToPrimitive
        extends EvolveCase {

        private static final String NAME =
            DisallowNonKeyField_ObjectToPrimitive.class.getName();

        @PrimaryKey
        int key = 99;

        int ff;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_ObjectToPrimitive version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_ObjectToPrimitive version: 1 Error: Old field type: java.lang.String is not compatible with the new type: int for field: ff";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", "88");
        }
    }

    @Persistent
    static class MyType {

        @Override
        public boolean equals(Object o) {
            return o instanceof MyType;
        }
    }

    @Persistent
    static class MySubtype extends MyType {

        @Override
        public boolean equals(Object o) {
            return o instanceof MySubtype;
        }
    }

    @Entity(version=1)
    static class DisallowNonKeyField_ObjectToSubtype
        extends EvolveCase {

        private static final String NAME =
            DisallowNonKeyField_ObjectToSubtype.class.getName();

        @PrimaryKey
        int key = 99;

        MySubtype ff;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_ObjectToSubtype version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_ObjectToSubtype version: 1 Error: Old field type: com.sleepycat.persist.test.EvolveClasses$MyType is not compatible with the new type: com.sleepycat.persist.test.EvolveClasses$MySubtype for field: ff";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawType embedType = store.getModel().getRawType
                (MyType.class.getName());
            RawObject embed = new RawObject(embedType, makeValues(), null);

            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", embed);
        }
    }

    @Entity(version=1)
    static class DisallowNonKeyField_ObjectToUnrelatedSimple
        extends EvolveCase {

        private static final String NAME =
            DisallowNonKeyField_ObjectToUnrelatedSimple.class.getName();

        @PrimaryKey
        int key = 99;

        String ff;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_ObjectToUnrelatedSimple version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_ObjectToUnrelatedSimple version: 1 Error: Old field type: java.lang.Integer is not compatible with the new type: java.lang.String for field: ff";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", 88);
        }
    }

    @Entity(version=1)
    static class DisallowNonKeyField_ObjectToUnrelatedOther
        extends EvolveCase {

        private static final String NAME =
            DisallowNonKeyField_ObjectToUnrelatedOther.class.getName();

        @PrimaryKey
        int key = 99;

        MyType ff;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_ObjectToUnrelatedOther version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_ObjectToUnrelatedOther version: 1 Error: Old field type: java.lang.Integer is not compatible with the new type: com.sleepycat.persist.test.EvolveClasses$MyType for field: ff";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", 88);
        }
    }

    @Entity(version=1)
    static class DisallowNonKeyField_byte2boolean
        extends EvolveCase {

        private static final String NAME =
            DisallowNonKeyField_byte2boolean.class.getName();

        @PrimaryKey
        int key = 99;

        boolean ff;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_byte2boolean version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_byte2boolean version: 1 Error: Old field type: byte is not compatible with the new type: boolean for field: ff";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", (byte) 88);
        }
    }

    @Entity(version=1)
    static class DisallowNonKeyField_short2byte
        extends EvolveCase {

        private static final String NAME =
            DisallowNonKeyField_short2byte.class.getName();

        @PrimaryKey
        int key = 99;

        byte ff;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_short2byte version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_short2byte version: 1 Error: Old field type: short is not compatible with the new type: byte for field: ff";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", (short) 88);
        }
    }

    @Entity(version=1)
    static class DisallowNonKeyField_int2short
        extends EvolveCase {

        private static final String NAME =
            DisallowNonKeyField_int2short.class.getName();

        @PrimaryKey
        int key = 99;

        short ff;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_int2short version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_int2short version: 1 Error: Old field type: int is not compatible with the new type: short for field: ff";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", 88);
        }
    }

    @Entity(version=1)
    static class DisallowNonKeyField_long2int
        extends EvolveCase {

        private static final String NAME =
            DisallowNonKeyField_long2int.class.getName();

        @PrimaryKey
        int key = 99;

        int ff;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_long2int version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_long2int version: 1 Error: Old field type: long is not compatible with the new type: int for field: ff";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", (long) 88);
        }
    }

    @Entity(version=1)
    static class DisallowNonKeyField_float2long
        extends EvolveCase {

        private static final String NAME =
            DisallowNonKeyField_float2long.class.getName();

        @PrimaryKey
        int key = 99;

        long ff;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_float2long version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_float2long version: 1 Error: Old field type: float is not compatible with the new type: long for field: ff";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", (float) 88);
        }
    }

    @Entity(version=1)
    static class DisallowNonKeyField_double2float
        extends EvolveCase {

        private static final String NAME =
            DisallowNonKeyField_double2float.class.getName();

        @PrimaryKey
        int key = 99;

        float ff;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_double2float version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_double2float version: 1 Error: Old field type: double is not compatible with the new type: float for field: ff";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", (double) 88);
        }
    }

    @Entity(version=1)
    static class DisallowNonKeyField_Byte2byte
        extends EvolveCase {

        private static final String NAME =
            DisallowNonKeyField_Byte2byte.class.getName();

        @PrimaryKey
        int key = 99;

        byte ff;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_Byte2byte version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_Byte2byte version: 1 Error: Old field type: java.lang.Byte is not compatible with the new type: byte for field: ff";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", (byte) 88);
        }
    }

    @Entity(version=1)
    static class DisallowNonKeyField_Character2char
        extends EvolveCase {

        private static final String NAME =
            DisallowNonKeyField_Character2char.class.getName();

        @PrimaryKey
        int key = 99;

        char ff;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_Character2char version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_Character2char version: 1 Error: Old field type: java.lang.Character is not compatible with the new type: char for field: ff";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", (char) 88);
        }
    }

    @Entity(version=1)
    static class DisallowNonKeyField_Short2short
        extends EvolveCase {

        private static final String NAME =
            DisallowNonKeyField_Short2short.class.getName();

        @PrimaryKey
        int key = 99;

        short ff;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_Short2short version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_Short2short version: 1 Error: Old field type: java.lang.Short is not compatible with the new type: short for field: ff";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", (short) 88);
        }
    }

    @Entity(version=1)
    static class DisallowNonKeyField_Integer2int
        extends EvolveCase {

        private static final String NAME =
            DisallowNonKeyField_Integer2int.class.getName();

        @PrimaryKey
        int key = 99;

        int ff;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_Integer2int version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_Integer2int version: 1 Error: Old field type: java.lang.Integer is not compatible with the new type: int for field: ff";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", 88);
        }
    }

    @Entity(version=1)
    static class DisallowNonKeyField_Long2long
        extends EvolveCase {

        private static final String NAME =
            DisallowNonKeyField_Long2long.class.getName();

        @PrimaryKey
        int key = 99;

        long ff;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_Long2long version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_Long2long version: 1 Error: Old field type: java.lang.Long is not compatible with the new type: long for field: ff";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", (long) 88);
        }
    }

    @Entity(version=1)
    static class DisallowNonKeyField_Float2float
        extends EvolveCase {

        private static final String NAME =
            DisallowNonKeyField_Float2float.class.getName();

        @PrimaryKey
        int key = 99;

        float ff;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_Float2float version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_Float2float version: 1 Error: Old field type: java.lang.Float is not compatible with the new type: float for field: ff";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", (float) 88);
        }
    }

    @Entity(version=1)
    static class DisallowNonKeyField_Double2double
        extends EvolveCase {

        private static final String NAME =
            DisallowNonKeyField_Double2double.class.getName();

        @PrimaryKey
        int key = 99;

        double ff;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_Double2double version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_Double2double version: 1 Error: Old field type: java.lang.Double is not compatible with the new type: double for field: ff";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", (double) 88);
        }
    }

    @Entity(version=1)
    static class DisallowNonKeyField_float2BigInt
        extends EvolveCase {

        private static final String NAME =
            DisallowNonKeyField_float2BigInt.class.getName();

        @PrimaryKey
        int key = 99;

        BigInteger ff;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_float2BigInt version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_float2BigInt version: 1 Error: Old field type: float is not compatible with the new type: java.math.BigInteger for field: ff";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", (float) 88);
        }
    }

    @Entity(version=1)
    static class DisallowNonKeyField_BigInt2long
        extends EvolveCase {

        private static final String NAME =
            DisallowNonKeyField_BigInt2long.class.getName();

        @PrimaryKey
        int key = 99;

        long ff;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_BigInt2long version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowNonKeyField_BigInt2long version: 1 Error: Old field type: java.math.BigInteger is not compatible with the new type: long for field: ff";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", BigInteger.valueOf(88));
        }
    }

    @Entity(version=1)
    static class DisallowSecKeyField_byte2short
        extends EvolveCase {

        private static final String NAME =
            DisallowSecKeyField_byte2short.class.getName();

        @PrimaryKey
        int key = 99;

        @SecondaryKey(relate=ONE_TO_ONE)
        short ff;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DisallowSecKeyField_byte2short version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowSecKeyField_byte2short version: 1 Error: Old field type: byte is not compatible with the new type: short for field: ff";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, "ff");
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", (byte) 88);
        }
    }

    @Entity(version=1)
    static class DisallowSecKeyField_char2int
        extends EvolveCase {

        private static final String NAME =
            DisallowSecKeyField_char2int.class.getName();

        @PrimaryKey
        int key = 99;

        @SecondaryKey(relate=ONE_TO_ONE)
        int ff;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DisallowSecKeyField_char2int version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowSecKeyField_char2int version: 1 Error: Old field type: char is not compatible with the new type: int for field: ff";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, "ff");
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", (char) 88);
        }
    }

    @Entity(version=1)
    static class DisallowSecKeyField_short2int
        extends EvolveCase {

        private static final String NAME =
            DisallowSecKeyField_short2int.class.getName();

        @PrimaryKey
        int key = 99;

        @SecondaryKey(relate=ONE_TO_ONE)
        int ff;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DisallowSecKeyField_short2int version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowSecKeyField_short2int version: 1 Error: Old field type: short is not compatible with the new type: int for field: ff";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, "ff");
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", (short) 88);
        }
    }

    @Entity(version=1)
    static class DisallowSecKeyField_int2long
        extends EvolveCase {

        private static final String NAME =
            DisallowSecKeyField_int2long.class.getName();

        @PrimaryKey
        int key = 99;

        @SecondaryKey(relate=ONE_TO_ONE)
        long ff;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DisallowSecKeyField_int2long version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowSecKeyField_int2long version: 1 Error: Old field type: int is not compatible with the new type: long for field: ff";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, "ff");
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", 88);
        }
    }

    @Entity(version=1)
    static class DisallowSecKeyField_long2float
        extends EvolveCase {

        private static final String NAME =
            DisallowSecKeyField_long2float.class.getName();

        @PrimaryKey
        int key = 99;

        @SecondaryKey(relate=ONE_TO_ONE)
        float ff;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DisallowSecKeyField_long2float version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowSecKeyField_long2float version: 1 Error: Old field type: long is not compatible with the new type: float for field: ff";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, "ff");
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", (long) 88);
        }
    }

    @Entity(version=1)
    static class DisallowSecKeyField_float2double
        extends EvolveCase {

        private static final String NAME =
            DisallowSecKeyField_float2double.class.getName();

        @PrimaryKey
        int key = 99;

        @SecondaryKey(relate=ONE_TO_ONE)
        double ff;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DisallowSecKeyField_float2double version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowSecKeyField_float2double version: 1 Error: Old field type: float is not compatible with the new type: double for field: ff";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, "ff");
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", (float) 88);
        }
    }

    @Entity(version=1)
    static class DisallowSecKeyField_Byte2short2
        extends EvolveCase {

        private static final String NAME =
            DisallowSecKeyField_Byte2short2.class.getName();

        @PrimaryKey
        int key = 99;

        @SecondaryKey(relate=ONE_TO_ONE)
        short ff;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DisallowSecKeyField_Byte2short2 version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowSecKeyField_Byte2short2 version: 1 Error: Old field type: java.lang.Byte is not compatible with the new type: short for field: ff";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, "ff");
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", (byte) 88);
        }
    }

    @Entity(version=1)
    static class DisallowSecKeyField_Character2int
        extends EvolveCase {

        private static final String NAME =
            DisallowSecKeyField_Character2int.class.getName();

        @PrimaryKey
        int key = 99;

        @SecondaryKey(relate=ONE_TO_ONE)
        int ff;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DisallowSecKeyField_Character2int version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowSecKeyField_Character2int version: 1 Error: Old field type: java.lang.Character is not compatible with the new type: int for field: ff";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, "ff");
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", (char) 88);
        }
    }

    @Entity(version=1)
    static class DisallowSecKeyField_Short2int2
        extends EvolveCase {

        private static final String NAME =
            DisallowSecKeyField_Short2int2.class.getName();

        @PrimaryKey
        int key = 99;

        @SecondaryKey(relate=ONE_TO_ONE)
        int ff;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DisallowSecKeyField_Short2int2 version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowSecKeyField_Short2int2 version: 1 Error: Old field type: java.lang.Short is not compatible with the new type: int for field: ff";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, "ff");
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", (short) 88);
        }
    }

    @Entity(version=1)
    static class DisallowSecKeyField_Integer2long
        extends EvolveCase {

        private static final String NAME =
            DisallowSecKeyField_Integer2long.class.getName();

        @PrimaryKey
        int key = 99;

        @SecondaryKey(relate=ONE_TO_ONE)
        long ff;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DisallowSecKeyField_Integer2long version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowSecKeyField_Integer2long version: 1 Error: Old field type: java.lang.Integer is not compatible with the new type: long for field: ff";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, "ff");
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", 88);
        }
    }

    @Entity(version=1)
    static class DisallowSecKeyField_Long2float2
        extends EvolveCase {

        private static final String NAME =
            DisallowSecKeyField_Long2float2.class.getName();

        @PrimaryKey
        int key = 99;

        @SecondaryKey(relate=ONE_TO_ONE)
        float ff;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DisallowSecKeyField_Long2float2 version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowSecKeyField_Long2float2 version: 1 Error: Old field type: java.lang.Long is not compatible with the new type: float for field: ff";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, "ff");
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", (long) 88);
        }
    }

    @Entity(version=1)
    static class DisallowSecKeyField_Float2double2
        extends EvolveCase {

        private static final String NAME =
            DisallowSecKeyField_Float2double2.class.getName();

        @PrimaryKey
        int key = 99;

        @SecondaryKey(relate=ONE_TO_ONE)
        double ff;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DisallowSecKeyField_Float2double2 version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowSecKeyField_Float2double2 version: 1 Error: Old field type: java.lang.Float is not compatible with the new type: double for field: ff";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, "ff");
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", (float) 88);
        }
    }

    @Entity(version=1)
    static class DisallowSecKeyField_int2BigInt
        extends EvolveCase {

        private static final String NAME =
            DisallowSecKeyField_int2BigInt.class.getName();

        @PrimaryKey
        int key = 99;

        @SecondaryKey(relate=ONE_TO_ONE)
        BigInteger ff;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: com.sleepycat.persist.test.EvolveClasses$DisallowSecKeyField_int2BigInt version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowSecKeyField_int2BigInt version: 1 Error: Old field type: int is not compatible with the new type: java.math.BigInteger for field: ff";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, "ff");
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "ff", 88);
        }
    }

    // ---

    @Entity(version=1)
    static class DisallowPriKeyField_byte2short
        extends EvolveCase {

        private static final String NAME =
            DisallowPriKeyField_byte2short.class.getName();

        @PrimaryKey
        short key;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Type may not be changed for a primary key field or composite key class field when evolving class: com.sleepycat.persist.test.EvolveClasses$DisallowPriKeyField_byte2short version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowPriKeyField_byte2short version: 1 Error: Old field type: byte is not compatible with the new type: short for field: key";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, (byte) 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", (byte) 99);
        }
    }

    @Entity(version=1)
    static class DisallowPriKeyField_char2int
        extends EvolveCase {

        private static final String NAME =
            DisallowPriKeyField_char2int.class.getName();

        @PrimaryKey
        int key;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Type may not be changed for a primary key field or composite key class field when evolving class: com.sleepycat.persist.test.EvolveClasses$DisallowPriKeyField_char2int version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowPriKeyField_char2int version: 1 Error: Old field type: char is not compatible with the new type: int for field: key";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, (char) 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", (char) 99);
        }
    }

    @Entity(version=1)
    static class DisallowPriKeyField_short2int
        extends EvolveCase {

        private static final String NAME =
            DisallowPriKeyField_short2int.class.getName();

        @PrimaryKey
        int key;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Type may not be changed for a primary key field or composite key class field when evolving class: com.sleepycat.persist.test.EvolveClasses$DisallowPriKeyField_short2int version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowPriKeyField_short2int version: 1 Error: Old field type: short is not compatible with the new type: int for field: key";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, (short) 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", (short) 99);
        }
    }

    @Entity(version=1)
    static class DisallowPriKeyField_int2long
        extends EvolveCase {

        private static final String NAME =
            DisallowPriKeyField_int2long.class.getName();

        @PrimaryKey
        long key;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Type may not be changed for a primary key field or composite key class field when evolving class: com.sleepycat.persist.test.EvolveClasses$DisallowPriKeyField_int2long version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowPriKeyField_int2long version: 1 Error: Old field type: int is not compatible with the new type: long for field: key";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99);
        }
    }

    @Entity(version=1)
    static class DisallowPriKeyField_long2float
        extends EvolveCase {

        private static final String NAME =
            DisallowPriKeyField_long2float.class.getName();

        @PrimaryKey
        float key;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Type may not be changed for a primary key field or composite key class field when evolving class: com.sleepycat.persist.test.EvolveClasses$DisallowPriKeyField_long2float version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowPriKeyField_long2float version: 1 Error: Old field type: long is not compatible with the new type: float for field: key";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, (long) 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", (long) 99);
        }
    }

    @Entity(version=1)
    static class DisallowPriKeyField_float2double
        extends EvolveCase {

        private static final String NAME =
            DisallowPriKeyField_float2double.class.getName();

        @PrimaryKey
        double key;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Type may not be changed for a primary key field or composite key class field when evolving class: com.sleepycat.persist.test.EvolveClasses$DisallowPriKeyField_float2double version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowPriKeyField_float2double version: 1 Error: Old field type: float is not compatible with the new type: double for field: key";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, (float) 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", (float) 99);
        }
    }

    @Entity(version=1)
    static class DisallowPriKeyField_Byte2short2
        extends EvolveCase {

        private static final String NAME =
            DisallowPriKeyField_Byte2short2.class.getName();

        @PrimaryKey
        short key;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Type may not be changed for a primary key field or composite key class field when evolving class: com.sleepycat.persist.test.EvolveClasses$DisallowPriKeyField_Byte2short2 version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowPriKeyField_Byte2short2 version: 1 Error: Old field type: java.lang.Byte is not compatible with the new type: short for field: key";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, (byte) 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", (byte) 99);
        }
    }

    @Entity(version=1)
    static class DisallowPriKeyField_Character2int
        extends EvolveCase {

        private static final String NAME =
            DisallowPriKeyField_Character2int.class.getName();

        @PrimaryKey
        int key;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Type may not be changed for a primary key field or composite key class field when evolving class: com.sleepycat.persist.test.EvolveClasses$DisallowPriKeyField_Character2int version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowPriKeyField_Character2int version: 1 Error: Old field type: java.lang.Character is not compatible with the new type: int for field: key";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, (char) 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", (char) 99);
        }
    }

    @Entity(version=1)
    static class DisallowPriKeyField_Short2int2
        extends EvolveCase {

        private static final String NAME =
            DisallowPriKeyField_Short2int2.class.getName();

        @PrimaryKey
        int key;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Type may not be changed for a primary key field or composite key class field when evolving class: com.sleepycat.persist.test.EvolveClasses$DisallowPriKeyField_Short2int2 version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowPriKeyField_Short2int2 version: 1 Error: Old field type: java.lang.Short is not compatible with the new type: int for field: key";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, (short) 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", (short) 99);
        }
    }

    @Entity(version=1)
    static class DisallowPriKeyField_Integer2long
        extends EvolveCase {

        private static final String NAME =
            DisallowPriKeyField_Integer2long.class.getName();

        @PrimaryKey
        long key;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Type may not be changed for a primary key field or composite key class field when evolving class: com.sleepycat.persist.test.EvolveClasses$DisallowPriKeyField_Integer2long version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowPriKeyField_Integer2long version: 1 Error: Old field type: java.lang.Integer is not compatible with the new type: long for field: key";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99);
        }
    }

    @Entity(version=1)
    static class DisallowPriKeyField_Long2float2
        extends EvolveCase {

        private static final String NAME =
            DisallowPriKeyField_Long2float2.class.getName();

        @PrimaryKey
        float key;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Type may not be changed for a primary key field or composite key class field when evolving class: com.sleepycat.persist.test.EvolveClasses$DisallowPriKeyField_Long2float2 version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowPriKeyField_Long2float2 version: 1 Error: Old field type: java.lang.Long is not compatible with the new type: float for field: key";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, (long) 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", (long) 99);
        }
    }

    @Entity(version=1)
    static class DisallowPriKeyField_Float2double2
        extends EvolveCase {

        private static final String NAME =
            DisallowPriKeyField_Float2double2.class.getName();

        @PrimaryKey
        double key;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Type may not be changed for a primary key field or composite key class field when evolving class: com.sleepycat.persist.test.EvolveClasses$DisallowPriKeyField_Float2double2 version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowPriKeyField_Float2double2 version: 1 Error: Old field type: java.lang.Float is not compatible with the new type: double for field: key";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, (float) 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", (float) 99);
        }
    }

    @Entity(version=1)
    static class DisallowPriKeyField_Long2BigInt
        extends EvolveCase {

        private static final String NAME =
            DisallowPriKeyField_Long2BigInt.class.getName();

        @PrimaryKey
        BigInteger key;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Type may not be changed for a primary key field or composite key class field when evolving class: com.sleepycat.persist.test.EvolveClasses$DisallowPriKeyField_Long2BigInt version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowPriKeyField_Long2BigInt version: 1 Error: Old field type: java.lang.Long is not compatible with the new type: java.math.BigInteger for field: key";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawObject obj = readRaw(store, 99L, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99L);
        }
    }

    @Persistent(version=1)
    static class DisallowCompositeKeyField_byte2short_Key {

        @KeyField(1)
        int f1 = 1;

        @KeyField(2)
        short f2 = 2;

        @KeyField(3)
        String f3 = "3";
    }

    @Entity
    static class DisallowCompositeKeyField_byte2short
        extends EvolveCase {

        private static final String NAME =
            DisallowCompositeKeyField_byte2short.class.getName();
        private static final String NAME2 =
            DisallowCompositeKeyField_byte2short_Key.class.getName();

        @PrimaryKey
        DisallowCompositeKeyField_byte2short_Key key;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Type may not be changed for a primary key field or composite key class field when evolving class: com.sleepycat.persist.test.EvolveClasses$DisallowCompositeKeyField_byte2short_Key version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowCompositeKeyField_byte2short_Key version: 1 Error: Old field type: byte is not compatible with the new type: short for field: f2";
        }

        @Override
        void checkUnevolvedModel(EntityModel model, Environment env) {
            checkEntity(true, model, env, NAME, 0, null);
            checkNonEntity(true, model, env, NAME2, 0);
            checkVersions(model, NAME, 0);
            checkVersions(model, NAME2, 0);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            if (expectEvolved) {
                TestCase.fail();
            }
            RawType rawKeyType = store.getModel().getRawType(NAME2);
            RawObject rawKey = new RawObject
                (rawKeyType,
                 makeValues("f1", 1, "f2", (byte) 2, "f3", "3"),
                 null);

            RawObject obj = readRaw(store, rawKey, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", rawKey);
        }
    }

    @Entity(version=1)
    static class AllowPriKeyField_byte2Byte
        extends EvolveCase {

        private static final String NAME =
            AllowPriKeyField_byte2Byte.class.getName();

        @PrimaryKey
        Byte key = 99;

        @Override
        void checkEvolvedModel(EntityModel model,
                               Environment env,
                               boolean oldTypesExist) {
            checkEntity(true, model, env, NAME, 1, null);
            if (oldTypesExist) {
                checkVersions(model, NAME, 1, NAME, 0);
            } else {
                checkVersions(model, NAME, 1);
            }
        }

        @Override
        void readObjects(EntityStore store, boolean doUpdate)
            throws DatabaseException {

            PrimaryIndex<Byte,AllowPriKeyField_byte2Byte>
                index = store.getPrimaryIndex
                    (Byte.class,
                     AllowPriKeyField_byte2Byte.class);
            AllowPriKeyField_byte2Byte obj = index.get(key);
            TestCase.assertNotNull(obj);
            TestCase.assertEquals(Byte.valueOf((byte) 99), obj.key);

            if (doUpdate) {
                index.put(obj);
            }
        }

        @Override
        void copyRawObjects(RawStore rawStore, EntityStore newStore)
            throws DatabaseException {

            PrimaryIndex<Byte,AllowPriKeyField_byte2Byte>
                index = newStore.getPrimaryIndex
                    (Byte.class,
                     AllowPriKeyField_byte2Byte.class);
            RawObject raw = rawStore.getPrimaryIndex(NAME).get((byte) 99);
            index.put((AllowPriKeyField_byte2Byte)
                      newStore.getModel().convertRawObject(raw));
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            RawObject obj;
            if (expectEvolved) {
                obj = readRaw(store, (byte) 99, NAME, 1, CASECLS, 0);
            } else {
                obj = readRaw(store, (byte) 99, NAME, 0, CASECLS, 0);
            }
            checkRawFields(obj, "key", (byte) 99);
        }
    }

    @Entity(version=1)
    static class AllowPriKeyField_Byte2byte2
        extends EvolveCase {

        private static final String NAME =
            AllowPriKeyField_Byte2byte2.class.getName();

        @PrimaryKey
        byte key = 99;

        @Override
        void checkEvolvedModel(EntityModel model,
                               Environment env,
                               boolean oldTypesExist) {
            checkEntity(true, model, env, NAME, 1, null);
            if (oldTypesExist) {
                checkVersions(model, NAME, 1, NAME, 0);
            } else {
                checkVersions(model, NAME, 1);
            }
        }

        @Override
        void readObjects(EntityStore store, boolean doUpdate)
            throws DatabaseException {

            PrimaryIndex<Byte,AllowPriKeyField_Byte2byte2>
                index = store.getPrimaryIndex
                    (Byte.class,
                     AllowPriKeyField_Byte2byte2.class);
            AllowPriKeyField_Byte2byte2 obj = index.get(key);
            TestCase.assertNotNull(obj);
            TestCase.assertEquals((byte) 99, obj.key);

            if (doUpdate) {
                index.put(obj);
            }
        }

        @Override
        void copyRawObjects(RawStore rawStore, EntityStore newStore)
            throws DatabaseException {

            PrimaryIndex<Byte,AllowPriKeyField_Byte2byte2>
                index = newStore.getPrimaryIndex
                    (Byte.class,
                     AllowPriKeyField_Byte2byte2.class);
            RawObject raw = rawStore.getPrimaryIndex(NAME).get((byte) 99);
            index.put((AllowPriKeyField_Byte2byte2)
                      newStore.getModel().convertRawObject(raw));
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            RawObject obj;
            if (expectEvolved) {
                obj = readRaw(store, (byte) 99, NAME, 1, CASECLS, 0);
            } else {
                obj = readRaw(store, (byte) 99, NAME, 0, CASECLS, 0);
            }
            checkRawFields(obj, "key", (byte) 99);
        }
    }

    @Persistent(version=1)
    static class AllowFieldTypeChanges_Key {

        AllowFieldTypeChanges_Key() {
            this(false);
        }

        AllowFieldTypeChanges_Key(boolean init) {
            if (init) {
                f1 = true;
                f2 = (byte) 2;
                f3 = (short) 3;
                f4 = 4;
                f5 = 5L;
                f6 = 6F;
                f7 = 7D;
                f8 = (char) 8;
                f9 = true;
                f10 = (byte) 10;
                f11 = (short) 11;
                f12 = 12;
                f13 = 13L;
                f14 = 14F;
                f15 = 15D;
                f16 = (char) 16;
            }
        }

        @KeyField(1)
        boolean f1;

        @KeyField(2)
        byte f2;

        @KeyField(3)
        short f3;

        @KeyField(4)
        int f4;

        @KeyField(5)
        long f5;

        @KeyField(6)
        float f6;

        @KeyField(7)
        double f7;

        @KeyField(8)
        char f8;

        @KeyField(9)
        Boolean f9;

        @KeyField(10)
        Byte f10;

        @KeyField(11)
        Short f11;

        @KeyField(12)
        Integer f12;

        @KeyField(13)
        Long f13;

        @KeyField(14)
        Float f14;

        @KeyField(15)
        Double f15;

        @KeyField(16)
        Character f16;
    }

    @Persistent(version=1)
    static class AllowFieldTypeChanges_Base
        extends EvolveCase {

        @SecondaryKey(relate=ONE_TO_ONE)
        AllowFieldTypeChanges_Key kComposite;

        Integer f_long2Integer;
        Long f_String2Long;
    }

    /**
     * Allow field type changes: automatic widening, supported widening,
     * and Converter mutations.  Also tests primary and secondary key field
     * renaming.
     */
    @Entity(version=1)
    static class AllowFieldTypeChanges
        extends AllowFieldTypeChanges_Base {

        private static final String NAME =
            AllowFieldTypeChanges.class.getName();
        private static final String NAME2 =
            AllowFieldTypeChanges_Base.class.getName();
        private static final String NAME3 =
            AllowFieldTypeChanges_Key.class.getName();

        @PrimaryKey
        Integer pkeyInteger;

        @SecondaryKey(relate=ONE_TO_ONE)
        Boolean kBoolean;

        @SecondaryKey(relate=ONE_TO_ONE)
        Byte kByte;

        @SecondaryKey(relate=ONE_TO_ONE)
        Short kShort;

        @SecondaryKey(relate=ONE_TO_ONE)
        Integer kInteger;

        @SecondaryKey(relate=ONE_TO_ONE)
        Long kLong;

        @SecondaryKey(relate=ONE_TO_ONE)
        Float kFloat;

        @SecondaryKey(relate=ONE_TO_ONE)
        Double kDouble;

        @SecondaryKey(relate=ONE_TO_ONE)
        Character kCharacter;

        short f01;
        int f02;
        long f03;
        float f04;
        double f06;
        int f07;
        long f08;
        float f09;
        double f10;
        int f11;
        long f12;
        float f13;
        double f14;
        long f15;
        float f16;
        double f17;
        float f18;
        double f19;
        double f20;

        Short f21;
        Integer f22;
        Long f23;
        Float f24;
        Double f26;
        Integer f27;
        Long f28;
        Float f29;
        Double f30;
        Integer f31;
        Long f32;
        Float f33;
        Double f34;
        Long f35;
        Float f36;
        Double f37;
        Float f38;
        Double f39;
        Double f40;

        Short f41;
        Integer f42;
        Long f43;
        Float f44;
        Double f46;
        Integer f47;
        Long f48;
        Float f49;
        Double f50;
        Integer f51;
        Long f52;
        Float f53;
        Double f54;
        Long f55;
        Float f56;
        Double f57;
        Float f58;
        Double f59;
        Double f60;

        BigInteger f70;
        BigInteger f71;
        BigInteger f72;
        BigInteger f73;
        BigInteger f74;
        BigInteger f75;
        BigInteger f76;
        BigInteger f77;
        BigInteger f78;
        BigInteger f79;

        int f_long2int;
        long f_String2long;

        @Override
        Mutations getMutations() {
            Mutations m = new Mutations();
            m.addRenamer(new Renamer(NAME, 0, "pkeyint", "pkeyInteger"));
            m.addRenamer(new Renamer(NAME, 0, "kboolean", "kBoolean"));
            m.addRenamer(new Renamer(NAME, 0, "kbyte", "kByte"));
            m.addRenamer(new Renamer(NAME, 0, "kshort", "kShort"));
            m.addRenamer(new Renamer(NAME, 0, "kint", "kInteger"));
            m.addRenamer(new Renamer(NAME, 0, "klong", "kLong"));
            m.addRenamer(new Renamer(NAME, 0, "kfloat", "kFloat"));
            m.addRenamer(new Renamer(NAME, 0, "kdouble", "kDouble"));
            m.addRenamer(new Renamer(NAME, 0, "kchar", "kCharacter"));
            m.addRenamer(new Renamer(NAME2, 0, "kcomposite", "kComposite"));

            Conversion conv1 = new MyConversion1();
            Conversion conv2 = new MyConversion2();

            m.addConverter(new Converter(NAME, 0, "f_long2int", conv1));
            m.addConverter(new Converter(NAME, 0, "f_String2long", conv2));
            m.addConverter(new Converter(NAME2, 0, "f_long2Integer", conv1));
            m.addConverter(new Converter(NAME2, 0, "f_String2Long", conv2));
            return m;
        }

        @SuppressWarnings("serial")
        static class MyConversion1 implements Conversion {

            public void initialize(EntityModel model) {}

            public Object convert(Object o) {
                return ((Long) o).intValue();
            }

            @Override
            public boolean equals(Object other) { return true; }
        }

        @SuppressWarnings("serial")
        static class MyConversion2 implements Conversion {

            public void initialize(EntityModel model) {}

            public Object convert(Object o) {
                return Long.valueOf((String) o);
            }

            @Override
            public boolean equals(Object other) { return true; }
        }

        @Override
        void checkEvolvedModel(EntityModel model,
                               Environment env,
                               boolean oldTypesExist) {
            checkEntity(true, model, env, NAME, 1, null);
            checkNonEntity(true, model, env, NAME2, 1);
            if (oldTypesExist) {
                checkVersions(model, NAME, 1, NAME, 0);
                checkVersions(model, NAME2, 1, NAME2, 0);
                checkVersions(model, NAME3, 1, NAME3, 0);
            } else {
                checkVersions(model, NAME, 1);
                checkVersions(model, NAME2, 1);
                checkVersions(model, NAME3, 1);
            }
        }

        @Override
        void readObjects(EntityStore store, boolean doUpdate)
            throws DatabaseException {

            PrimaryIndex<Integer,AllowFieldTypeChanges>
                index = store.getPrimaryIndex
                    (Integer.class, AllowFieldTypeChanges.class);
            AllowFieldTypeChanges obj = index.get(99);
            checkValues(obj);
            checkSecondaries(store, index);

            if (doUpdate) {
                index.put(obj);
                checkSecondaries(store, index);
            }
        }

        @Override
        void copyRawObjects(RawStore rawStore, EntityStore newStore)
            throws DatabaseException {

            PrimaryIndex<Integer,AllowFieldTypeChanges>
                index = newStore.getPrimaryIndex
                    (Integer.class, AllowFieldTypeChanges.class);
            RawObject raw = rawStore.getPrimaryIndex(NAME).get(99);
            index.put((AllowFieldTypeChanges)
                      newStore.getModel().convertRawObject(raw));
        }

        private void checkSecondaries(EntityStore store,
                                      PrimaryIndex<Integer,
                                                   AllowFieldTypeChanges>
                                                   index)
            throws DatabaseException {

            checkValues(store.getSecondaryIndex
                (index, Boolean.class, "kBoolean").get(true));
            checkValues(store.getSecondaryIndex
                (index, Byte.class, "kByte").get((byte) 77));
            checkValues(store.getSecondaryIndex
                (index, Short.class, "kShort").get((short) 66));
            checkValues(store.getSecondaryIndex
                (index, Integer.class, "kInteger").get(55));
            checkValues(store.getSecondaryIndex
                (index, Long.class, "kLong").get((long) 44));
            checkValues(store.getSecondaryIndex
                (index, Float.class, "kFloat").get((float) 33));
            checkValues(store.getSecondaryIndex
                (index, Double.class, "kDouble").get((double) 22));
            checkValues(store.getSecondaryIndex
                (index, Character.class, "kCharacter").get((char) 11));
            checkValues(store.getSecondaryIndex
                (index, AllowFieldTypeChanges_Key.class, "kComposite").get
                    (new AllowFieldTypeChanges_Key(true)));
        }

        private void checkValues(AllowFieldTypeChanges obj) {
            TestCase.assertNotNull(obj);
            TestCase.assertEquals(obj.pkeyInteger, Integer.valueOf(99));
            TestCase.assertEquals(obj.kBoolean, Boolean.valueOf(true));
            TestCase.assertEquals(obj.kByte, Byte.valueOf((byte) 77));
            TestCase.assertEquals(obj.kShort, Short.valueOf((short) 66));
            TestCase.assertEquals(obj.kInteger, Integer.valueOf(55));
            TestCase.assertEquals(obj.kLong, Long.valueOf(44));
            TestCase.assertEquals(obj.kFloat, Float.valueOf(33));
            TestCase.assertEquals(obj.kDouble, Double.valueOf(22));
            TestCase.assertEquals(obj.kCharacter, Character.valueOf((char) 11));

            AllowFieldTypeChanges_Key embed = obj.kComposite;
            TestCase.assertNotNull(embed);
            TestCase.assertEquals(embed.f1, true);
            TestCase.assertEquals(embed.f2, (byte) 2);
            TestCase.assertEquals(embed.f3, (short) 3);
            TestCase.assertEquals(embed.f4, 4);
            TestCase.assertEquals(embed.f5, 5L);
            TestCase.assertEquals(embed.f6, 6F);
            TestCase.assertEquals(embed.f7, 7D);
            TestCase.assertEquals(embed.f8, (char) 8);
            TestCase.assertEquals(embed.f9, Boolean.valueOf(true));
            TestCase.assertEquals(embed.f10, Byte.valueOf((byte) 10));
            TestCase.assertEquals(embed.f11, Short.valueOf((short) 11));
            TestCase.assertEquals(embed.f12, Integer.valueOf(12));
            TestCase.assertEquals(embed.f13, Long.valueOf(13L));
            TestCase.assertEquals(embed.f14, Float.valueOf(14F));
            TestCase.assertEquals(embed.f15, Double.valueOf(15D));
            TestCase.assertEquals(embed.f16, Character.valueOf((char) 16));

            TestCase.assertEquals(obj.f01, (short) 1);
            TestCase.assertEquals(obj.f02, 2);
            TestCase.assertEquals(obj.f03, 3);
            TestCase.assertEquals(obj.f04, (float) 4);
            TestCase.assertEquals(obj.f06, (double) 6);
            TestCase.assertEquals(obj.f07, 7);
            TestCase.assertEquals(obj.f08, 8);
            TestCase.assertEquals(obj.f09, (float) 9);
            TestCase.assertEquals(obj.f10, (double) 10);
            TestCase.assertEquals(obj.f11, 11);
            TestCase.assertEquals(obj.f12, 12);
            TestCase.assertEquals(obj.f13, (float) 13);
            TestCase.assertEquals(obj.f14, (double) 14);
            TestCase.assertEquals(obj.f15, 15L);
            TestCase.assertEquals(obj.f16, 16F);
            TestCase.assertEquals(obj.f17, 17D);
            TestCase.assertEquals(obj.f18, (float) 18);
            TestCase.assertEquals(obj.f19, (double) 19);
            TestCase.assertEquals(obj.f20, (double) 20);

            TestCase.assertEquals(obj.f21, Short.valueOf((byte) 21));
            TestCase.assertEquals(obj.f22, Integer.valueOf((byte) 22));
            TestCase.assertEquals(obj.f23, Long.valueOf((byte) 23));
            TestCase.assertEquals(obj.f24, Float.valueOf((byte) 24));
            TestCase.assertEquals(obj.f26, Double.valueOf((byte) 26));
            TestCase.assertEquals(obj.f27, Integer.valueOf((short) 27));
            TestCase.assertEquals(obj.f28, Long.valueOf((short) 28));
            TestCase.assertEquals(obj.f29, Float.valueOf((short) 29));
            TestCase.assertEquals(obj.f30, Double.valueOf((short) 30));
            TestCase.assertEquals(obj.f31, Integer.valueOf((char) 31));
            TestCase.assertEquals(obj.f32, Long.valueOf((char) 32));
            TestCase.assertEquals(obj.f33, Float.valueOf((char) 33));
            TestCase.assertEquals(obj.f34, Double.valueOf((char) 34));
            TestCase.assertEquals(obj.f35, Long.valueOf(35));
            TestCase.assertEquals(obj.f36, Float.valueOf(36));
            TestCase.assertEquals(obj.f37, Double.valueOf(37));
            TestCase.assertEquals(obj.f38, Float.valueOf(38));
            TestCase.assertEquals(obj.f39, Double.valueOf(39));
            TestCase.assertEquals(obj.f40, Double.valueOf(40));

            TestCase.assertEquals(obj.f41, Short.valueOf((byte) 41));
            TestCase.assertEquals(obj.f42, Integer.valueOf((byte) 42));
            TestCase.assertEquals(obj.f43, Long.valueOf((byte) 43));
            TestCase.assertEquals(obj.f44, Float.valueOf((byte) 44));
            TestCase.assertEquals(obj.f46, Double.valueOf((byte) 46));
            TestCase.assertEquals(obj.f47, Integer.valueOf((short) 47));
            TestCase.assertEquals(obj.f48, Long.valueOf((short) 48));
            TestCase.assertEquals(obj.f49, Float.valueOf((short) 49));
            TestCase.assertEquals(obj.f50, Double.valueOf((short) 50));
            TestCase.assertEquals(obj.f51, Integer.valueOf((char) 51));
            TestCase.assertEquals(obj.f52, Long.valueOf((char) 52));
            TestCase.assertEquals(obj.f53, Float.valueOf((char) 53));
            TestCase.assertEquals(obj.f54, Double.valueOf((char) 54));
            TestCase.assertEquals(obj.f55, Long.valueOf(55));
            TestCase.assertEquals(obj.f56, Float.valueOf(56));
            TestCase.assertEquals(obj.f57, Double.valueOf(57));
            TestCase.assertEquals(obj.f58, Float.valueOf(58));
            TestCase.assertEquals(obj.f59, Double.valueOf(59));
            TestCase.assertEquals(obj.f60, Double.valueOf(60));

            TestCase.assertEquals(obj.f70, BigInteger.valueOf(70));
            TestCase.assertEquals(obj.f71, BigInteger.valueOf(71));
            TestCase.assertEquals(obj.f72, BigInteger.valueOf(72));
            TestCase.assertEquals(obj.f73, BigInteger.valueOf(73));
            TestCase.assertEquals(obj.f74, BigInteger.valueOf(74));
            TestCase.assertEquals(obj.f75, BigInteger.valueOf(75));
            TestCase.assertEquals(obj.f76, BigInteger.valueOf(76));
            TestCase.assertEquals(obj.f77, BigInteger.valueOf(77));
            TestCase.assertEquals(obj.f78, BigInteger.valueOf(78));
            TestCase.assertEquals(obj.f79, BigInteger.valueOf(79));

            TestCase.assertEquals(obj.f_long2Integer, Integer.valueOf(111));
            TestCase.assertEquals(obj.f_String2Long, Long.valueOf(222));
            TestCase.assertEquals(obj.f_long2int, 333);
            TestCase.assertEquals(obj.f_String2long, 444L);
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            RawType embedType = store.getModel().getRawType(NAME3);
            RawObject embed = new RawObject
                (embedType,
                 makeValues
                    ("f1", true,
                     "f2", (byte) 2,
                     "f3", (short) 3,
                     "f4", 4,
                     "f5", 5L,
                     "f6", 6F,
                     "f7", 7D,
                     "f8", (char) 8,
                     "f9", true,
                     "f10", (byte) 10,
                     "f11", (short) 11,
                     "f12", 12,
                     "f13", 13L,
                     "f14", 14F,
                     "f15", 15D,
                     "f16", (char) 16),
                 null);

            RawObject obj;
            if (expectEvolved) {
                obj = readRaw(store, 99, NAME, 1, NAME2, 1, CASECLS, 0);
                checkRawFields(obj, "pkeyInteger", 99,
                               "kBoolean", true,
                               "kByte", (byte) 77,
                               "kShort", (short) 66,
                               "kInteger", 55,
                               "kLong", (long) 44,
                               "kFloat", (float) 33,
                               "kDouble", (double) 22,
                               "kCharacter", (char) 11,

                               "f01", (short) 1,
                               "f02", 2,
                               "f03", (long) 3,
                               "f04", (float) 4,
                               "f06", (double) 6,
                               "f07", 7,
                               "f08", (long) 8,
                               "f09", (float) 9,
                               "f10", (double) 10,
                               "f11", 11,
                               "f12", (long) 12,
                               "f13", (float) 13,
                               "f14", (double) 14,
                               "f15", 15L,
                               "f16", 16F,
                               "f17", 17D,
                               "f18", (float) 18,
                               "f19", (double) 19,
                               "f20", (double) 20,

                               "f21", (short) 21,
                               "f22", 22,
                               "f23", (long) 23,
                               "f24", (float) 24,
                               "f26", (double) 26,
                               "f27", 27,
                               "f28", (long) 28,
                               "f29", (float) 29,
                               "f30", (double) 30,
                               "f31", 31,
                               "f32", (long) 32,
                               "f33", (float) 33,
                               "f34", (double) 34,
                               "f35", 35L,
                               "f36", 36F,
                               "f37", 37D,
                               "f38", (float) 38,
                               "f39", (double) 39,
                               "f40", (double) 40,

                               "f41", (short) 41,
                               "f42", 42,
                               "f43", (long) 43,
                               "f44", (float) 44,
                               "f46", (double) 46,
                               "f47", 47,
                               "f48", (long) 48,
                               "f49", (float) 49,
                               "f50", (double) 50,
                               "f51", 51,
                               "f52", (long) 52,
                               "f53", (float) 53,
                               "f54", (double) 54,
                               "f55", 55L,
                               "f56", 56F,
                               "f57", 57D,
                               "f58", (float) 58,
                               "f59", (double) 59,
                               "f60", (double) 60,

                               "f70", BigInteger.valueOf(70),
                               "f71", BigInteger.valueOf(71),
                               "f72", BigInteger.valueOf(72),
                               "f73", BigInteger.valueOf(73),
                               "f74", BigInteger.valueOf(74),
                               "f75", BigInteger.valueOf(75),
                               "f76", BigInteger.valueOf(76),
                               "f77", BigInteger.valueOf(77),
                               "f78", BigInteger.valueOf(78),
                               "f79", BigInteger.valueOf(79),

                               "f_long2int", 333,
                               "f_String2long", 444L);
                checkRawFields(obj.getSuper(),
                               "kComposite", embed,
                               "f_long2Integer", 111,
                               "f_String2Long", 222L);
            } else {
                obj = readRaw(store, 99, NAME, 0, NAME2, 0, CASECLS, 0);
                checkRawFields(obj, "pkeyint", 99,
                               "kboolean", true,
                               "kbyte", (byte) 77,
                               "kshort", (short) 66,
                               "kint", 55,
                               "klong", (long) 44,
                               "kfloat", (float) 33,
                               "kdouble", (double) 22,
                               "kchar", (char) 11,

                               "f01", (byte) 1,
                               "f02", (byte) 2,
                               "f03", (byte) 3,
                               "f04", (byte) 4,
                               "f06", (byte) 6,
                               "f07", (short) 7,
                               "f08", (short) 8,
                               "f09", (short) 9,
                               "f10", (short) 10,
                               "f11", (char) 11,
                               "f12", (char) 12,
                               "f13", (char) 13,
                               "f14", (char) 14,
                               "f15", 15,
                               "f16", 16,
                               "f17", 17,
                               "f18", (long) 18,
                               "f19", (long) 19,
                               "f20", (float) 20,

                               "f21", (byte) 21,
                               "f22", (byte) 22,
                               "f23", (byte) 23,
                               "f24", (byte) 24,
                               "f26", (byte) 26,
                               "f27", (short) 27,
                               "f28", (short) 28,
                               "f29", (short) 29,
                               "f30", (short) 30,
                               "f31", (char) 31,
                               "f32", (char) 32,
                               "f33", (char) 33,
                               "f34", (char) 34,
                               "f35", 35,
                               "f36", 36,
                               "f37", 37,
                               "f38", (long) 38,
                               "f39", (long) 39,
                               "f40", (float) 40,

                               "f41", (byte) 41,
                               "f42", (byte) 42,
                               "f43", (byte) 43,
                               "f44", (byte) 44,
                               "f46", (byte) 46,
                               "f47", (short) 47,
                               "f48", (short) 48,
                               "f49", (short) 49,
                               "f50", (short) 50,
                               "f51", (char) 51,
                               "f52", (char) 52,
                               "f53", (char) 53,
                               "f54", (char) 54,
                               "f55", 55,
                               "f56", 56,
                               "f57", 57,
                               "f58", (long) 58,
                               "f59", (long) 59,
                               "f60", (float) 60,

                               "f70", (byte) 70,
                               "f71", (short) 71,
                               "f72", (char) 72,
                               "f73", 73,
                               "f74", (long) 74,
                               "f75", (byte) 75,
                               "f76", (short) 76,
                               "f77", (char) 77,
                               "f78", 78,
                               "f79", (long) 79,

                               "f_long2int", 333L,
                               "f_String2long", "444");

                checkRawFields(obj.getSuper(),
                               "kcomposite", embed,
                               "f_long2Integer", 111L,
                               "f_String2Long", "222");
            }
            Environment env = store.getEnvironment();

            assertDbExists(expectEvolved, env, NAME, "kBoolean");
            assertDbExists(expectEvolved, env, NAME, "kByte");
            assertDbExists(expectEvolved, env, NAME, "kShort");
            assertDbExists(expectEvolved, env, NAME, "kInteger");
            assertDbExists(expectEvolved, env, NAME, "kLong");
            assertDbExists(expectEvolved, env, NAME, "kFloat");
            assertDbExists(expectEvolved, env, NAME, "kDouble");
            assertDbExists(expectEvolved, env, NAME, "kCharacter");
            assertDbExists(expectEvolved, env, NAME, "kComposite");

            assertDbExists(!expectEvolved, env, NAME, "kboolean");
            assertDbExists(!expectEvolved, env, NAME, "kbyte");
            assertDbExists(!expectEvolved, env, NAME, "kshort");
            assertDbExists(!expectEvolved, env, NAME, "kint");
            assertDbExists(!expectEvolved, env, NAME, "klong");
            assertDbExists(!expectEvolved, env, NAME, "kfloat");
            assertDbExists(!expectEvolved, env, NAME, "kdouble");
            assertDbExists(!expectEvolved, env, NAME, "kchar");
            assertDbExists(!expectEvolved, env, NAME, "kcomposite");
        }
    }

    @SuppressWarnings("serial")
    static class ConvertFieldContent_Conversion implements Conversion {

        public void initialize(EntityModel model) {
        }

        public Object convert(Object fromValue) {
            String s1 = (String) fromValue;
            return (new StringBuilder(s1)).reverse().toString();
        }

        @Override
        public boolean equals(Object o) {
            return o instanceof ConvertFieldContent_Conversion;
        }
    }

    @Entity(version=1)
    static class ConvertFieldContent_Entity
        extends EvolveCase {

        private static final String NAME =
            ConvertFieldContent_Entity.class.getName();

        @PrimaryKey
        int key = 99;

        String f1;
        String f2;

        @Override
        Mutations getMutations() {
            Mutations m = new Mutations();
            Converter converter = new Converter
                (ConvertFieldContent_Entity.class.getName(), 0,
                 "f1", new ConvertFieldContent_Conversion());
            m.addConverter(converter);
            converter = new Converter
                (ConvertFieldContent_Entity.class.getName(), 0,
                 "f2", new ConvertFieldContent_Conversion());
            m.addConverter(converter);
            return m;
        }

        @Override
        void checkEvolvedModel(EntityModel model,
                               Environment env,
                               boolean oldTypesExist) {
            checkEntity(true, model, env, NAME, 1, null);
            if (oldTypesExist) {
                checkVersions(model, NAME, 1, NAME, 0);
            } else {
                checkVersions(model, NAME, 1);
            }
        }

        @Override
        void readObjects(EntityStore store, boolean doUpdate)
            throws DatabaseException {

            PrimaryIndex<Integer,ConvertFieldContent_Entity>
                index = store.getPrimaryIndex
                    (Integer.class,
                     ConvertFieldContent_Entity.class);
            ConvertFieldContent_Entity obj = index.get(99);
            TestCase.assertNotNull(obj);
            TestCase.assertEquals(99, obj.key);
            TestCase.assertEquals("43210", obj.f1);
            TestCase.assertEquals("98765", obj.f2);

            if (doUpdate) {
                index.put(obj);
            }
        }

        @Override
        void copyRawObjects(RawStore rawStore, EntityStore newStore)
            throws DatabaseException {

            PrimaryIndex<Integer,ConvertFieldContent_Entity>
                index = newStore.getPrimaryIndex
                    (Integer.class,
                     ConvertFieldContent_Entity.class);
            RawObject raw = rawStore.getPrimaryIndex(NAME).get(99);
            index.put((ConvertFieldContent_Entity)
                      newStore.getModel().convertRawObject(raw));
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            RawObject obj =
                readRaw(store, 99, NAME, expectEvolved ? 1 : 0, CASECLS, 0);
            if (expectEvolved) {
                checkRawFields(obj, "key", 99,
                                    "f1", "43210",
                                    "f2", "98765");
            } else {
                checkRawFields(obj, "key", 99,
                                    "f1", "01234",
                                    "f2", "56789");
            }
        }
    }

    @Persistent(version=1)
    static class ConvertExample1_Address {
        String street;
        String city;
        String state;
        int zipCode;
    }

    @SuppressWarnings("serial")
    static class ConvertExample1_Conversion implements Conversion {

        public void initialize(EntityModel model) {
        }

        public Object convert(Object fromValue) {
            return Integer.valueOf((String) fromValue);
        }

        @Override
        public boolean equals(Object o) {
            return o instanceof ConvertExample1_Conversion;
        }
    }

    @Entity
    static class ConvertExample1_Entity
        extends EvolveCase {

        private static final String NAME =
            ConvertExample1_Entity.class.getName();
        private static final String NAME2 =
            ConvertExample1_Address.class.getName();

        @PrimaryKey
        int key = 99;

        ConvertExample1_Address embed;

        @Override
        Mutations getMutations() {
            Mutations m = new Mutations();
            Converter converter = new Converter
                (ConvertExample1_Address.class.getName(), 0,
                 "zipCode", new ConvertExample1_Conversion());
            m.addConverter(converter);
            return m;
        }

        @Override
        void checkEvolvedModel(EntityModel model,
                               Environment env,
                               boolean oldTypesExist) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
            if (oldTypesExist) {
                checkVersions(model, NAME2, 1, NAME2, 0);
            } else {
                checkVersions(model, NAME2, 1);
            }
        }

        @Override
        void readObjects(EntityStore store, boolean doUpdate)
            throws DatabaseException {

            PrimaryIndex<Integer,ConvertExample1_Entity>
                index = store.getPrimaryIndex
                    (Integer.class,
                     ConvertExample1_Entity.class);
            ConvertExample1_Entity obj = index.get(99);
            TestCase.assertNotNull(obj);
            TestCase.assertEquals(99, obj.key);
            TestCase.assertNotNull(obj.embed);
            TestCase.assertEquals("street", obj.embed.street);
            TestCase.assertEquals("city", obj.embed.city);
            TestCase.assertEquals("state", obj.embed.state);
            TestCase.assertEquals(12345, obj.embed.zipCode);

            if (doUpdate) {
                index.put(obj);
            }
        }

        @Override
        void copyRawObjects(RawStore rawStore, EntityStore newStore)
            throws DatabaseException {

            PrimaryIndex<Integer,ConvertExample1_Entity>
                index = newStore.getPrimaryIndex
                    (Integer.class,
                     ConvertExample1_Entity.class);
            RawObject raw = rawStore.getPrimaryIndex(NAME).get(99);
            index.put((ConvertExample1_Entity)
                      newStore.getModel().convertRawObject(raw));
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            RawType embedType = store.getModel().getRawType(NAME2);
            RawObject embed;
            if (expectEvolved) {
                embed = new RawObject
                    (embedType,
                     makeValues("street", "street",
                                "city", "city",
                                "state", "state",
                                "zipCode", 12345),
                     null);
            } else {
                embed = new RawObject
                    (embedType,
                     makeValues("street", "street",
                                "city", "city",
                                "state", "state",
                                "zipCode", "12345"),
                     null);
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "embed", embed);
        }
    }

    @Persistent
    static class ConvertExample2_Address {
        String street;
        String city;
        String state;
        int zipCode;
    }

    @Entity(version=1)
    static class ConvertExample2_Person
        extends EvolveCase {

        private static final String NAME =
            ConvertExample2_Person.class.getName();
        private static final String NAME2 =
            ConvertExample2_Address .class.getName();

        @PrimaryKey
        int key;

        ConvertExample2_Address address;

        @Override
        Mutations getMutations() {
            Mutations m = new Mutations();
            Converter converter = new Converter
                (ConvertExample2_Person.class.getName(), 0,
                 "address", new ConvertExample2_Conversion());
            m.addConverter(converter);
            return m;
        }

        @Override
        void checkEvolvedModel(EntityModel model,
                               Environment env,
                               boolean oldTypesExist) {
            checkEntity(true, model, env, NAME, 1, null);
            if (oldTypesExist) {
                checkVersions(model, NAME, 1, NAME, 0);
            } else {
                checkVersions(model, NAME, 1);
            }
            checkVersions(model, NAME2, 0);
        }

        @Override
        void readObjects(EntityStore store, boolean doUpdate)
            throws DatabaseException {

            PrimaryIndex<Integer,ConvertExample2_Person>
                index = store.getPrimaryIndex
                    (Integer.class,
                     ConvertExample2_Person.class);
            ConvertExample2_Person obj = index.get(99);
            TestCase.assertNotNull(obj);
            TestCase.assertEquals(99, obj.key);
            TestCase.assertNotNull(obj.address);
            TestCase.assertEquals("street", obj.address.street);
            TestCase.assertEquals("city", obj.address.city);
            TestCase.assertEquals("state", obj.address.state);
            TestCase.assertEquals(12345, obj.address.zipCode);

            if (doUpdate) {
                index.put(obj);
            }
        }

        @Override
        void copyRawObjects(RawStore rawStore, EntityStore newStore)
            throws DatabaseException {

            PrimaryIndex<Integer,ConvertExample2_Person>
                index = newStore.getPrimaryIndex
                    (Integer.class,
                     ConvertExample2_Person.class);
            RawObject raw = rawStore.getPrimaryIndex(NAME).get(99);
            index.put((ConvertExample2_Person)
                      newStore.getModel().convertRawObject(raw));
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            Object embed;
            if (expectEvolved) {
                RawType embedType = store.getModel().getRawType(NAME2);
                embed = new RawObject
                    (embedType,
                     makeValues("street", "street",
                                "city", "city",
                                "state", "state",
                                "zipCode", 12345),
                     null);
            } else {
                embed = "street#city#state#12345";
            }
            RawObject obj = readRaw
                (store, 99, NAME, expectEvolved ? 1 : 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "address", embed);
        }
    }

    @SuppressWarnings("serial")
    static class ConvertExample2_Conversion implements Conversion {
        private transient RawType addressType;

        public void initialize(EntityModel model) {
            addressType = model.getRawType
                (ConvertExample2_Address.class.getName());
        }

        public Object convert(Object fromValue) {

            String oldAddress = (String) fromValue;
            Map<String,Object> addressValues = new HashMap<String,Object>();
            addressValues.put("street", parseAddress(1, oldAddress));
            addressValues.put("city", parseAddress(2, oldAddress));
            addressValues.put("state", parseAddress(3, oldAddress));
            addressValues.put("zipCode",
                              Integer.valueOf(parseAddress(4, oldAddress)));

            return new RawObject(addressType, addressValues, null);
        }

        @Override
        public boolean equals(Object o) {
            return o instanceof ConvertExample2_Conversion;
        }

        private String parseAddress(int fieldNum, String oldAddress) {
            StringTokenizer tokens = new StringTokenizer(oldAddress, "#");
            String field = null;
            for (int i = 0; i < fieldNum; i += 1) {
                field = tokens.nextToken();
            }
            return field;
        }
    }

    @Persistent
    static class ConvertExample3_Address {
        String street;
        String city;
        String state;
        int zipCode;
    }

    @SuppressWarnings("serial")
    static class ConvertExample3_Conversion implements Conversion {
        private transient RawType newPersonType;
        private transient RawType addressType;

        public void initialize(EntityModel model) {
            newPersonType = model.getRawType
                (ConvertExample3_Person.class.getName());
            addressType = model.getRawType
                (ConvertExample3_Address.class.getName());
        }

        public Object convert(Object fromValue) {

            RawObject person = (RawObject) fromValue;
            Map<String,Object> personValues = person.getValues();
            Map<String,Object> addressValues = new HashMap<String,Object>();
            RawObject address = new RawObject
                (addressType, addressValues, null);

            addressValues.put("street", personValues.remove("street"));
            addressValues.put("city", personValues.remove("city"));
            addressValues.put("state", personValues.remove("state"));
            addressValues.put("zipCode", personValues.remove("zipCode"));
            personValues.put("address", address);

            return new RawObject
                (newPersonType, personValues, person.getSuper());
        }

        @Override
        public boolean equals(Object o) {
            return o instanceof ConvertExample3_Conversion;
        }
    }

    @Entity(version=1)
    static class ConvertExample3_Person
        extends EvolveCase {

        private static final String NAME =
            ConvertExample3_Person.class.getName();
        private static final String NAME2 =
            ConvertExample3_Address .class.getName();

        @PrimaryKey
        int key;

        ConvertExample3_Address address;

        @Override
        Mutations getMutations() {
            Mutations m = new Mutations();
            Converter converter = new Converter
                (ConvertExample3_Person.class.getName(), 0,
                 new ConvertExample3_Conversion());
            m.addConverter(converter);
            return m;
        }

        @Override
        void checkEvolvedModel(EntityModel model,
                               Environment env,
                               boolean oldTypesExist) {
            checkEntity(true, model, env, NAME, 1, null);
            if (oldTypesExist) {
                checkVersions(model, NAME, 1, NAME, 0);
            } else {
                checkVersions(model, NAME, 1);
            }
            checkVersions(model, NAME2, 0);
        }

        @Override
        void readObjects(EntityStore store, boolean doUpdate)
            throws DatabaseException {

            PrimaryIndex<Integer,ConvertExample3_Person>
                index = store.getPrimaryIndex
                    (Integer.class,
                     ConvertExample3_Person.class);
            ConvertExample3_Person obj = index.get(99);
            TestCase.assertNotNull(obj);
            TestCase.assertEquals(99, obj.key);
            TestCase.assertNotNull(obj.address);
            TestCase.assertEquals("street", obj.address.street);
            TestCase.assertEquals("city", obj.address.city);
            TestCase.assertEquals("state", obj.address.state);
            TestCase.assertEquals(12345, obj.address.zipCode);

            if (doUpdate) {
                index.put(obj);
            }
        }

        @Override
        void copyRawObjects(RawStore rawStore, EntityStore newStore)
            throws DatabaseException {

            PrimaryIndex<Integer,ConvertExample3_Person>
                index = newStore.getPrimaryIndex
                    (Integer.class,
                     ConvertExample3_Person.class);
            RawObject raw = rawStore.getPrimaryIndex(NAME).get(99);
            index.put((ConvertExample3_Person)
                      newStore.getModel().convertRawObject(raw));
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            RawObject obj = readRaw
                (store, 99, NAME, expectEvolved ? 1 : 0, CASECLS, 0);
            if (expectEvolved) {
                RawType embedType = store.getModel().getRawType(NAME2);
                Object embed = new RawObject
                    (embedType,
                     makeValues("street", "street",
                                "city", "city",
                                "state", "state",
                                "zipCode", 12345),
                     null);
                checkRawFields(obj, "key", 99, "address", embed);
            } else {
                checkRawFields(obj, "key", 99,
                                    "street", "street",
                                    "city", "city",
                                    "state", "state",
                                    "zipCode", 12345);
            }
        }
    }

    @SuppressWarnings("serial")
    static class ConvertExample3Reverse_Conversion implements Conversion {
        private transient RawType newPersonType;

        public void initialize(EntityModel model) {
            newPersonType = model.getRawType
                (ConvertExample3Reverse_Person.class.getName());
        }

        public Object convert(Object fromValue) {

            RawObject person = (RawObject) fromValue;
            Map<String,Object> personValues = person.getValues();
            RawObject address = (RawObject) personValues.remove("address");
            Map<String,Object> addressValues = address.getValues();

            personValues.put("street", addressValues.remove("street"));
            personValues.put("city", addressValues.remove("city"));
            personValues.put("state", addressValues.remove("state"));
            personValues.put("zipCode", addressValues.remove("zipCode"));

            return new RawObject
                (newPersonType, personValues, person.getSuper());
        }

        @Override
        public boolean equals(Object o) {
            return o instanceof ConvertExample3Reverse_Conversion;
        }
    }

    @Entity(version=1)
    static class ConvertExample3Reverse_Person
        extends EvolveCase {

        private static final String NAME =
            ConvertExample3Reverse_Person.class.getName();
        private static final String NAME2 =
            PREFIX + "ConvertExample3Reverse_Address";

        @PrimaryKey
        int key;

        String street;
        String city;
        String state;
        int zipCode;

        @Override
        Mutations getMutations() {
            Mutations m = new Mutations();
            Converter converter = new Converter
                (ConvertExample3Reverse_Person.class.getName(), 0,
                 new ConvertExample3Reverse_Conversion());
            m.addConverter(converter);
            m.addDeleter(new Deleter(NAME2, 0));
            return m;
        }

        @Override
        void checkEvolvedModel(EntityModel model,
                               Environment env,
                               boolean oldTypesExist) {
            checkEntity(true, model, env, NAME, 1, null);
            if (oldTypesExist) {
                checkVersions(model, NAME, 1, NAME, 0);
                checkVersions(model, NAME2, 0);
            } else {
                checkVersions(model, NAME, 1);
            }
        }

        @Override
        void readObjects(EntityStore store, boolean doUpdate)
            throws DatabaseException {

            PrimaryIndex<Integer,ConvertExample3Reverse_Person>
                index = store.getPrimaryIndex
                    (Integer.class,
                     ConvertExample3Reverse_Person.class);
            ConvertExample3Reverse_Person obj = index.get(99);
            TestCase.assertNotNull(obj);
            TestCase.assertEquals(99, obj.key);
            TestCase.assertEquals("street", obj.street);
            TestCase.assertEquals("city", obj.city);
            TestCase.assertEquals("state", obj.state);
            TestCase.assertEquals(12345, obj.zipCode);

            if (doUpdate) {
                index.put(obj);
            }
        }

        @Override
        void copyRawObjects(RawStore rawStore, EntityStore newStore)
            throws DatabaseException {

            PrimaryIndex<Integer,ConvertExample3Reverse_Person>
                index = newStore.getPrimaryIndex
                    (Integer.class,
                     ConvertExample3Reverse_Person.class);
            RawObject raw = rawStore.getPrimaryIndex(NAME).get(99);
            index.put((ConvertExample3Reverse_Person)
                      newStore.getModel().convertRawObject(raw));
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            RawObject obj = readRaw
                (store, 99, NAME, expectEvolved ? 1 : 0, CASECLS, 0);
            if (expectEvolved) {
                checkRawFields(obj, "key", 99,
                                    "street", "street",
                                    "city", "city",
                                    "state", "state",
                                    "zipCode", 12345);
            } else {
                RawType embedType = store.getModel().getRawType(NAME2);
                Object embed = new RawObject
                    (embedType,
                     makeValues("street", "street",
                                "city", "city",
                                "state", "state",
                                "zipCode", 12345),
                     null);
                checkRawFields(obj, "key", 99, "address", embed);
            }
        }
    }

    @Persistent(version=1)
    static class ConvertExample4_A extends ConvertExample4_B {
    }

    @Persistent(version=1)
    static class ConvertExample4_B {
        String name;
    }

    @SuppressWarnings("serial")
    static class Example4_Conversion implements Conversion {
        private transient RawType newAType;
        private transient RawType newBType;

        public void initialize(EntityModel model) {
            newAType = model.getRawType(ConvertExample4_A.class.getName());
            newBType = model.getRawType(ConvertExample4_B.class.getName());
        }

        public Object convert(Object fromValue) {
            RawObject oldA = (RawObject) fromValue;
            RawObject oldB = oldA.getSuper();
            Map<String,Object> aValues = oldA.getValues();
            Map<String,Object> bValues = oldB.getValues();
            bValues.put("name", aValues.remove("name"));
            RawObject newB = new RawObject(newBType, bValues, oldB.getSuper());
            RawObject newA = new RawObject(newAType, aValues, newB);
            return newA;
        }

        @Override
        public boolean equals(Object o) {
            return o instanceof Example4_Conversion;
        }
    }

    @Entity(version=1)
    static class ConvertExample4_Entity
        extends EvolveCase {

        private static final String NAME =
            ConvertExample4_Entity.class.getName();
        private static final String NAME2 =
            ConvertExample4_A .class.getName();
        private static final String NAME3 =
            ConvertExample4_B .class.getName();

        @PrimaryKey
        int key;

        ConvertExample4_A embed;

        @Override
        Mutations getMutations() {
            Mutations m = new Mutations();
            Converter converter = new Converter
                (ConvertExample4_A.class.getName(), 0,
                 new Example4_Conversion());
            m.addConverter(converter);
            return m;
        }

        @Override
        void checkEvolvedModel(EntityModel model,
                               Environment env,
                               boolean oldTypesExist) {
            checkEntity(true, model, env, NAME, 1, null);
            if (oldTypesExist) {
                checkVersions(model, NAME, 1, NAME, 0);
                checkVersions(model, NAME2, 1, NAME2, 0);
                checkVersions(model, NAME3, 1, NAME3, 0);
            } else {
                checkVersions(model, NAME, 1);
                checkVersions(model, NAME2, 1);
                checkVersions(model, NAME3, 1);
            }
        }

        @Override
        void readObjects(EntityStore store, boolean doUpdate)
            throws DatabaseException {

            PrimaryIndex<Integer,ConvertExample4_Entity>
                index = store.getPrimaryIndex
                    (Integer.class,
                     ConvertExample4_Entity.class);
            ConvertExample4_Entity obj = index.get(99);
            TestCase.assertNotNull(obj);
            TestCase.assertEquals(99, obj.key);
            TestCase.assertNotNull(obj.embed);
            TestCase.assertEquals("name", obj.embed.name);

            if (doUpdate) {
                index.put(obj);
            }
        }

        @Override
        void copyRawObjects(RawStore rawStore, EntityStore newStore)
            throws DatabaseException {

            PrimaryIndex<Integer,ConvertExample4_Entity>
                index = newStore.getPrimaryIndex
                    (Integer.class,
                     ConvertExample4_Entity.class);
            RawObject raw = rawStore.getPrimaryIndex(NAME).get(99);
            index.put((ConvertExample4_Entity)
                      newStore.getModel().convertRawObject(raw));
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            RawType embedTypeA = store.getModel().getRawType(NAME2);
            RawType embedTypeB = store.getModel().getRawType(NAME3);
            Object embed;
            if (expectEvolved) {
                embed = new RawObject(embedTypeA, makeValues(),
                        new RawObject
                            (embedTypeB, makeValues("name", "name"), null));
            } else {
                embed = new RawObject(embedTypeA, makeValues("name", "name"),
                        new RawObject
                            (embedTypeB, makeValues(), null));
            }
            RawObject obj = readRaw
                (store, 99, NAME, expectEvolved ? 1 : 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "embed", embed);
        }
    }

    @Persistent(version=1)
    static class ConvertExample5_Pet {
        String name;
    }

    @Persistent
    static class ConvertExample5_Cat extends ConvertExample5_Pet {
        int finickyLevel;
    }

    @Persistent
    static class ConvertExample5_Dog extends ConvertExample5_Pet {
        double barkVolume;
    }

    @SuppressWarnings("serial")
    static class ConvertExample5_Conversion implements Conversion {
        private transient RawType newPetType;
        private transient RawType dogType;
        private transient RawType catType;

        public void initialize(EntityModel model) {
            newPetType = model.getRawType(ConvertExample5_Pet.class.getName());
            dogType = model.getRawType(ConvertExample5_Dog.class.getName());
            catType = model.getRawType(ConvertExample5_Cat.class.getName());
        }

        public Object convert(Object fromValue) {
            RawObject pet = (RawObject) fromValue;
            Map<String,Object> petValues = pet.getValues();
            Map<String,Object> subTypeValues = new HashMap<String,Object>();
            Boolean isCat = (Boolean) petValues.remove("isCatNotDog");
            Integer finickyLevel = (Integer) petValues.remove("finickyLevel");
            Double barkVolume = (Double) petValues.remove("barkVolume");
            RawType newSubType;
            if (isCat) {
                newSubType = catType;
                subTypeValues.put("finickyLevel", finickyLevel);
            } else {
                newSubType = dogType;
                subTypeValues.put("barkVolume", barkVolume);
            }
            RawObject newPet = new RawObject
                (newPetType, petValues, pet.getSuper());
            return new RawObject(newSubType, subTypeValues, newPet);
        }

        @Override
        public boolean equals(Object o) {
            return o instanceof ConvertExample5_Conversion;
        }
    }

    @Entity(version=1)
    static class ConvertExample5_Entity
        extends EvolveCase {

        private static final String NAME =
            ConvertExample5_Entity.class.getName();
        private static final String NAME2 =
            ConvertExample5_Pet.class.getName();
        private static final String NAME3 =
            ConvertExample5_Cat.class.getName();
        private static final String NAME4 =
            ConvertExample5_Dog.class.getName();

        @PrimaryKey
        int key;

        ConvertExample5_Cat cat;
        ConvertExample5_Dog dog;

        @Override
        Mutations getMutations() {
            Mutations m = new Mutations();
            Converter converter = new Converter
                (ConvertExample5_Pet.class.getName(), 0,
                 new ConvertExample5_Conversion());
            m.addConverter(converter);
            return m;
        }

        @Override
        void checkEvolvedModel(EntityModel model,
                               Environment env,
                               boolean oldTypesExist) {
            checkEntity(true, model, env, NAME, 1, null);
            if (oldTypesExist) {
                checkVersions(model, NAME, 1, NAME, 0);
                checkVersions(model, NAME2, 1, NAME2, 0);
            } else {
                checkVersions(model, NAME, 1);
                checkVersions(model, NAME2, 1);
            }
            checkVersions(model, NAME3, 0);
            checkVersions(model, NAME4, 0);
        }

        @Override
        void readObjects(EntityStore store, boolean doUpdate)
            throws DatabaseException {

            PrimaryIndex<Integer,ConvertExample5_Entity>
                index = store.getPrimaryIndex
                    (Integer.class,
                     ConvertExample5_Entity.class);
            ConvertExample5_Entity obj = index.get(99);
            TestCase.assertNotNull(obj);
            TestCase.assertEquals(99, obj.key);
            TestCase.assertNotNull(obj.cat);
            TestCase.assertEquals("Jeffry", obj.cat.name);
            TestCase.assertEquals(999, obj.cat.finickyLevel);
            TestCase.assertNotNull(obj.dog);
            TestCase.assertEquals("Nelson", obj.dog.name);
            TestCase.assertEquals(0.01, obj.dog.barkVolume);

            if (doUpdate) {
                index.put(obj);
            }
        }

        @Override
        void copyRawObjects(RawStore rawStore, EntityStore newStore)
            throws DatabaseException {

            PrimaryIndex<Integer,ConvertExample5_Entity>
                index = newStore.getPrimaryIndex
                    (Integer.class,
                     ConvertExample5_Entity.class);
            RawObject raw = rawStore.getPrimaryIndex(NAME).get(99);
            index.put((ConvertExample5_Entity)
                      newStore.getModel().convertRawObject(raw));
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            RawType petType = store.getModel().getRawType(NAME2);
            RawObject cat;
            RawObject dog;
            if (expectEvolved) {
                RawType catType = store.getModel().getRawType(NAME3);
                RawType dogType = store.getModel().getRawType(NAME4);
                cat = new RawObject(catType, makeValues("finickyLevel", 999),
                      new RawObject(petType, makeValues("name", "Jeffry"),
                                    null));
                dog = new RawObject(dogType, makeValues("barkVolume", 0.01),
                      new RawObject(petType, makeValues("name", "Nelson"),
                                    null));
            } else {
                cat = new RawObject(petType, makeValues("name", "Jeffry",
                                                        "isCatNotDog", true,
                                                        "finickyLevel", 999,
                                                        "barkVolume", 0.0),
                                    null);
                dog = new RawObject(petType, makeValues("name", "Nelson",
                                                        "isCatNotDog", false,
                                                        "finickyLevel", 0,
                                                        "barkVolume", 0.01),
                                    null);
            }
            RawObject obj = readRaw
                (store, 99, NAME, expectEvolved ? 1 : 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "cat", cat, "dog", dog);
        }
    }

    @Persistent(version=1)
    static class AllowFieldAddDelete_Embed {
        private final String f0 = "0";
        private String f2;
        private final int f3 = 3;
        private String f4;
        private final int f5 = 5;
        private final String f8 = "8";
        private final int f9 = 9;
    }

    @Persistent(version=1)
    static class AllowFieldAddDelete_Base
        extends EvolveCase {

        private final String f0 = "0";
        private String f2;
        private final int f3 = 3;
        private String f4;
        private final int f5 = 5;
        private final String f8 = "8";
        private final int f9 = 9;
    }

    @Entity(version=1)
    static class AllowFieldAddDelete
        extends AllowFieldAddDelete_Base {

        private static final String NAME =
            AllowFieldAddDelete.class.getName();
        private static final String NAME2 =
            AllowFieldAddDelete_Base.class.getName();
        private static final String NAME3 =
            AllowFieldAddDelete_Embed.class.getName();

        @PrimaryKey
        int key;

        AllowFieldAddDelete_Embed embed;

        private final String f0 = "0";
        private String f2;
        private final int f3 = 3;
        private String f4;
        private final int f5 = 5;
        private final String f8 = "8";
        private final int f9 = 9;

        @Override
        Mutations getMutations() {
            Mutations m = new Mutations();
            for (String name : new String[] {NAME, NAME2, NAME3}) {
                m.addDeleter(new Deleter(name, 0, "f1"));
                m.addDeleter(new Deleter(name, 0, "f6"));
                m.addDeleter(new Deleter(name, 0, "f7"));
            }
            return m;
        }

        @Override
        void checkEvolvedModel(EntityModel model,
                               Environment env,
                               boolean oldTypesExist) {
            checkEntity(true, model, env, NAME, 1, null);
            if (oldTypesExist) {
                checkVersions(model, NAME, 1, NAME, 0);
                checkVersions(model, NAME2, 1, NAME2, 0);
                checkVersions(model, NAME3, 1, NAME3, 0);
            } else {
                checkVersions(model, NAME, 1);
                checkVersions(model, NAME2, 1);
                checkVersions(model, NAME3, 1);
            }
        }

        @Override
        void readObjects(EntityStore store, boolean doUpdate)
            throws DatabaseException {

            PrimaryIndex<Integer,AllowFieldAddDelete>
                index = store.getPrimaryIndex
                    (Integer.class,
                     AllowFieldAddDelete.class);
            AllowFieldAddDelete obj = index.get(99);
            TestCase.assertNotNull(obj);
            TestCase.assertEquals(99, obj.key);
            {
                AllowFieldAddDelete o = obj;

                TestCase.assertNotNull(o);
                TestCase.assertEquals("0", o.f0);
                TestCase.assertEquals("2", o.f2);
                TestCase.assertEquals(3, o.f3);
                TestCase.assertEquals("4", o.f4);
                TestCase.assertEquals(5, o.f5);
                TestCase.assertEquals("8", o.f8);
                TestCase.assertEquals(9, o.f9);
            }
            {
                AllowFieldAddDelete_Base o = obj;

                TestCase.assertNotNull(o);
                TestCase.assertEquals("0", o.f0);
                TestCase.assertEquals("2", o.f2);
                TestCase.assertEquals(3, o.f3);
                TestCase.assertEquals("4", o.f4);
                TestCase.assertEquals(5, o.f5);
                TestCase.assertEquals("8", o.f8);
                TestCase.assertEquals(9, o.f9);
            }
            {
                AllowFieldAddDelete_Embed o = obj.embed;

                TestCase.assertNotNull(o);
                TestCase.assertEquals("0", o.f0);
                TestCase.assertEquals("2", o.f2);
                TestCase.assertEquals(3, o.f3);
                TestCase.assertEquals("4", o.f4);
                TestCase.assertEquals(5, o.f5);
                TestCase.assertEquals("8", o.f8);
                TestCase.assertEquals(9, o.f9);
            }

            if (doUpdate) {
                index.put(obj);
            }
        }

        @Override
        void copyRawObjects(RawStore rawStore, EntityStore newStore)
            throws DatabaseException {

            PrimaryIndex<Integer,AllowFieldAddDelete>
                index = newStore.getPrimaryIndex
                    (Integer.class,
                     AllowFieldAddDelete.class);
            RawObject raw = rawStore.getPrimaryIndex(NAME).get(99);
            index.put((AllowFieldAddDelete)
                      newStore.getModel().convertRawObject(raw));
        }

        static final Object[] fixedFields0 = {
            "f1", 1,
            "f2", "2",
            "f4", "4",
            "f6", 6,
            "f7", "7",
        };

        static final Object[] fixedFields1 = {
            "f2", "2",
            "f4", "4",
        };

        static final Object[] fixedFields2 = {
            "f0", "0",
            "f2", "2",
            "f3", 3,
            "f4", "4",
            "f5", 5,
            "f8", "8",
            "f9", 9,
        };

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            RawType baseType = store.getModel().getRawType(NAME2);
            RawType embedType = store.getModel().getRawType(NAME3);

            Object[] ff;
            if (expectEvolved) {
                if (expectUpdated) {
                    ff = fixedFields2;
                } else {
                    ff = fixedFields1;
                }
            } else {
                ff = fixedFields0;
            }
            RawObject embed = new RawObject(embedType, makeValues(ff), null);
            RawObject obj = readRaw
                (store, 99, NAME, expectEvolved ? 1 : 0,
                            NAME2, expectEvolved ? 1 : 0,
                            CASECLS, 0);
            checkRaw(obj, ff, "key", 99, "embed", embed);
            checkRaw(obj.getSuper(), ff);
        }

        private void checkRaw(RawObject obj,
                              Object[] fixedFields,
                              Object... otherFields) {
            Object[] allFields =
                new Object[otherFields.length + fixedFields.length];
            System.arraycopy(otherFields, 0, allFields, 0, otherFields.length);
            System.arraycopy(fixedFields, 0, allFields,
                             otherFields.length, fixedFields.length);
            checkRawFields(obj, allFields);
        }
    }

    static class ProxiedClass {
        int data;

        ProxiedClass(int data) {
            this.data = data;
        }
    }

    @Persistent(version=1, proxyFor=ProxiedClass.class)
    static class ProxiedClass_Proxy implements PersistentProxy<ProxiedClass> {
        long data;

        public void initializeProxy(ProxiedClass o) {
            data = o.data;
        }

        public ProxiedClass convertProxy() {
            return new ProxiedClass((int) data);
        }
    }

    @Entity
    static class ProxiedClass_Entity
        extends EvolveCase {

        private static final String NAME =
            ProxiedClass_Entity.class.getName();
        private static final String NAME2 =
            ProxiedClass_Proxy.class.getName();

        @PrimaryKey
        int key;

        ProxiedClass embed;

        @Override
        void configure(EntityModel model, StoreConfig config) {
            model.registerClass(ProxiedClass_Proxy.class);
        }

        @Override
        void checkEvolvedModel(EntityModel model,
                               Environment env,
                               boolean oldTypesExist) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
            if (oldTypesExist) {
                checkVersions(model, NAME2, 1, NAME2, 0);
            } else {
                checkVersions(model, NAME2, 1);
            }
        }

        @Override
        void readObjects(EntityStore store, boolean doUpdate)
            throws DatabaseException {

            PrimaryIndex<Integer,ProxiedClass_Entity>
                index = store.getPrimaryIndex
                    (Integer.class,
                     ProxiedClass_Entity.class);
            ProxiedClass_Entity obj = index.get(99);
            TestCase.assertNotNull(obj);
            TestCase.assertEquals(99, obj.key);
            TestCase.assertNotNull(obj.embed);
            TestCase.assertEquals(88, obj.embed.data);

            if (doUpdate) {
                index.put(obj);
            }
        }

        @Override
        void copyRawObjects(RawStore rawStore, EntityStore newStore)
            throws DatabaseException {

            PrimaryIndex<Integer,ProxiedClass_Entity>
                index = newStore.getPrimaryIndex
                    (Integer.class,
                     ProxiedClass_Entity.class);
            RawObject raw = rawStore.getPrimaryIndex(NAME).get(99);
            index.put((ProxiedClass_Entity)
                      newStore.getModel().convertRawObject(raw));
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            RawType embedType = store.getModel().getRawType(NAME2);
            RawObject embed;
            if (expectEvolved) {
                embed = new RawObject
                    (embedType, makeValues("data", 88L), null);
            } else {
                embed = new RawObject
                    (embedType, makeValues("data", 88), null);
            }
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "embed", embed);
        }
    }

    @Persistent(proxyFor=StringBuffer.class)
    static class DisallowChangeProxyFor_Proxy2
        implements PersistentProxy<StringBuffer> {

        String data;

        public void initializeProxy(StringBuffer o) {
            data = o.toString();
        }

        public StringBuffer convertProxy() {
            return new StringBuffer(data);
        }
    }

    @Persistent(proxyFor=StringBuilder.class)
    static class DisallowChangeProxyFor_Proxy
        implements PersistentProxy<StringBuilder> {

        String data;

        public void initializeProxy(StringBuilder o) {
            data = o.toString();
        }

        public StringBuilder convertProxy() {
            return new StringBuilder(data);
        }
    }

    @Entity
    static class DisallowChangeProxyFor
        extends EvolveCase {

        @PrimaryKey
        int key;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Error when evolving class: java.lang.StringBuffer version: 0 to class: java.lang.StringBuffer version: 0 Error: The proxy class for this type has been changed from: com.sleepycat.persist.test.EvolveClasses$DisallowChangeProxyFor_Proxy to: com.sleepycat.persist.test.EvolveClasses$DisallowChangeProxyFor_Proxy2";
        }

        @Override
        void configure(EntityModel model, StoreConfig config) {
            model.registerClass(DisallowChangeProxyFor_Proxy.class);
            model.registerClass(DisallowChangeProxyFor_Proxy2.class);
        }
    }

    @Persistent
    static class DisallowDeleteProxyFor_Proxy {
        String data;
    }

    @Entity
    static class DisallowDeleteProxyFor
        extends EvolveCase {

        @PrimaryKey
        int key;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Mutation is missing to evolve class: java.lang.StringBuffer version: 0 Error: java.lang.IllegalArgumentException: Class could not be loaded or is not persistent: java.lang.StringBuffer";
        }
    }

    @Persistent(version=1)
    static class ArrayNameChange_Component_Renamed {

        long data;
    }

    @Entity
    static class ArrayNameChange_Entity
        extends EvolveCase {

        private static final String NAME =
            ArrayNameChange_Entity.class.getName();
        private static final String NAME2 =
            ArrayNameChange_Component_Renamed.class.getName();
        private static final String NAME3 =
            PREFIX + "ArrayNameChange_Component";

        @PrimaryKey
        int key;

        ArrayNameChange_Component_Renamed[] embed;
        ArrayNameChange_Component_Renamed embed2;

        @Override
        Mutations getMutations() {
            Mutations m = new Mutations();
            m.addRenamer(new Renamer(NAME3, 0, NAME2));
            return m;
        }

        @Override
        void checkEvolvedModel(EntityModel model,
                               Environment env,
                               boolean oldTypesExist) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
            if (oldTypesExist) {
                checkVersions(model, NAME2, 1, NAME3, 0);
            } else {
                checkVersions(model, NAME2, 1);
            }
        }

        @Override
        void readObjects(EntityStore store, boolean doUpdate)
            throws DatabaseException {

            PrimaryIndex<Integer,ArrayNameChange_Entity>
                index = store.getPrimaryIndex
                    (Integer.class,
                     ArrayNameChange_Entity.class);
            ArrayNameChange_Entity obj = index.get(99);
            TestCase.assertNotNull(obj);
            TestCase.assertEquals(99, obj.key);
            TestCase.assertNotNull(obj.embed);
            TestCase.assertEquals(1, obj.embed.length);
            TestCase.assertEquals(88L, obj.embed[0].data);
            TestCase.assertSame(obj.embed2, obj.embed[0]);

            if (doUpdate) {
                index.put(obj);
            }
        }

        @Override
        void copyRawObjects(RawStore rawStore, EntityStore newStore)
            throws DatabaseException {

            PrimaryIndex<Integer,ArrayNameChange_Entity>
                index = newStore.getPrimaryIndex
                    (Integer.class,
                     ArrayNameChange_Entity.class);
            RawObject raw = rawStore.getPrimaryIndex(NAME).get(99);
            index.put((ArrayNameChange_Entity)
                      newStore.getModel().convertRawObject(raw));
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            String compTypeName = expectEvolved ? NAME2 : NAME3;
            String arrayTypeName = "[L" + compTypeName + ';';
            RawType compType = store.getModel().getRawType(compTypeName);
            RawType arrayType = store.getModel().getRawType(arrayTypeName);
            RawObject embed2;
            if (expectEvolved) {
                embed2 = new RawObject
                    (compType, makeValues("data", 88L), null);
            } else {
                embed2 = new RawObject
                    (compType, makeValues("data", 88), null);
            }
            RawObject embed = new RawObject
                (arrayType, new Object[] { embed2 });
            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            checkRawFields(obj, "key", 99, "embed", embed, "embed2", embed2);
        }
    }

    enum AddEnumConstant_Enum {
        A, B, C;
    }

    @Entity(version=1)
    static class AddEnumConstant_Entity
        extends EvolveCase {

        private static final String NAME =
            AddEnumConstant_Entity.class.getName();
        private static final String NAME2 =
            AddEnumConstant_Enum.class.getName();

        @PrimaryKey
        int key;

        AddEnumConstant_Enum e1;
        AddEnumConstant_Enum e2;
        AddEnumConstant_Enum e3 = AddEnumConstant_Enum.C;

        @Override
        void checkEvolvedModel(EntityModel model,
                               Environment env,
                               boolean oldTypesExist) {
            checkEntity(true, model, env, NAME, 1, null);
            if (oldTypesExist) {
                checkVersions(model, NAME, 1, NAME, 0);
                checkVersions(model, NAME2, 0, NAME2, 0);
            } else {
                checkVersions(model, NAME, 1);
                checkVersions(model, NAME2, 0);
            }
        }

        @Override
        void readObjects(EntityStore store, boolean doUpdate)
            throws DatabaseException {

            PrimaryIndex<Integer,AddEnumConstant_Entity>
                index = store.getPrimaryIndex
                    (Integer.class,
                     AddEnumConstant_Entity.class);
            AddEnumConstant_Entity obj = index.get(99);
            TestCase.assertNotNull(obj);
            TestCase.assertEquals(99, obj.key);
            TestCase.assertSame(AddEnumConstant_Enum.A, obj.e1);
            TestCase.assertSame(AddEnumConstant_Enum.B, obj.e2);
            TestCase.assertSame(AddEnumConstant_Enum.C, obj.e3);

            if (doUpdate) {
                index.put(obj);
            }
        }

        @Override
        void copyRawObjects(RawStore rawStore, EntityStore newStore)
            throws DatabaseException {

            PrimaryIndex<Integer,AddEnumConstant_Entity>
                index = newStore.getPrimaryIndex
                    (Integer.class,
                     AddEnumConstant_Entity.class);
            RawObject raw = rawStore.getPrimaryIndex(NAME).get(99);
            index.put((AddEnumConstant_Entity)
                      newStore.getModel().convertRawObject(raw));
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            RawObject obj = readRaw
                (store, 99, NAME, expectEvolved ? 1 : 0, CASECLS, 0);
            RawType enumType = store.getModel().getRawType(NAME2);
            if (expectUpdated) {
                checkRawFields(obj, "key", 99,
                               "e1", new RawObject(enumType, "A"),
                               "e2", new RawObject(enumType, "B"),
                               "e3", new RawObject(enumType, "C"));
            } else {
                checkRawFields(obj, "key", 99,
                               "e1", new RawObject(enumType, "A"),
                               "e2", new RawObject(enumType, "B"));
            }
        }
    }

    enum InsertEnumConstant_Enum {
        X, A, Y, B, Z;
    }

    @Persistent
    static class InsertEnumConstant_KeyClass
        implements Comparable<InsertEnumConstant_KeyClass > {

        @KeyField(1)
        InsertEnumConstant_Enum key;

        private InsertEnumConstant_KeyClass() {}

        InsertEnumConstant_KeyClass(InsertEnumConstant_Enum key) {
            this.key = key;
        }

        public int compareTo(InsertEnumConstant_KeyClass o) {
            /* Use the natural order, in spite of insertions. */
            return key.compareTo(o.key);
        }
    }

    @Entity(version=1)
    static class InsertEnumConstant_Entity
        extends EvolveCase {

        private static final String NAME =
            InsertEnumConstant_Entity.class.getName();
        private static final String NAME2 =
            InsertEnumConstant_Enum.class.getName();
        private static final String NAME3 =
            InsertEnumConstant_KeyClass.class.getName();

        @PrimaryKey
        int key;

        @SecondaryKey(relate=MANY_TO_ONE)
        InsertEnumConstant_KeyClass secKey;

        InsertEnumConstant_Enum e1;
        InsertEnumConstant_Enum e2;
        InsertEnumConstant_Enum e3 = InsertEnumConstant_Enum.X;
        InsertEnumConstant_Enum e4 = InsertEnumConstant_Enum.Y;
        InsertEnumConstant_Enum e5 = InsertEnumConstant_Enum.Z;

        @Override
        void checkEvolvedModel(EntityModel model,
                               Environment env,
                               boolean oldTypesExist) {
            checkEntity(true, model, env, NAME, 1, null);
            if (oldTypesExist) {
                checkVersions(model, NAME, 1, NAME, 0);
                checkVersions(model, NAME2, 0, NAME2, 0);
                checkVersions(model, NAME3, 0, NAME3, 0);
            } else {
                checkVersions(model, NAME, 1);
                checkVersions(model, NAME2, 0);
                checkVersions(model, NAME3, 0);
            }
        }

        @Override
        void readObjects(EntityStore store, boolean doUpdate)
            throws DatabaseException {

            PrimaryIndex<Integer,InsertEnumConstant_Entity>
                index = store.getPrimaryIndex
                    (Integer.class,
                     InsertEnumConstant_Entity.class);
            InsertEnumConstant_Entity obj = index.get(99);
            TestCase.assertNotNull(obj);
            TestCase.assertEquals(99, obj.key);
            if (updated) {
                TestCase.assertSame(InsertEnumConstant_Enum.X, obj.secKey.key);
            } else {
                TestCase.assertSame(InsertEnumConstant_Enum.A, obj.secKey.key);
            }
            TestCase.assertSame(InsertEnumConstant_Enum.A, obj.e1);
            TestCase.assertSame(InsertEnumConstant_Enum.B, obj.e2);
            TestCase.assertSame(InsertEnumConstant_Enum.X, obj.e3);
            TestCase.assertSame(InsertEnumConstant_Enum.Y, obj.e4);
            TestCase.assertSame(InsertEnumConstant_Enum.Z, obj.e5);

            if (doUpdate) {
                obj.secKey =
                    new InsertEnumConstant_KeyClass(InsertEnumConstant_Enum.X);
                index.put(obj);
                updated = true;
            }
        }

        @Override
        void copyRawObjects(RawStore rawStore, EntityStore newStore)
            throws DatabaseException {

            PrimaryIndex<Integer,InsertEnumConstant_Entity>
                index = newStore.getPrimaryIndex
                    (Integer.class,
                     InsertEnumConstant_Entity.class);
            RawObject raw = rawStore.getPrimaryIndex(NAME).get(99);
            index.put((InsertEnumConstant_Entity)
                      newStore.getModel().convertRawObject(raw));
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            RawObject obj = readRaw
                (store, 99, NAME, expectEvolved ? 1 : 0, CASECLS, 0);
            RawType enumType = store.getModel().getRawType(NAME2);

            Map<String, Object> secKeyFields = new HashMap<String, Object>();
            RawType secKeyType = store.getModel().getRawType(NAME3);
            RawObject secKeyObject =
                new RawObject(secKeyType, secKeyFields, null /*superObject*/);

            if (expectUpdated) {
                secKeyFields.put("key", new RawObject(enumType, "X"));
                checkRawFields(obj, "key", 99,
                               "secKey", secKeyObject,
                               "e1", new RawObject(enumType, "A"),
                               "e2", new RawObject(enumType, "B"),
                               "e3", new RawObject(enumType, "X"),
                               "e4", new RawObject(enumType, "Y"),
                               "e5", new RawObject(enumType, "Z"));
            } else {
                secKeyFields.put("key", new RawObject(enumType, "A"));
                checkRawFields(obj, "key", 99,
                               "secKey", secKeyObject,
                               "e1", new RawObject(enumType, "A"),
                               "e2", new RawObject(enumType, "B"));
            }
        }
    }

    enum DeleteEnumConstant_Enum {
        A, C;
    }

    /**
     * Don't allow deleting (or renaming, which appears as a deletion) enum
     * values without mutations.
     */
    @Entity
    static class DeleteEnumConstant_NoMutation
        extends EvolveCase {

        @PrimaryKey
        int key;

        DeleteEnumConstant_Enum e1;
        DeleteEnumConstant_Enum e2;
        DeleteEnumConstant_Enum e3;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Incompatible enum type changed detected when evolving class: com.sleepycat.persist.test.EvolveClasses$DeleteEnumConstant_Enum version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DeleteEnumConstant_Enum version: 0 Error: Enum values may not be removed: [B]";
        }
    }

    /**
     * With a Deleter, deleted enum values are null.  Note that version is not
     * bumped.
     */
    /* Disabled until support for enum deletion is added.
    @Entity
    static class DeleteEnumConstant_WithDeleter
        extends EvolveCase {

        private static final String NAME =
            DeleteEnumConstant_WithDeleter.class.getName();
        private static final String NAME2 =
            DeleteEnumConstant_Enum.class.getName();

        @PrimaryKey
        int key;

        DeleteEnumConstant_Enum e1;
        DeleteEnumConstant_Enum e2;
        DeleteEnumConstant_Enum e3;

        @Override
        void checkEvolvedModel(EntityModel model,
                               Environment env,
                               boolean oldTypesExist) {
            checkEntity(true, model, env, NAME, 0, null);
            checkVersions(model, NAME, 0);
            if (oldTypesExist) {
                checkVersions(model, NAME2, 0, NAME2, 0);
            } else {
                checkVersions(model, NAME2, 0);
            }
        }

        @Override
        void readObjects(EntityStore store, boolean doUpdate)
            throws DatabaseException {

            PrimaryIndex<Integer,DeleteEnumConstant_WithDeleter>
                index = store.getPrimaryIndex
                    (Integer.class,
                     DeleteEnumConstant_WithDeleter.class);
            DeleteEnumConstant_WithDeleter obj = index.get(99);
            TestCase.assertNotNull(obj);
            TestCase.assertEquals(99, obj.key);
            TestCase.assertSame(DeleteEnumConstant_Enum.A, obj.e1);
            TestCase.assertSame(null, obj.e2);
            TestCase.assertSame(DeleteEnumConstant_Enum.C, obj.e3);

            if (doUpdate) {
                index.put(obj);
            }
        }

        @Override
        void copyRawObjects(RawStore rawStore, EntityStore newStore)
            throws DatabaseException {

            PrimaryIndex<Integer,DeleteEnumConstant_WithDeleter>
                index = newStore.getPrimaryIndex
                    (Integer.class,
                     DeleteEnumConstant_WithDeleter.class);
            RawObject raw = rawStore.getPrimaryIndex(NAME).get(99);
            index.put((DeleteEnumConstant_WithDeleter)
                      newStore.getModel().convertRawObject(raw));
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            RawObject obj = readRaw(store, 99, NAME, 0, CASECLS, 0);
            RawType enumType = store.getModel().getRawType(NAME2);
            if (expectUpdated) {
                checkRawFields(obj, "key", 99,
                               "e1", new RawObject(enumType, "A"),
                               "e2", null,
                               "e3", new RawObject(enumType, "C"));
            } else {
                checkRawFields(obj, "key", 99,
                               "e1", new RawObject(enumType, "A"),
                               "e2", new RawObject(enumType, "B"),
                               "e3", new RawObject(enumType, "C"));
            }
        }
    }
    */

    /**
     * A field converter can assign deleted enum values.  Version must be 
     * bumped when a converter is added.
     */
    /* Disabled until support for enum deletion is added.
    @Entity(version=1)
    static class DeleteEnumConstant_WithConverter
        extends EvolveCase {

        private static final String NAME =
            DeleteEnumConstant_WithConverter.class.getName();
        private static final String NAME2 =
            DeleteEnumConstant_Enum.class.getName();

        @PrimaryKey
        int key;

        DeleteEnumConstant_Enum e1;
        DeleteEnumConstant_Enum e2;
        DeleteEnumConstant_Enum e3;

        @Override
        Mutations getMutations() {
            Mutations m = new Mutations();
            Conversion c = new MyConversion();
            m.addConverter(new Converter(NAME, 0, "e1", c));
            m.addConverter(new Converter(NAME, 0, "e2", c));
            m.addConverter(new Converter(NAME, 0, "e3", c));
            return m;
        }

        @SuppressWarnings("serial")
        static class MyConversion implements Conversion {

            transient RawType newType;

            public void initialize(EntityModel model) {
                newType = model.getRawType(NAME2);
                TestCase.assertNotNull(newType);
            }

            public Object convert(Object fromValue) {
                TestCase.assertNotNull(newType);
                RawObject obj = (RawObject) fromValue;
                String val = obj.getEnum();
                TestCase.assertNotNull(val);
                if ("B".equals(val)) {
                    val = "C";
                }
                return new RawObject(newType, val);
            }

            @Override
            public boolean equals(Object other) {
                return other instanceof MyConversion;
            }
        }

        @Override
        void checkEvolvedModel(EntityModel model,
                               Environment env,
                               boolean oldTypesExist) {
            checkEntity(true, model, env, NAME, 1, null);
            if (oldTypesExist) {
                checkVersions(model, NAME, 1, NAME, 0);
                checkVersions(model, NAME2, 0, NAME2, 0);
            } else {
                checkVersions(model, NAME, 1);
                checkVersions(model, NAME2, 0);
            }
        }

        @Override
        void readObjects(EntityStore store, boolean doUpdate)
            throws DatabaseException {

            PrimaryIndex<Integer,DeleteEnumConstant_WithConverter>
                index = store.getPrimaryIndex
                    (Integer.class,
                     DeleteEnumConstant_WithConverter.class);
            DeleteEnumConstant_WithConverter obj = index.get(99);
            TestCase.assertNotNull(obj);
            TestCase.assertEquals(99, obj.key);
            TestCase.assertSame(DeleteEnumConstant_Enum.A, obj.e1);
            TestCase.assertSame(DeleteEnumConstant_Enum.C, obj.e2);
            TestCase.assertSame(DeleteEnumConstant_Enum.C, obj.e3);

            if (doUpdate) {
                index.put(obj);
            }
        }

        @Override
        void copyRawObjects(RawStore rawStore, EntityStore newStore)
            throws DatabaseException {

            PrimaryIndex<Integer,DeleteEnumConstant_WithConverter>
                index = newStore.getPrimaryIndex
                    (Integer.class,
                     DeleteEnumConstant_WithConverter.class);
            RawObject raw = rawStore.getPrimaryIndex(NAME).get(99);
            index.put((DeleteEnumConstant_WithConverter)
                      newStore.getModel().convertRawObject(raw));
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            RawObject obj = readRaw(store, 99, NAME, expectEvolved ? 1 : 0,
                                    CASECLS, 0);
            RawType enumType = store.getModel().getRawType(NAME2);
            if (expectEvolved) {
                checkRawFields(obj, "key", 99,
                               "e1", new RawObject(enumType, "A"),
                               "e2", new RawObject(enumType, "C"),
                               "e3", new RawObject(enumType, "C"));
            } else {
                checkRawFields(obj, "key", 99,
                               "e1", new RawObject(enumType, "A"),
                               "e2", new RawObject(enumType, "B"),
                               "e3", new RawObject(enumType, "C"));
            }
        }
    }
    */

    @Entity
    static class DisallowChangeKeyRelate
        extends EvolveCase {

        private static final String NAME =
            DisallowChangeKeyRelate.class.getName();

        @PrimaryKey
        int key;

        @SecondaryKey(relate=MANY_TO_ONE)
        int skey;

        @Override
        public String getStoreOpenException() {
            return "com.sleepycat.persist.evolve.IncompatibleClassException: Change detected in the relate attribute (Relationship) of a secondary key when evolving class: com.sleepycat.persist.test.EvolveClasses$DisallowChangeKeyRelate version: 0 to class: com.sleepycat.persist.test.EvolveClasses$DisallowChangeKeyRelate version: 0 Error: Old key: skey relate: ONE_TO_ONE new key: skey relate: MANY_TO_ONE";
        }
    }

    @Entity(version=1)
    static class AllowChangeKeyMetadata
        extends EvolveCase {

        private static final String NAME =
            AllowChangeKeyMetadata.class.getName();

        @PrimaryKey
        int key;

        /*
         * Combined fields from version 0 and 1:
         *  addAnnotation = 88;
         *  dropField = 77;
         *  dropAnnotation = 66;
         *  addField = 55;
         *  renamedField = 44; // was toBeRenamedField
         *  aa = 33;
         *  ff = 22;
         */

        int aa;

        @SecondaryKey(relate=ONE_TO_ONE)
        int addAnnotation;

        int dropAnnotation;

        @SecondaryKey(relate=ONE_TO_ONE)
        Integer addField;

        @SecondaryKey(relate=ONE_TO_ONE)
        int renamedField;

        int ff;

        @Override
        Mutations getMutations() {
            Mutations m = new Mutations();
            m.addDeleter(new Deleter(NAME, 0, "dropField"));
            m.addRenamer(new Renamer(NAME, 0, "toBeRenamedField",
                                              "renamedField"));
            return m;
        }

        @Override
        void checkEvolvedModel(EntityModel model,
                               Environment env,
                               boolean oldTypesExist) {
            checkEntity(true, model, env, NAME, 1, null);
            if (oldTypesExist) {
                checkVersions(model, NAME, 1, NAME, 0);
            } else {
                checkVersions(model, NAME, 1);
            }
        }

        @Override
        void readObjects(EntityStore store, boolean doUpdate)
            throws DatabaseException {

            PrimaryIndex<Integer,AllowChangeKeyMetadata>
                index = store.getPrimaryIndex
                    (Integer.class,
                     AllowChangeKeyMetadata.class);
            AllowChangeKeyMetadata obj = index.get(99);
            checkValues(obj);

            checkValues(store.getSecondaryIndex
                (index, Integer.class, "addAnnotation").get(88));
            checkValues(store.getSecondaryIndex
                (index, Integer.class, "renamedField").get(44));
            if (updated) {
                checkValues(store.getSecondaryIndex
                    (index, Integer.class, "addField").get(55));
            } else {
                TestCase.assertNull(store.getSecondaryIndex
                    (index, Integer.class, "addField").get(55));
            }

            if (doUpdate) {
                obj.addField = 55;
                index.put(obj);
                updated = true;
                checkValues(store.getSecondaryIndex
                    (index, Integer.class, "addAnnotation").get(88));
                checkValues(store.getSecondaryIndex
                    (index, Integer.class, "addField").get(55));
            }
        }

        @Override
        void copyRawObjects(RawStore rawStore, EntityStore newStore)
            throws DatabaseException {

            PrimaryIndex<Integer,AllowChangeKeyMetadata>
                index = newStore.getPrimaryIndex
                    (Integer.class,
                     AllowChangeKeyMetadata.class);
            RawObject raw = rawStore.getPrimaryIndex(NAME).get(99);
            index.put((AllowChangeKeyMetadata)
                      newStore.getModel().convertRawObject(raw));
        }

        private void checkValues(AllowChangeKeyMetadata obj) {
            TestCase.assertNotNull(obj);
            TestCase.assertEquals(99, obj.key);
            TestCase.assertEquals(88, obj.addAnnotation);
            TestCase.assertEquals(66, obj.dropAnnotation);
            TestCase.assertEquals(44, obj.renamedField);
            TestCase.assertEquals(33, obj.aa);
            TestCase.assertEquals(22, obj.ff);
            if (updated) {
                TestCase.assertEquals(Integer.valueOf(55), obj.addField);
            } else {
                TestCase.assertNull(obj.addField);
            }
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            RawObject obj = readRaw
                (store, 99, NAME, expectEvolved ? 1 : 0, CASECLS, 0);
            if (expectUpdated) {
                checkRawFields(obj, "key", 99,
                               "addAnnotation", 88,
                               "dropAnnotation", 66,
                               "addField", 55,
                               "renamedField", 44,
                               "aa", 33,
                               "ff", 22);
            } else if (expectEvolved) {
                checkRawFields(obj, "key", 99,
                               "addAnnotation", 88,
                               "dropAnnotation", 66,
                               "renamedField", 44,
                               "aa", 33,
                               "ff", 22);
            } else {
                checkRawFields(obj, "key", 99,
                               "addAnnotation", 88,
                               "dropField", 77,
                               "dropAnnotation", 66,
                               "toBeRenamedField", 44,
                               "aa", 33,
                               "ff", 22);
            }
            Environment env = store.getEnvironment();
            assertDbExists(expectEvolved, env, NAME, "addAnnotation");
            assertDbExists(expectEvolved, env, NAME, "addField");
            assertDbExists(expectEvolved, env, NAME, "renamedField");
            assertDbExists(!expectEvolved, env, NAME, "toBeRenamedField");
            assertDbExists(!expectEvolved, env, NAME, "dropField");
            assertDbExists(!expectEvolved, env, NAME, "dropAnnotation");
        }
    }

    /**
     * Same test as AllowChangeKeyMetadata but with the secondary keys in an
     * entity subclass.  [#16253]
     */
    @Persistent(version=1)
    static class AllowChangeKeyMetadataInSubclass
        extends AllowChangeKeyMetadataEntity {

        private static final String NAME =
            AllowChangeKeyMetadataInSubclass.class.getName();
        private static final String NAME2 =
            AllowChangeKeyMetadataEntity.class.getName();

        /*
         * Combined fields from version 0 and 1:
         *  addAnnotation = 88;
         *  dropField = 77;
         *  dropAnnotation = 66;
         *  addField = 55;
         *  renamedField = 44; // was toBeRenamedField
         *  aa = 33;
         *  ff = 22;
         */

        int aa;

        @SecondaryKey(relate=ONE_TO_ONE)
        int addAnnotation;

        int dropAnnotation;

        @SecondaryKey(relate=ONE_TO_ONE)
        Integer addField;

        @SecondaryKey(relate=ONE_TO_ONE)
        int renamedField;

        int ff;

        @Override
        void configure(EntityModel model, StoreConfig config) {
            model.registerClass(AllowChangeKeyMetadataInSubclass.class);
        }

        @Override
        Mutations getMutations() {
            Mutations m = new Mutations();
            m.addDeleter(new Deleter(NAME, 0, "dropField"));
            m.addRenamer(new Renamer(NAME, 0, "toBeRenamedField",
                                              "renamedField"));
            return m;
        }

        @Override
        void checkEvolvedModel(EntityModel model,
                               Environment env,
                               boolean oldTypesExist) {
            checkNonEntity(true, model, env, NAME, 1);
            checkEntity(true, model, env, NAME2, 0, null);
            if (oldTypesExist) {
                checkVersions(model, NAME, 1, NAME, 0);
                checkVersions(model, NAME2, 0);
            } else {
                checkVersions(model, NAME, 1);
                checkVersions(model, NAME2, 0);
            }
        }

        @Override
        void readObjects(EntityStore store, boolean doUpdate)
            throws DatabaseException {

            PrimaryIndex<Integer,AllowChangeKeyMetadataEntity>
                index = store.getPrimaryIndex
                    (Integer.class,
                     AllowChangeKeyMetadataEntity.class);
            AllowChangeKeyMetadataEntity obj = index.get(99);
            checkValues(obj);

            checkValues(store.getSecondaryIndex
                (index, Integer.class, "addAnnotation").get(88));
            checkValues(store.getSecondaryIndex
                (index, Integer.class, "renamedField").get(44));
            if (updated) {
                checkValues(store.getSecondaryIndex
                    (index, Integer.class, "addField").get(55));
            } else {
                TestCase.assertNull(store.getSecondaryIndex
                    (index, Integer.class, "addField").get(55));
            }

            if (doUpdate) {
                ((AllowChangeKeyMetadataInSubclass) obj).addField = 55;
                index.put(obj);
                updated = true;
                checkValues(store.getSecondaryIndex
                    (index, Integer.class, "addAnnotation").get(88));
                checkValues(store.getSecondaryIndex
                    (index, Integer.class, "addField").get(55));
            }
        }

        @Override
        void copyRawObjects(RawStore rawStore, EntityStore newStore)
            throws DatabaseException {

            PrimaryIndex<Integer,AllowChangeKeyMetadataEntity>
                index = newStore.getPrimaryIndex
                    (Integer.class,
                     AllowChangeKeyMetadataEntity.class);
            RawObject raw = rawStore.getPrimaryIndex(NAME2).get(99);
            index.put((AllowChangeKeyMetadataInSubclass)
                      newStore.getModel().convertRawObject(raw));
        }

        private void checkValues(AllowChangeKeyMetadataEntity objParam) {
            AllowChangeKeyMetadataInSubclass obj =
                (AllowChangeKeyMetadataInSubclass) objParam;
            TestCase.assertNotNull(obj);
            TestCase.assertEquals(99, obj.key);
            TestCase.assertEquals(88, obj.addAnnotation);
            TestCase.assertEquals(66, obj.dropAnnotation);
            TestCase.assertEquals(44, obj.renamedField);
            TestCase.assertEquals(33, obj.aa);
            TestCase.assertEquals(22, obj.ff);
            if (updated) {
                TestCase.assertEquals(Integer.valueOf(55), obj.addField);
            } else {
                TestCase.assertNull(obj.addField);
            }
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            RawObject obj = readRaw
                (store, NAME2, 99, NAME, expectEvolved ? 1 : 0,
                 NAME2, 0, CASECLS, 0);
            checkRawFields(obj.getSuper(), "key", 99);
            if (expectUpdated) {
                checkRawFields(obj,
                               "addAnnotation", 88,
                               "dropAnnotation", 66,
                               "addField", 55,
                               "renamedField", 44,
                               "aa", 33,
                               "ff", 22);
            } else if (expectEvolved) {
                checkRawFields(obj,
                               "addAnnotation", 88,
                               "dropAnnotation", 66,
                               "renamedField", 44,
                               "aa", 33,
                               "ff", 22);
            } else {
                checkRawFields(obj,
                               "addAnnotation", 88,
                               "dropField", 77,
                               "dropAnnotation", 66,
                               "toBeRenamedField", 44,
                               "aa", 33,
                               "ff", 22);
            }
            Environment env = store.getEnvironment();
            assertDbExists(expectEvolved, env, NAME2, "addAnnotation");
            assertDbExists(expectEvolved, env, NAME2, "addField");
            assertDbExists(expectEvolved, env, NAME2, "renamedField");
            assertDbExists(!expectEvolved, env, NAME2, "toBeRenamedField");
            assertDbExists(!expectEvolved, env, NAME2, "dropField");
            assertDbExists(!expectEvolved, env, NAME2, "dropAnnotation");
        }
    }

    @Entity
    static class AllowChangeKeyMetadataEntity
        extends EvolveCase {

        @PrimaryKey
        int key;
    }

    /**
     * Special case of adding secondaries that caused
     * IndexOutOfBoundsException. [#15524]
     */
    @Entity(version=1)
    static class AllowAddSecondary
        extends EvolveCase {

        private static final String NAME =
            AllowAddSecondary.class.getName();

        @PrimaryKey
        long key;

        @SecondaryKey(relate=ONE_TO_ONE)
        int a;

        @SecondaryKey(relate=ONE_TO_ONE)
        int b;

        @Override
        void checkEvolvedModel(EntityModel model,
                               Environment env,
                               boolean oldTypesExist) {
            checkEntity(true, model, env, NAME, 1, null);
            if (oldTypesExist) {
                checkVersions(model, NAME, 1, NAME, 0);
            } else {
                checkVersions(model, NAME, 1);
            }
        }

        @Override
        void readObjects(EntityStore store, boolean doUpdate)
            throws DatabaseException {

            PrimaryIndex<Long,AllowAddSecondary>
                index = store.getPrimaryIndex
                    (Long.class,
                     AllowAddSecondary.class);
            AllowAddSecondary obj = index.get(99L);
            checkValues(obj);

            checkValues(store.getSecondaryIndex
                (index, Integer.class, "a").get(1));
            if (updated) {
                checkValues(store.getSecondaryIndex
                    (index, Integer.class, "b").get(3));
                TestCase.assertNull(store.getSecondaryIndex
                    (index, Integer.class, "b").get(2));
            } else {
                checkValues(store.getSecondaryIndex
                    (index, Integer.class, "b").get(2));
                TestCase.assertNull(store.getSecondaryIndex
                    (index, Integer.class, "b").get(3));
            }

            if (doUpdate) {
                obj.b = 3;
                index.put(obj);
                updated = true;
                checkValues(store.getSecondaryIndex
                    (index, Integer.class, "a").get(1));
                checkValues(store.getSecondaryIndex
                    (index, Integer.class, "b").get(3));
            }
        }

        @Override
        void copyRawObjects(RawStore rawStore, EntityStore newStore)
            throws DatabaseException {

            PrimaryIndex<Long,AllowAddSecondary>
                index = newStore.getPrimaryIndex
                    (Long.class,
                     AllowAddSecondary.class);
            RawObject raw = rawStore.getPrimaryIndex(NAME).get(99L);
            index.put((AllowAddSecondary)
                      newStore.getModel().convertRawObject(raw));
        }

        private void checkValues(AllowAddSecondary obj) {
            TestCase.assertNotNull(obj);
            TestCase.assertEquals(99L, obj.key);
            TestCase.assertEquals(1, obj.a);
            if (updated) {
                TestCase.assertEquals(3, obj.b);
            } else {
                TestCase.assertEquals(2, obj.b);
            }
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            RawObject obj = readRaw
                (store, 99L, NAME, expectEvolved ? 1 : 0, CASECLS, 0);
            if (expectUpdated) {
                checkRawFields(obj, "key", 99L,
                               "a", 1,
                               "b", 3);
            } else {
                checkRawFields(obj, "key", 99L,
                               "a", 1,
                               "b", 2);
            }
            Environment env = store.getEnvironment();
            assertDbExists(expectEvolved, env, NAME, "a");
            assertDbExists(expectEvolved, env, NAME, "b");
        }
    }

    @Entity(version=1)
    static class FieldAddAndConvert
        extends EvolveCase {

        private static final String NAME =
            FieldAddAndConvert.class.getName();

        @PrimaryKey
        int key;

        private final String f0 = "0"; // new field
        private final String f1 = "1"; // converted field
        private final String f2 = "2"; // new field
        private final String f3 = "3"; // converted field
        private final String f4 = "4"; // new field

        @Override
        Mutations getMutations() {
            Mutations m = new Mutations();
            m.addConverter(new Converter(NAME, 0, "f1", new IntToString()));
            m.addConverter(new Converter(NAME, 0, "f3", new IntToString()));
            return m;
        }

        @SuppressWarnings("serial")
        private static class IntToString implements Conversion {

            public void initialize(EntityModel model) {
            }

            public Object convert(Object fromValue) {
                return fromValue.toString();
            }

            @Override
            public boolean equals(Object other) {
                return other instanceof IntToString;
            }
        }

        @Override
        void checkEvolvedModel(EntityModel model,
                               Environment env,
                               boolean oldTypesExist) {
            checkEntity(true, model, env, NAME, 1, null);
            if (oldTypesExist) {
                checkVersions(model, NAME, 1, NAME, 0);
            } else {
                checkVersions(model, NAME, 1);
            }
        }

        @Override
        void readObjects(EntityStore store, boolean doUpdate)
            throws DatabaseException {

            PrimaryIndex<Integer,FieldAddAndConvert>
                index = store.getPrimaryIndex
                    (Integer.class,
                     FieldAddAndConvert.class);
            FieldAddAndConvert obj = index.get(99);
            TestCase.assertNotNull(obj);
            TestCase.assertEquals(99, obj.key);
            TestCase.assertEquals("0", obj.f0);
            TestCase.assertEquals("1", obj.f1);
            TestCase.assertEquals("2", obj.f2);
            TestCase.assertEquals("3", obj.f3);
            TestCase.assertEquals("4", obj.f4);

            if (doUpdate) {
                index.put(obj);
            }
        }

        @Override
        void copyRawObjects(RawStore rawStore, EntityStore newStore)
            throws DatabaseException {

            PrimaryIndex<Integer,FieldAddAndConvert>
                index = newStore.getPrimaryIndex
                    (Integer.class,
                     FieldAddAndConvert.class);
            RawObject raw = rawStore.getPrimaryIndex(NAME).get(99);
            index.put((FieldAddAndConvert)
                      newStore.getModel().convertRawObject(raw));
        }

        @Override
        void readRawObjects(RawStore store,
                            boolean expectEvolved,
                            boolean expectUpdated)
            throws DatabaseException {

            RawObject obj = readRaw
                (store, 99, NAME, expectEvolved ? 1 : 0, CASECLS, 0);
            if (expectUpdated) {
                checkRawFields(obj,
                               "key", 99,
                               "f0", "0",
                               "f1", "1",
                               "f2", "2",
                               "f3", "3",
                               "f4", "4");
            } else if (expectEvolved) {
                checkRawFields(obj,
                               "key", 99,
                               "f1", "1",
                               "f3", "3");
            } else {
                checkRawFields(obj,
                               "key", 99,
                               "f1", 1,
                               "f3", 3);
            }
        }
    }
}

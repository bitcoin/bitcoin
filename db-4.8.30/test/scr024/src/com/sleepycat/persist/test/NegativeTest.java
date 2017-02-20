/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.test;

import static com.sleepycat.persist.model.Relationship.ONE_TO_MANY;
import static com.sleepycat.persist.model.Relationship.ONE_TO_ONE;
import static com.sleepycat.persist.model.DeleteAction.NULLIFY;

import java.math.BigDecimal;
import java.util.ArrayList;
import java.util.Collection;

import junit.framework.Test;

import com.sleepycat.db.DatabaseException;
import com.sleepycat.persist.EntityStore;
import com.sleepycat.persist.PrimaryIndex;
import com.sleepycat.persist.StoreConfig;
import com.sleepycat.persist.model.AnnotationModel;
import com.sleepycat.persist.model.Entity;
import com.sleepycat.persist.model.KeyField;
import com.sleepycat.persist.model.Persistent;
import com.sleepycat.persist.model.PersistentProxy;
import com.sleepycat.persist.model.PrimaryKey;
import com.sleepycat.persist.model.SecondaryKey;
import com.sleepycat.util.test.TxnTestCase;

/**
 * Negative tests.
 *
 * @author Mark Hayes
 */
public class NegativeTest extends TxnTestCase {

    static protected Class<?> testClass = NegativeTest.class;

    public static Test suite() {
        return txnTestSuite(testClass, null, null);
    }

    private EntityStore store;

    private void open()
        throws DatabaseException {

        open(null);
    }

    private void open(Class<ProxyExtendsEntity> clsToRegister)
        throws DatabaseException {

        StoreConfig config = new StoreConfig();
        config.setAllowCreate(envConfig.getAllowCreate());
        config.setTransactional(envConfig.getTransactional());

        if (clsToRegister != null) {
            AnnotationModel model = new AnnotationModel();
            model.registerClass(clsToRegister);
            config.setModel(model);
        }

        store = new EntityStore(env, "test", config);
    }

    private void close()
        throws DatabaseException {

        store.close();
        store = null;
    }

    @Override
    public void setUp()
        throws Exception {

        super.setUp();
    }

    @Override
    public void tearDown()
        throws Exception {

        if (store != null) {
            try {
                store.close();
            } catch (Throwable e) {
                System.out.println("tearDown: " + e);
            }
            store = null;
        }
        super.tearDown();
    }

    public void testBadKeyClass1()
        throws DatabaseException {

        open();
        try {
            store.getPrimaryIndex(BadKeyClass1.class, UseBadKeyClass1.class);
            fail();
        } catch (IllegalArgumentException expected) {
            assertTrue(expected.getMessage().indexOf("@KeyField") >= 0);
        }
        close();
    }

    /** Missing @KeyField in composite key class. */
    @Persistent
    static class BadKeyClass1 {

        private int f1;
    }

    @Entity
    static class UseBadKeyClass1 {

        @PrimaryKey
        private BadKeyClass1 f1 = new BadKeyClass1();

        @SecondaryKey(relate=ONE_TO_ONE)
        private BadKeyClass1 f2 = new BadKeyClass1();
    }

    public void testBadSequenceKeys()
        throws DatabaseException {

        open();
        try {
            store.getPrimaryIndex(Boolean.class, BadSequenceKeyEntity1.class);
            fail();
        } catch (IllegalArgumentException expected) {
            assertTrue(expected.getMessage().indexOf
                ("Type not allowed for sequence") >= 0);
        }
        try {
            store.getPrimaryIndex(BadSequenceKeyEntity2.Key.class,
                     BadSequenceKeyEntity2.class);
            fail();
        } catch (IllegalArgumentException expected) {
            assertTrue(expected.getMessage().indexOf
                ("Type not allowed for sequence") >= 0);
        }
        try {
            store.getPrimaryIndex(BadSequenceKeyEntity3.Key.class,
                     BadSequenceKeyEntity3.class);
            fail();
        } catch (IllegalArgumentException expected) {
            assertTrue(expected.getMessage().indexOf
                ("A composite key class used with a sequence may contain " +
                 "only a single integer key field")>= 0);
        }
        close();
    }

    /** Boolean not allowed for sequence key. */
    @Entity
    static class BadSequenceKeyEntity1 {

        @PrimaryKey(sequence="X")
        private boolean key;
    }

    /** Composite key with non-integer field not allowed for sequence key. */
    @Entity
    static class BadSequenceKeyEntity2 {

        @PrimaryKey(sequence="X")
        private Key key;

        @Persistent
        static class Key {
            @KeyField(1)
            boolean key;
        }
    }

    /** Composite key with multiple key fields not allowed for sequence key. */
    @Entity
    static class BadSequenceKeyEntity3 {

        @PrimaryKey(sequence="X")
        private Key key;

        @Persistent
        static class Key {
            @KeyField(1)
            int key;
            @KeyField(2)
            int key2;
        }
    }

    /**
     * A proxied object may not current contain a field that references the
     * parent proxy.  [#15815]
     */
    public void testProxyNestedRef()
        throws DatabaseException {

        open();
        PrimaryIndex<Integer,ProxyNestedRef> index = store.getPrimaryIndex
            (Integer.class, ProxyNestedRef.class);
        ProxyNestedRef entity = new ProxyNestedRef();
        entity.list.add(entity.list);
        try {
            index.put(entity);
            fail();
        } catch (IllegalArgumentException expected) {
            assertTrue(expected.getMessage().indexOf
                ("Cannot embed a reference to a proxied object") >= 0);
        }
        close();
    }

    @Entity
    static class ProxyNestedRef {

        @PrimaryKey
        private int key;

        ArrayList<Object> list = new ArrayList<Object>();
    }

    /**
     * Disallow primary keys on entity subclasses.  [#15757]
     */
    public void testEntitySubclassWithPrimaryKey()
        throws DatabaseException {

        open();
        PrimaryIndex<Integer,EntitySuperClass> index = store.getPrimaryIndex
            (Integer.class, EntitySuperClass.class);
        EntitySuperClass e1 = new EntitySuperClass(1, "one");
        index.put(e1);
        assertEquals(e1, index.get(1));
        EntitySubClass e2 = new EntitySubClass(2, "two", "foo", 9);
        try {
            index.put(e2);
            fail();
        } catch (IllegalArgumentException e) {
            assertTrue(e.getMessage().contains
                ("PrimaryKey may not appear on an Entity subclass"));
        }
        assertEquals(e1, index.get(1));
        close();
    }

    @Entity
    static class EntitySuperClass {

        @PrimaryKey
        private int x;

        private String y;

        EntitySuperClass(int x, String y) {
            assert y != null;
            this.x = x;
            this.y = y;
        }

        private EntitySuperClass() {}

        @Override
        public String toString() {
            return "x=" + x + " y=" + y;
        }

        @Override
        public boolean equals(Object other) {
            if (other instanceof EntitySuperClass) {
                EntitySuperClass o = (EntitySuperClass) other;
                return x == o.x && y.equals(o.y);
            } else {
                return false;
            }
        }
    }

    @Persistent
    static class EntitySubClass extends EntitySuperClass {

        @PrimaryKey
        private String foo;

        private int z;

        EntitySubClass(int x, String y, String foo, int z) {
            super(x, y);
            assert foo != null;
            this.foo = foo;
            this.z = z;
        }

        private EntitySubClass() {}

        @Override
        public String toString() {
            return super.toString() + " z=" + z;
        }

        @Override
        public boolean equals(Object other) {
            if (other instanceof EntitySubClass) {
                EntitySubClass o = (EntitySubClass) other;
                return super.equals(o) && z == o.z;
            } else {
                return false;
            }
        }
    }

    /**
     * Disallow embedded entity classes and subclasses.  [#16077]
     */
    public void testEmbeddedEntity()
        throws DatabaseException {

        open();
        PrimaryIndex<Integer,EmbeddingEntity> index = store.getPrimaryIndex
            (Integer.class, EmbeddingEntity.class);
        EmbeddingEntity e1 = new EmbeddingEntity(1, null);
        index.put(e1);
        assertEquals(e1, index.get(1));

        EmbeddingEntity e2 =
            new EmbeddingEntity(2, new EntitySuperClass(2, "two"));
        try {
            index.put(e2);
            fail();
        } catch (IllegalArgumentException e) {
            assertTrue(e.getMessage().contains
                ("References to entities are not allowed"));
        }

        EmbeddingEntity e3 = new EmbeddingEntity
            (3, new EmbeddedEntitySubClass(3, "three", "foo", 9));
        try {
            index.put(e3);
            fail();
        } catch (IllegalArgumentException e) {
            assertTrue(e.toString(), e.getMessage().contains
                ("References to entities are not allowed"));
        }

        assertEquals(e1, index.get(1));
        close();
    }

    @Entity
    static class EmbeddingEntity {

        @PrimaryKey
        private int x;

        private EntitySuperClass y;

        EmbeddingEntity(int x, EntitySuperClass y) {
            this.x = x;
            this.y = y;
        }

        private EmbeddingEntity() {}

        @Override
        public String toString() {
            return "x=" + x + " y=" + y;
        }

        @Override
        public boolean equals(Object other) {
            if (other instanceof EmbeddingEntity) {
                EmbeddingEntity o = (EmbeddingEntity) other;
                return x == o.x &&
                       ((y == null) ? (o.y == null) : y.equals(o.y));
            } else {
                return false;
            }
        }
    }

    @Persistent
    static class EmbeddedEntitySubClass extends EntitySuperClass {

        private String foo;

        private int z;

        EmbeddedEntitySubClass(int x, String y, String foo, int z) {
            super(x, y);
            assert foo != null;
            this.foo = foo;
            this.z = z;
        }

        private EmbeddedEntitySubClass() {}

        @Override
        public String toString() {
            return super.toString() + " z=" + z;
        }

        @Override
        public boolean equals(Object other) {
            if (other instanceof EmbeddedEntitySubClass) {
                EmbeddedEntitySubClass o = (EmbeddedEntitySubClass) other;
                return super.equals(o) && z == o.z;
            } else {
                return false;
            }
        }
    }

    /**
     * Disallow SecondaryKey collection with no type parameter. [#15950]
     */
    public void testTypelessKeyCollection()
        throws DatabaseException {

        open();
        try {
            store.getPrimaryIndex
                (Integer.class, TypelessKeyCollectionEntity.class);
            fail();
        } catch (IllegalArgumentException e) {
            assertTrue(e.toString(), e.getMessage().contains
                ("Collection typed secondary key field must have a " +
                 "single generic type argument and a wildcard or type " +
                 "bound is not allowed"));
        }
        close();
    }

    @Entity
    static class TypelessKeyCollectionEntity {

        @PrimaryKey
        private int x;

        @SecondaryKey(relate=ONE_TO_MANY)
        private Collection keys = new ArrayList();

        TypelessKeyCollectionEntity(int x) {
            this.x = x;
        }

        private TypelessKeyCollectionEntity() {}
    }

    /**
     * Disallow a persistent proxy that extends an entity.  [#15950]
     */
    public void testProxyEntity()
        throws DatabaseException {

        try {
            open(ProxyExtendsEntity.class);
            fail();
        } catch (IllegalArgumentException e) {
            assertTrue(e.toString(), e.getMessage().contains
                ("A proxy may not be an entity"));
        }
    }

    @Persistent(proxyFor=BigDecimal.class)
    static class ProxyExtendsEntity
        extends EntitySuperClass
        implements PersistentProxy<BigDecimal> {

        private String rep;

        public BigDecimal convertProxy() {
            return new BigDecimal(rep);
        }

        public void initializeProxy(BigDecimal o) {
            rep = o.toString();
        }
    }

    /**
     * Wrapper type not allowed for nullified foreign key.
     */
    public void testBadNullifyKey()
        throws DatabaseException {

        open();
        try {
            store.getPrimaryIndex(Integer.class, BadNullifyKeyEntity1.class);
            fail();
        } catch (IllegalArgumentException expected) {
            assertTrue(expected.getMessage().indexOf
                ("NULLIFY may not be used with primitive fields") >= 0);
        }
        close();
    }

    @Entity
    static class BadNullifyKeyEntity1 {

        @PrimaryKey
        private int key;

        @SecondaryKey(relate=ONE_TO_ONE,
                      relatedEntity=BadNullifyKeyEntity2.class,
                      onRelatedEntityDelete=NULLIFY)
        private int secKey; // Should be Integer, not int.
    }

    @Entity
    static class BadNullifyKeyEntity2 {

        @PrimaryKey
        private int key;
    }

    /**
     * @Persistent not allowed on an enum.
     */
    public void testPersistentEnum()
        throws DatabaseException {

        open();
        try {
            store.getPrimaryIndex(Integer.class, PersistentEnumEntity.class);
            fail();
        } catch (IllegalArgumentException expected) {
            assertTrue(expected.getMessage().indexOf
                ("not allowed for enum, interface, or primitive") >= 0);
        }
        close();
    }

    @Entity
    static class PersistentEnumEntity {

        @PrimaryKey
        private int key;

        @Persistent
        enum MyEnum {X, Y, Z};

        MyEnum f1;
    }

    /**
     * Disallow a reference to an interface marked @Persistent.
     */
    public void testPersistentInterface()
        throws DatabaseException {

        open();
        try {
            store.getPrimaryIndex(Integer.class,
                                  PersistentInterfaceEntity1.class);
            fail();
        } catch (IllegalArgumentException expected) {
            assertTrue(expected.getMessage().indexOf
                ("not allowed for enum, interface, or primitive") >= 0);
        }
        close();
    }

    @Entity
    static class PersistentInterfaceEntity1 {

        @PrimaryKey
        private int key;

        @SecondaryKey(relate=ONE_TO_ONE,
                      relatedEntity=PersistentInterfaceEntity2.class)
        private int secKey; // Should be Integer, not int.
    }

    @Persistent
    interface PersistentInterfaceEntity2 {
    }

    /**
     * Disallow reference to @Persistent inner class.
     */
    public void testPersistentInnerClass()
        throws DatabaseException {

        open();
        try {
            store.getPrimaryIndex(Integer.class,
                                  PersistentInnerClassEntity1.class);
            fail();
        } catch (IllegalArgumentException expected) {
            assertTrue(expected.getMessage().indexOf
                ("Inner classes not allowed") >= 0);
        }
        close();
    }

    @Entity
    static class PersistentInnerClassEntity1 {

        @PrimaryKey
        private int key;

        private PersistentInnerClass f;
    }

    /* An inner (non-static) class is illegal. */
    @Persistent
    class PersistentInnerClass {

        private int x;
    }

    /**
     * Disallow @Entity inner class.
     */
    public void testEntityInnerClass()
        throws DatabaseException {

        open();
        try {
            store.getPrimaryIndex(Integer.class,
                                  EntityInnerClassEntity.class);
            fail();
        } catch (IllegalArgumentException expected) {
            assertTrue(expected.getMessage().indexOf
                ("Inner classes not allowed") >= 0);
        }
        close();
    }

    /* An inner (non-static) class is illegal. */
    @Entity
    class EntityInnerClassEntity {

        @PrimaryKey
        private int key;
    }
}

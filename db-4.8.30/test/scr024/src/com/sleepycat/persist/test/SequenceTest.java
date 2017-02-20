/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.test;

import java.io.File;

import com.sleepycat.db.Environment;
import com.sleepycat.db.EnvironmentConfig;
import com.sleepycat.db.util.DualTestCase;
import com.sleepycat.persist.EntityStore;
import com.sleepycat.persist.PrimaryIndex;
import com.sleepycat.persist.StoreConfig;
import com.sleepycat.persist.model.Entity;
import com.sleepycat.persist.model.KeyField;
import com.sleepycat.persist.model.Persistent;
import com.sleepycat.persist.model.PrimaryKey;
import com.sleepycat.util.test.SharedTestUtils;
import com.sleepycat.util.test.TestEnv;

/**
 * @author Mark Hayes
 */
public class SequenceTest extends DualTestCase {

    private File envHome;
    private Environment env;

    @Override
    public void setUp()
        throws Exception {

        super.setUp();

        envHome = new File(System.getProperty(SharedTestUtils.DEST_DIR));
        SharedTestUtils.emptyDir(envHome);
    }

    @Override
    public void tearDown()
        throws Exception {

        super.tearDown();

        envHome = null;
        env = null;
    }

    public void testSequenceKeys()
        throws Exception {

        Class[] classes = {
            SequenceEntity_Long.class,
            SequenceEntity_Integer.class,
            SequenceEntity_Short.class,
            SequenceEntity_Byte.class,
            SequenceEntity_tlong.class,
            SequenceEntity_tint.class,
            SequenceEntity_tshort.class,
            SequenceEntity_tbyte.class,
            SequenceEntity_Long_composite.class,
            SequenceEntity_Integer_composite.class,
            SequenceEntity_Short_composite.class,
            SequenceEntity_Byte_composite.class,
            SequenceEntity_tlong_composite.class,
            SequenceEntity_tint_composite.class,
            SequenceEntity_tshort_composite.class,
            SequenceEntity_tbyte_composite.class,
        };

        EnvironmentConfig envConfig = TestEnv.TXN.getConfig();
        envConfig.setAllowCreate(true);
        env = create(envHome, envConfig);

        StoreConfig storeConfig = new StoreConfig();
        storeConfig.setAllowCreate(true);
        storeConfig.setTransactional(true);
        EntityStore store = new EntityStore(env, "foo", storeConfig);

        long seq = 0;

        for (int i = 0; i < classes.length; i += 1) {
            Class entityCls = classes[i];
            SequenceEntity entity = (SequenceEntity) entityCls.newInstance();
            Class keyCls = entity.getKeyClass();

            PrimaryIndex<Object,SequenceEntity> index =
                store.getPrimaryIndex(keyCls, entityCls);
            index.putNoReturn(entity);
            seq += 1;
            assertEquals(seq, entity.getKey());

            index.putNoReturn(entity);
            assertEquals(seq, entity.getKey());

            entity.nullifyKey();
            index.putNoReturn(entity);
            seq += 1;
            assertEquals(seq, entity.getKey());
        }

        store.close();
        close(env);
        env = null;
    }

    interface SequenceEntity {
        Class getKeyClass();
        long getKey();
        void nullifyKey();
    }

    @Entity
    static class SequenceEntity_Long implements SequenceEntity {

        @PrimaryKey(sequence="X")
        Long priKey;

        public Class getKeyClass() {
            return Long.class;
        }

        public long getKey() {
            return priKey;
        }

        public void nullifyKey() {
            priKey = null;
        }
    }

    @Entity
    static class SequenceEntity_Integer implements SequenceEntity {

        @PrimaryKey(sequence="X")
        Integer priKey;

        public Class getKeyClass() {
            return Integer.class;
        }

        public long getKey() {
            return priKey;
        }

        public void nullifyKey() {
            priKey = null;
        }
    }

    @Entity
    static class SequenceEntity_Short implements SequenceEntity {

        @PrimaryKey(sequence="X")
        Short priKey;

        public Class getKeyClass() {
            return Short.class;
        }

        public long getKey() {
            return priKey;
        }

        public void nullifyKey() {
            priKey = null;
        }
    }

    @Entity
    static class SequenceEntity_Byte implements SequenceEntity {

        @PrimaryKey(sequence="X")
        Byte priKey;

        public Class getKeyClass() {
            return Byte.class;
        }

        public long getKey() {
            return priKey;
        }

        public void nullifyKey() {
            priKey = null;
        }
    }

    @Entity
    static class SequenceEntity_tlong implements SequenceEntity {

        @PrimaryKey(sequence="X")
        long priKey;

        public Class getKeyClass() {
            return Long.class;
        }

        public long getKey() {
            return priKey;
        }

        public void nullifyKey() {
            priKey = 0;
        }
    }

    @Entity
    static class SequenceEntity_tint implements SequenceEntity {

        @PrimaryKey(sequence="X")
        int priKey;

        public Class getKeyClass() {
            return Integer.class;
        }

        public long getKey() {
            return priKey;
        }

        public void nullifyKey() {
            priKey = 0;
        }
    }

    @Entity
    static class SequenceEntity_tshort implements SequenceEntity {

        @PrimaryKey(sequence="X")
        short priKey;

        public Class getKeyClass() {
            return Short.class;
        }

        public long getKey() {
            return priKey;
        }

        public void nullifyKey() {
            priKey = 0;
        }
    }

    @Entity
    static class SequenceEntity_tbyte implements SequenceEntity {

        @PrimaryKey(sequence="X")
        byte priKey;

        public Class getKeyClass() {
            return Byte.class;
        }

        public long getKey() {
            return priKey;
        }

        public void nullifyKey() {
            priKey = 0;
        }
    }

    @Entity
    static class SequenceEntity_Long_composite implements SequenceEntity {

        @PrimaryKey(sequence="X")
        Key priKey;

        @Persistent
        static class Key {
            @KeyField(1)
            Long priKey;
        }

        public Class getKeyClass() {
            return Key.class;
        }

        public long getKey() {
            return priKey.priKey;
        }

        public void nullifyKey() {
            priKey = null;
        }
    }

    @Entity
    static class SequenceEntity_Integer_composite implements SequenceEntity {

        @PrimaryKey(sequence="X")
        Key priKey;

        @Persistent
        static class Key {
            @KeyField(1)
            Integer priKey;
        }

        public Class getKeyClass() {
            return Key.class;
        }

        public long getKey() {
            return priKey.priKey;
        }

        public void nullifyKey() {
            priKey = null;
        }
    }

    @Entity
    static class SequenceEntity_Short_composite implements SequenceEntity {

        @PrimaryKey(sequence="X")
        Key priKey;

        @Persistent
        static class Key {
            @KeyField(1)
            Short priKey;
        }

        public Class getKeyClass() {
            return Key.class;
        }

        public long getKey() {
            return priKey.priKey;
        }

        public void nullifyKey() {
            priKey = null;
        }
    }

    @Entity
    static class SequenceEntity_Byte_composite implements SequenceEntity {

        @PrimaryKey(sequence="X")
        Key priKey;

        @Persistent
        static class Key {
            @KeyField(1)
            Byte priKey;
        }

        public Class getKeyClass() {
            return Key.class;
        }

        public long getKey() {
            return priKey.priKey;
        }

        public void nullifyKey() {
            priKey = null;
        }
    }

    @Entity
    static class SequenceEntity_tlong_composite implements SequenceEntity {

        @PrimaryKey(sequence="X")
        Key priKey;

        @Persistent
        static class Key {
            @KeyField(1)
            long priKey;
        }

        public Class getKeyClass() {
            return Key.class;
        }

        public long getKey() {
            return priKey.priKey;
        }

        public void nullifyKey() {
            priKey = null;
        }
    }

    @Entity
    static class SequenceEntity_tint_composite implements SequenceEntity {

        @PrimaryKey(sequence="X")
        Key priKey;

        @Persistent
        static class Key {
            @KeyField(1)
            int priKey;
        }

        public Class getKeyClass() {
            return Key.class;
        }

        public long getKey() {
            return priKey.priKey;
        }

        public void nullifyKey() {
            priKey = null;
        }
    }

    @Entity
    static class SequenceEntity_tshort_composite implements SequenceEntity {

        @PrimaryKey(sequence="X")
        Key priKey;

        @Persistent
        static class Key {
            @KeyField(1)
            short priKey;
        }

        public Class getKeyClass() {
            return Key.class;
        }

        public long getKey() {
            return priKey.priKey;
        }

        public void nullifyKey() {
            priKey = null;
        }
    }

    @Entity
    static class SequenceEntity_tbyte_composite implements SequenceEntity {

        @PrimaryKey(sequence="X")
        Key priKey;

        @Persistent
        static class Key {
            @KeyField(1)
            byte priKey;
        }

        public Class getKeyClass() {
            return Key.class;
        }

        public long getKey() {
            return priKey.priKey;
        }

        public void nullifyKey() {
            priKey = null;
        }
    }
}

/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.test;

import static com.sleepycat.persist.model.Relationship.MANY_TO_ONE;

import java.io.File;

import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.Environment;
import com.sleepycat.db.EnvironmentConfig;
import com.sleepycat.db.Transaction;
import com.sleepycat.db.util.DualTestCase;
import com.sleepycat.persist.EntityCursor;
import com.sleepycat.persist.EntityStore;
import com.sleepycat.persist.PrimaryIndex;
import com.sleepycat.persist.SecondaryIndex;
import com.sleepycat.persist.StoreConfig;
import com.sleepycat.persist.model.AnnotationModel;
import com.sleepycat.persist.model.Entity;
import com.sleepycat.persist.model.EntityModel;
import com.sleepycat.persist.model.Persistent;
import com.sleepycat.persist.model.PrimaryKey;
import com.sleepycat.persist.model.SecondaryKey;
import com.sleepycat.util.test.SharedTestUtils;
import com.sleepycat.util.test.TestEnv;

public class SubclassIndexTest extends DualTestCase {

    private File envHome;
    private Environment env;
    private EntityStore store;

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

    private void open()
        throws DatabaseException {

        EnvironmentConfig envConfig = TestEnv.TXN.getConfig();
        envConfig.setAllowCreate(true);
        env = create(envHome, envConfig);

        EntityModel model = new AnnotationModel();
        model.registerClass(Manager.class);
        model.registerClass(SalariedManager.class);

        StoreConfig storeConfig = new StoreConfig();
        storeConfig.setModel(model);
        storeConfig.setAllowCreate(true);
        storeConfig.setTransactional(true);
        store = new EntityStore(env, "foo", storeConfig);
    }

    private void close()
        throws DatabaseException {

        store.close();
        store = null;
        close(env);
        env = null;
    }

    public void testSubclassIndex()
        throws DatabaseException {

        open();

        PrimaryIndex<String, Employee> employeesById =
            store.getPrimaryIndex(String.class, Employee.class);

        employeesById.put(new Employee("1"));
        employeesById.put(new Manager("2", "a"));
        employeesById.put(new Manager("3", "a"));
        employeesById.put(new Manager("4", "b"));

        Employee e;
        Manager m;

        e = employeesById.get("1");
        assertNotNull(e);
        assertTrue(!(e instanceof Manager));

        /* Ensure DB exists BEFORE calling getSubclassIndex. [#15247] */
        PersistTestUtils.assertDbExists
            (true, env, "foo", Employee.class.getName(), "dept");

        /* Normal use: Subclass index for a key in the subclass. */
        SecondaryIndex<String, String, Manager> managersByDept =
            store.getSubclassIndex
                (employeesById, Manager.class, String.class, "dept");

        m = managersByDept.get("a");
        assertNotNull(m);
        assertEquals("2", m.id);

        m = managersByDept.get("b");
        assertNotNull(m);
        assertEquals("4", m.id);

        Transaction txn = env.beginTransaction(null, null);
        EntityCursor<Manager> managers = managersByDept.entities(txn, null);
        try {
            m = managers.next();
            assertNotNull(m);
            assertEquals("2", m.id);
            m = managers.next();
            assertNotNull(m);
            assertEquals("3", m.id);
            m = managers.next();
            assertNotNull(m);
            assertEquals("4", m.id);
            m = managers.next();
            assertNull(m);
        } finally {
            managers.close();
            txn.commit();
        }

        /* Getting a subclass index for the entity class is also allowed. */
        store.getSubclassIndex
            (employeesById, Employee.class, String.class, "other");

        /* Getting a subclass index for a base class key is not allowed. */
        try {
            store.getSubclassIndex
                (employeesById, Manager.class, String.class, "other");
            fail();
        } catch (IllegalArgumentException expected) {
        }

        close();
    }

    /**
     * Previously this tested that a secondary key database was added only
     * AFTER storing the first instance of the subclass that defines the key.
     * Now that we require registering the subclass up front, the database is
     * created up front also.  So this test is somewhat less useful, but still
     * nice to have around.  [#16399]
     */
    public void testAddSecKey()
        throws DatabaseException {

        open();
        PrimaryIndex<String, Employee> employeesById =
            store.getPrimaryIndex(String.class, Employee.class);
        employeesById.put(new Employee("1"));
        assertTrue(hasEntityKey("dept"));
        close();

        open();
        employeesById = store.getPrimaryIndex(String.class, Employee.class);
        assertTrue(hasEntityKey("dept"));
        employeesById.put(new Manager("2", "a"));
        assertTrue(hasEntityKey("dept"));
        close();

        open();
        assertTrue(hasEntityKey("dept"));
        close();
        
        open();
        employeesById = store.getPrimaryIndex(String.class, Employee.class);
        assertTrue(hasEntityKey("salary"));
        employeesById.put(new SalariedManager("3", "a", "111"));
        assertTrue(hasEntityKey("salary"));
        close();

        open();
        assertTrue(hasEntityKey("dept"));
        assertTrue(hasEntityKey("salary"));
        close();
    }

    private boolean hasEntityKey(String keyName) {
        return store.getModel().
               getRawType(Employee.class.getName()).
               getEntityMetadata().
               getSecondaryKeys().
               keySet().
               contains(keyName);
    }

    @Entity
    private static class Employee {

        @PrimaryKey
        String id;

        @SecondaryKey(relate=MANY_TO_ONE)
        String other;

        Employee(String id) {
            this.id = id;
        }

        private Employee() {}
    }

    @Persistent
    private static class Manager extends Employee {

        @SecondaryKey(relate=MANY_TO_ONE)
        String dept;

        Manager(String id, String dept) {
            super(id);
            this.dept = dept;
        }

        private Manager() {}
    }

    @Persistent
    private static class SalariedManager extends Manager {

        @SecondaryKey(relate=MANY_TO_ONE)
        String salary;

        SalariedManager(String id, String dept, String salary) {
            super(id, dept);
            this.salary = salary;
        }

        private SalariedManager() {}
    }
}

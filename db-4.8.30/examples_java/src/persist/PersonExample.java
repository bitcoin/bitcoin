/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package persist;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.HashSet;
import java.util.Set;

import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.Environment;
import com.sleepycat.db.EnvironmentConfig;
import com.sleepycat.persist.EntityCursor;
import com.sleepycat.persist.EntityIndex;
import com.sleepycat.persist.EntityStore;
import com.sleepycat.persist.PrimaryIndex;
import com.sleepycat.persist.SecondaryIndex;
import com.sleepycat.persist.StoreConfig;
import com.sleepycat.persist.model.Entity;
import com.sleepycat.persist.model.Persistent;
import com.sleepycat.persist.model.PrimaryKey;
import com.sleepycat.persist.model.SecondaryKey;
import static com.sleepycat.persist.model.DeleteAction.NULLIFY;
import static com.sleepycat.persist.model.Relationship.ONE_TO_ONE;
import static com.sleepycat.persist.model.Relationship.ONE_TO_MANY;
import static com.sleepycat.persist.model.Relationship.MANY_TO_ONE;
import static com.sleepycat.persist.model.Relationship.MANY_TO_MANY;

public class PersonExample {

    /* An entity class. */
    @Entity
    static class Person {

        @PrimaryKey
        String ssn;

        String name;
        Address address;

        @SecondaryKey(relate=MANY_TO_ONE, relatedEntity=Person.class)
        String parentSsn;

        @SecondaryKey(relate=ONE_TO_MANY)
        Set<String> emailAddresses = new HashSet<String>();

        @SecondaryKey(relate=MANY_TO_MANY,
                      relatedEntity=Employer.class,
                      onRelatedEntityDelete=NULLIFY)
        Set<Long> employerIds = new HashSet<Long>();

        Person(String name, String ssn, String parentSsn) {
            this.name = name;
            this.ssn = ssn;
            this.parentSsn = parentSsn;
        }

        private Person() {} // For deserialization
    }

    /* Another entity class. */
    @Entity
    static class Employer {

        @PrimaryKey(sequence="ID")
        long id;

        @SecondaryKey(relate=ONE_TO_ONE)
        String name;

        Address address;

        Employer(String name) {
            this.name = name;
        }

        private Employer() {} // For deserialization
    }

    /* A persistent class used in other classes. */
    @Persistent
    static class Address {
        String street;
        String city;
        String state;
        int zipCode;
        private Address() {} // For deserialization
    }

    /* The data accessor class for the entity model. */
    static class PersonAccessor {

        /* Person accessors */
        PrimaryIndex<String,Person> personBySsn;
        SecondaryIndex<String,String,Person> personByParentSsn;
        SecondaryIndex<String,String,Person> personByEmailAddresses;
        SecondaryIndex<Long,String,Person> personByEmployerIds;

        /* Employer accessors */
        PrimaryIndex<Long,Employer> employerById;
        SecondaryIndex<String,Long,Employer> employerByName;

        /* Opens all primary and secondary indices. */
        public PersonAccessor(EntityStore store)
            throws DatabaseException {

            personBySsn = store.getPrimaryIndex(
                String.class, Person.class);

            personByParentSsn = store.getSecondaryIndex(
                personBySsn, String.class, "parentSsn");

            personByEmailAddresses = store.getSecondaryIndex(
                personBySsn, String.class, "emailAddresses");

            personByEmployerIds = store.getSecondaryIndex(
                personBySsn, Long.class, "employerIds");

            employerById = store.getPrimaryIndex(
                Long.class, Employer.class);

            employerByName = store.getSecondaryIndex(
                employerById, String.class, "name");
        }
    }

    public static void main(String[] args)
        throws DatabaseException, FileNotFoundException {

        if (args.length != 2 || !"-h".equals(args[0])) {
            System.err.println
                ("Usage: java " + PersonExample.class.getName() +
                 " -h <envHome>");
            System.exit(2);
        }
        PersonExample example = new PersonExample(new File(args[1]));
        example.run();
        example.close();
    }

    private Environment env;
    private EntityStore store;
    private PersonAccessor dao;

    private PersonExample(File envHome)
        throws DatabaseException, FileNotFoundException {

        /* Open a transactional Berkeley DB engine environment. */
        EnvironmentConfig envConfig = new EnvironmentConfig();
        envConfig.setAllowCreate(true);
        envConfig.setTransactional(true);
        envConfig.setInitializeCache(true);
        envConfig.setInitializeLocking(true);
        env = new Environment(envHome, envConfig);

        /* Open a transactional entity store. */
        StoreConfig storeConfig = new StoreConfig();
        storeConfig.setAllowCreate(true);
        storeConfig.setTransactional(true);
        store = new EntityStore(env, "PersonStore", storeConfig);

        /* Initialize the data access object. */
        dao = new PersonAccessor(store);
    }

    private void run()
        throws DatabaseException {

        /*
         * Add a parent and two children using the Person primary index.
         * Specifying a non-null parentSsn adds the child Person to the
         * sub-index of children for that parent key.
         */
        dao.personBySsn.put
            (new Person("Bob Smith", "111-11-1111", null));
        dao.personBySsn.put
            (new Person("Mary Smith", "333-33-3333", "111-11-1111"));
        dao.personBySsn.put
            (new Person("Jack Smith", "222-22-2222", "111-11-1111"));

        /* Print the children of a parent using a sub-index and a cursor. */
        EntityCursor<Person> children =
            dao.personByParentSsn.subIndex("111-11-1111").entities();
        try {
            for (Person child : children) {
                System.out.println(child.ssn + ' ' + child.name);
            }
        } finally {
            children.close();
        }

        /* Get Bob by primary key using the primary index. */
        Person bob = dao.personBySsn.get("111-11-1111");
        assert bob != null;

        /*
         * Create two employers if they do not already exist.  Their primary
         * keys are assigned from a sequence.
         */
        Employer gizmoInc = dao.employerByName.get("Gizmo Inc");
        if (gizmoInc == null) {
            gizmoInc = new Employer("Gizmo Inc");
            dao.employerById.put(gizmoInc);
        }
        Employer gadgetInc = dao.employerByName.get("Gadget Inc");
        if (gadgetInc == null) {
            gadgetInc = new Employer("Gadget Inc");
            dao.employerById.put(gadgetInc);
        }

        /* Bob has two jobs and two email addresses. */
        bob.employerIds.add(gizmoInc.id);
        bob.employerIds.add(gadgetInc.id);
        bob.emailAddresses.add("bob@bob.com");
        bob.emailAddresses.add("bob@gmail.com");

        /* Update Bob's record. */
        dao.personBySsn.put(bob);

        /* Bob can now be found by both email addresses. */
        bob = dao.personByEmailAddresses.get("bob@bob.com");
        assert bob != null;
        bob = dao.personByEmailAddresses.get("bob@gmail.com");
        assert bob != null;

        /* Bob can also be found as an employee of both employers. */
        EntityIndex<String,Person> employees;
        employees = dao.personByEmployerIds.subIndex(gizmoInc.id);
        assert employees.contains("111-11-1111");
        employees = dao.personByEmployerIds.subIndex(gadgetInc.id);
        assert employees.contains("111-11-1111");

        /*
         * When an employer is deleted, the onRelatedEntityDelete=NULLIFY for
         * the employerIds key causes the deleted ID to be removed from Bob's
         * employerIds.
         */
        dao.employerById.delete(gizmoInc.id);
        bob = dao.personBySsn.get("111-11-1111");
        assert bob != null;
        assert !bob.employerIds.contains(gizmoInc.id);
    }

    private void close()
        throws DatabaseException {

        store.close();
        env.close();
    }
}

/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist;

import java.util.Map;
import java.util.SortedMap;

import com.sleepycat.bind.EntityBinding;
import com.sleepycat.bind.EntryBinding;
import com.sleepycat.collections.StoredSortedMap;
import com.sleepycat.compat.DbCompat;
import com.sleepycat.db.Cursor;
import com.sleepycat.db.Database;
import com.sleepycat.db.DatabaseEntry;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.Environment;
import com.sleepycat.db.LockMode;
import com.sleepycat.db.OperationStatus;
import com.sleepycat.db.Transaction;
import com.sleepycat.persist.impl.PersistEntityBinding;
import com.sleepycat.persist.impl.PersistKeyAssigner;
import com.sleepycat.persist.model.Entity;
import com.sleepycat.persist.model.PrimaryKey;

/**
 * The primary index for an entity class and its primary key.
 *
 * <p>{@code PrimaryIndex} objects are thread-safe.  Multiple threads may
 * safely call the methods of a shared {@code PrimaryIndex} object.</p>
 *
 * <p>{@code PrimaryIndex} implements {@link EntityIndex} to map the primary
 * key type (PK) to the entity type (E).</p>
 *
 * <p>The {@link Entity} annotation may be used to define an entity class and
 * the {@link PrimaryKey} annotation may be used to define a primary key as
 * shown in the following example.</p>
 *
 * <pre class="code">
 * {@literal @Entity}
 * class Employee {
 *
 *     {@literal @PrimaryKey}
 *     long id;
 *
 *     String name;
 *
 *     Employee(long id, String name) {
 *         this.id = id;
 *         this.name = name;
 *     }
 *
 *     private Employee() {} // For bindings
 * }</pre>
 *
 * <p>To obtain the {@code PrimaryIndex} for a given entity class, call {@link
 * EntityStore#getPrimaryIndex EntityStore.getPrimaryIndex}, passing the
 * primary key class and the entity class.  For example:</p>
 *
 * <pre class="code">
 * EntityStore store = new EntityStore(...);
 *
 * {@code PrimaryIndex<Long,Employee>} primaryIndex =
 *     store.getPrimaryIndex(Long.class, Employee.class);</pre>
 * </pre>
 *
 * <p>Note that {@code Long.class} is passed as the primary key class, but the
 * primary key field has the primitive type {@code long}.  When a primitive
 * primary key field is used, the corresponding primitive wrapper class is used
 * to access the primary index.  For more information on key field types, see
 * {@link PrimaryKey}.</p>
 *
 * <p>The {@code PrimaryIndex} provides the primary storage and access methods
 * for the instances of a particular entity class.  Entities are inserted and
 * updated in the {@code PrimaryIndex} by calling a method in the family of
 * {@link #put} methods.  The {@link #put} method will insert the entity if no
 * entity with the same primary key already exists.  If an entity with the same
 * primary key does exist, it will update the entity and return the existing
 * (old) entity.  For example:</p>
 *
 * <pre class="code">
 * Employee oldEntity;
 * oldEntity = primaryIndex.put(new Employee(1, "Jane Smith"));    // Inserts an entity
 * assert oldEntity == null;
 * oldEntity = primaryIndex.put(new Employee(2, "Joan Smith"));    // Inserts an entity
 * assert oldEntity == null;
 * oldEntity = primaryIndex.put(new Employee(2, "Joan M. Smith")); // Updates an entity
 * assert oldEntity != null;</pre>
 *
 * <p>The {@link #putNoReturn} method can be used to avoid the overhead of
 * returning the existing entity, when the existing entity is not important to
 * the application.  The return type of {@link #putNoReturn} is void.  For
 * example:</p>
 *
 * <pre class="code">
 * primaryIndex.putNoReturn(new Employee(1, "Jane Smith"));    // Inserts an entity
 * primaryIndex.putNoReturn(new Employee(2, "Joan Smith"));    // Inserts an entity
 * primaryIndex.putNoReturn(new Employee(2, "Joan M. Smith")); // Updates an entity</pre>
 *
 * <p>The {@link #putNoOverwrite} method can be used to ensure that an existing
 * entity is not overwritten.  {@link #putNoOverwrite} returns true if the
 * entity was inserted, or false if an existing entity exists and no action was
 * taken.  For example:<p>
 *
 * <pre class="code">
 * boolean inserted;
 * inserted = primaryIndex.putNoOverwrite(new Employee(1, "Jane Smith"));    // Inserts an entity
 * assert inserted;
 * inserted = primaryIndex.putNoOverwrite(new Employee(2, "Joan Smith"));    // Inserts an entity
 * assert inserted;
 * inserted = primaryIndex.putNoOverwrite(new Employee(2, "Joan M. Smith")); // <strong>No action was taken!</strong>
 * assert !inserted;</pre>
 *
 * <p>Primary key values must be unique, in other words, each instance of a
 * given entity class must have a distinct primary key value.  Rather than
 * assigning the unique primary key values yourself, a <em>sequence</em> can be
 * used to assign sequential integer values automatically, starting with the
 * value 1 (one).  A sequence is defined using the {@link PrimaryKey#sequence}
 * annotation property.  For example:</p>
 *
 * <pre class="code">
 * {@literal @Entity}
 * class Employee {
 *
 *     {@literal @PrimaryKey(sequence="ID")}
 *     long id;
 *
 *     String name;
 *
 *     Employee(String name) {
 *         this.name = name;
 *     }
 *
 *     private Employee() {} // For bindings
 * }</pre>
 *
 * <p>The name of the sequence used above is "ID".  Any name can be used.  If
 * the same sequence name is used in more than one entity class, the sequence
 * will be shared by those classes, in other words, a single sequence of
 * integers will be used for all instances of those classes.  See {@link
 * PrimaryKey#sequence} for more information.</p>
 *
 * <p>Any method in the family of {@link #put} methods may be used to insert
 * entities where the primary key is assigned from a sequence.  When the {@link
 * #put} method returns, the primary key field of the entity object will be set
 * to the assigned key value.  For example:</p>
 *
 * <pre class="code">
 * Employee employee;
 * employee = new Employee("Jane Smith");
 * primaryIndex.putNoReturn(employee);    // Inserts an entity
 * assert employee.id == 1;
 * employee = new Employee("Joan Smith");
 * primaryIndex.putNoReturn(employee);    // Inserts an entity
 * assert employee.id == 2;</pre>
 *
 * <p>This begs the question:  How do you update an existing entity, without
 * assigning a new primary key?  The answer is that the {@link #put} methods
 * will only assign a new key from the sequence if the primary key field is
 * zero or null (for reference types).  If an entity with a non-zero and
 * non-null key field is passed to a {@link #put} method, any existing entity
 * with that primary key value will be updated.  For example:</p>
 *
 * <pre class="code">
 * Employee employee;
 * employee = new Employee("Jane Smith");
 * primaryIndex.putNoReturn(employee);    // Inserts an entity
 * assert employee.id == 1;
 * employee = new Employee("Joan Smith");
 * primaryIndex.putNoReturn(employee);    // Inserts an entity
 * assert employee.id == 2;
 * employee.name = "Joan M. Smith";
 * primaryIndex.putNoReturn(employee);    // Updates an existing entity
 * assert employee.id == 2;</pre>
 *
 * <p>Since {@code PrimaryIndex} implements the {@link EntityIndex} interface,
 * it shares the common index methods for retrieving and deleting entities,
 * opening cursors and using transactions.  See {@link EntityIndex} for more
 * information on these topics.</p>
 *
 * <p>Note that when using an index, keys and values are stored and retrieved
 * by value not by reference.  In other words, if an entity object is stored
 * and then retrieved, or retrieved twice, each object will be a separate
 * instance.  For example, in the code below the assertion will always
 * fail.</p>
 * <pre class="code">
 * MyKey key = ...;
 * MyEntity entity1 = new MyEntity(key, ...);
 * index.put(entity1);
 * MyEntity entity2 = index.get(key);
 * assert entity1 == entity2; // always fails!
 * </pre>
 *
 * @author Mark Hayes
 */
public class PrimaryIndex<PK,E> extends BasicIndex<PK,E> {

    private Class<E> entityClass;
    private EntityBinding entityBinding;
    private SortedMap<PK,E> map;
    private PersistKeyAssigner keyAssigner;

    /**
     * Creates a primary index without using an <code>EntityStore</code>.
     *
     * <p>This constructor is not normally needed and is provided for
     * applications that wish to use custom bindings along with the Direct
     * Persistence Layer.  Normally, {@link EntityStore#getPrimaryIndex
     * getPrimaryIndex} is used instead.</p>
     *
     * <p>Note that when this constructor is used directly, primary keys cannot
     * be automatically assigned from a sequence.  The key assignment feature
     * requires knowledge of the primary key field, which is only available if
     * an <code>EntityStore</code> is used.  Of course, primary keys may be
     * assigned from a sequence manually before calling the <code>put</code>
     * methods in this class.</p>
     *
     * @param database the primary database.
     *
     * @param keyClass the class of the primary key.
     *
     * @param keyBinding the binding to be used for primary keys.
     *
     * @param entityClass the class of the entities stored in this index.
     *
     * @param entityBinding the binding to be used for entities.
     */
    public PrimaryIndex(Database database,
                        Class<PK> keyClass,
                        EntryBinding<PK> keyBinding,
                        Class<E> entityClass,
                        EntityBinding<E> entityBinding)
        throws DatabaseException {

        super(database, keyClass, keyBinding,
              new EntityValueAdapter(entityClass, entityBinding, false));

        this.entityClass = entityClass;
        this.entityBinding = entityBinding;

        if (entityBinding instanceof PersistEntityBinding) {
            keyAssigner =
                ((PersistEntityBinding) entityBinding).getKeyAssigner();
        }
    }

    /**
     * Returns the underlying database for this index.
     *
     * @return the database.
     */
    public Database getDatabase() {
        return db;
    }

    /**
     * Returns the primary key class for this index.
     *
     * @return the key class.
     */
    public Class<PK> getKeyClass() {
        return keyClass;
    }

    /**
     * Returns the primary key binding for this index.
     *
     * @return the key binding.
     */
    public EntryBinding<PK> getKeyBinding() {
        return keyBinding;
    }

    /**
     * Returns the entity class for this index.
     *
     * @return the entity class.
     */
    public Class<E> getEntityClass() {
        return entityClass;
    }

    /**
     * Returns the entity binding for this index.
     *
     * @return the entity binding.
     */
    public EntityBinding<E> getEntityBinding() {
        return entityBinding;
    }

    /**
     * Inserts an entity and returns null, or updates it if the primary key
     * already exists and returns the existing entity.
     *
     * <p>If a {@link PrimaryKey#sequence} is used and the primary key field of
     * the given entity is null or zero, this method will assign the next value
     * from the sequence to the primary key field of the given entity.</p>
     *
     * <p>Auto-commit is used implicitly if the store is transactional.</p>
     *
     * @param entity the entity to be inserted or updated.
     *
     * @return the existing entity that was updated, or null if the entity was
     * inserted.
     */
    public E put(E entity)
        throws DatabaseException {

        return put(null, entity);
    }

    /**
     * Inserts an entity and returns null, or updates it if the primary key
     * already exists and returns the existing entity.
     *
     * <p>If a {@link PrimaryKey#sequence} is used and the primary key field of
     * the given entity is null or zero, this method will assign the next value
     * from the sequence to the primary key field of the given entity.</p>
     *
     * @param txn the transaction used to protect this operation, null to use
     * auto-commit, or null if the store is non-transactional.
     *
     * @param entity the entity to be inserted or updated.
     *
     * @return the existing entity that was updated, or null if the entity was
     * inserted.
     */
    public E put(Transaction txn, E entity)
        throws DatabaseException {

        DatabaseEntry keyEntry = new DatabaseEntry();
        DatabaseEntry dataEntry = new DatabaseEntry();
        assignKey(entity, keyEntry);

        boolean autoCommit = false;
	Environment env = db.getEnvironment();
        if (transactional &&
	    txn == null &&
	    DbCompat.getThreadTransaction(env) == null) {
            txn = env.beginTransaction(null, null);
            autoCommit = true;
        }

        boolean failed = true;
        Cursor cursor = db.openCursor(txn, null);
        LockMode lockMode = locking ? LockMode.RMW : null;
        try {
            while (true) {
                OperationStatus status =
                    cursor.getSearchKey(keyEntry, dataEntry, lockMode);
                if (status == OperationStatus.SUCCESS) {
                    E existing =
                        (E) entityBinding.entryToObject(keyEntry, dataEntry);
                    entityBinding.objectToData(entity, dataEntry);
                    cursor.put(keyEntry, dataEntry);
                    failed = false;
                    return existing;
                } else {
                    entityBinding.objectToData(entity, dataEntry);
                    status = cursor.putNoOverwrite(keyEntry, dataEntry);
                    if (status != OperationStatus.KEYEXIST) {
                        failed = false;
                        return null;
                    }
                }
            }
        } finally {
            cursor.close();
            if (autoCommit) {
                if (failed) {
                    txn.abort();
                } else {
                    txn.commit();
                }
            }
        }
    }

    /**
     * Inserts an entity, or updates it if the primary key already exists (does
     * not return the existing entity).  This method may be used instead of
     * {@link #put(Object)} to save the overhead of returning the existing
     * entity.
     *
     * <p>If a {@link PrimaryKey#sequence} is used and the primary key field of
     * the given entity is null or zero, this method will assign the next value
     * from the sequence to the primary key field of the given entity.</p>
     *
     * <p>Auto-commit is used implicitly if the store is transactional.</p>
     *
     * @param entity the entity to be inserted or updated.
     */
    public void putNoReturn(E entity)
        throws DatabaseException {

        putNoReturn(null, entity);
    }

    /**
     * Inserts an entity, or updates it if the primary key already exists (does
     * not return the existing entity).  This method may be used instead of
     * {@link #put(Transaction,Object)} to save the overhead of returning the
     * existing entity.
     *
     * <p>If a {@link PrimaryKey#sequence} is used and the primary key field of
     * the given entity is null or zero, this method will assign the next value
     * from the sequence to the primary key field of the given entity.</p>
     *
     * @param txn the transaction used to protect this operation, null to use
     * auto-commit, or null if the store is non-transactional.
     *
     * @param entity the entity to be inserted or updated.
     */
    public void putNoReturn(Transaction txn, E entity)
        throws DatabaseException {

        DatabaseEntry keyEntry = new DatabaseEntry();
        DatabaseEntry dataEntry = new DatabaseEntry();
        assignKey(entity, keyEntry);
        entityBinding.objectToData(entity, dataEntry);

        db.put(txn, keyEntry, dataEntry);
    }

    /**
     * Inserts an entity and returns true, or returns false if the primary key
     * already exists.
     *
     * <p>If a {@link PrimaryKey#sequence} is used and the primary key field of
     * the given entity is null or zero, this method will assign the next value
     * from the sequence to the primary key field of the given entity.</p>
     *
     * <p>Auto-commit is used implicitly if the store is transactional.</p>
     *
     * @param entity the entity to be inserted.
     *
     * @return true if the entity was inserted, or false if an entity with the
     * same primary key is already present.
     */
    public boolean putNoOverwrite(E entity)
        throws DatabaseException {

        return putNoOverwrite(null, entity);
    }

    /**
     * Inserts an entity and returns true, or returns false if the primary key
     * already exists.
     *
     * <p>If a {@link PrimaryKey#sequence} is used and the primary key field of
     * the given entity is null or zero, this method will assign the next value
     * from the sequence to the primary key field of the given entity.</p>
     *
     * @param txn the transaction used to protect this operation, null to use
     * auto-commit, or null if the store is non-transactional.
     *
     * @param entity the entity to be inserted.
     *
     * @return true if the entity was inserted, or false if an entity with the
     * same primary key is already present.
     */
    public boolean putNoOverwrite(Transaction txn, E entity)
        throws DatabaseException {

        DatabaseEntry keyEntry = new DatabaseEntry();
        DatabaseEntry dataEntry = new DatabaseEntry();
        assignKey(entity, keyEntry);
        entityBinding.objectToData(entity, dataEntry);

        OperationStatus status = db.putNoOverwrite(txn, keyEntry, dataEntry);

        return (status == OperationStatus.SUCCESS);
    }

    /**
     * If we are assigning primary keys from a sequence, assign the next key
     * and set the primary key field.
     */
    private void assignKey(E entity, DatabaseEntry keyEntry)
        throws DatabaseException {

        if (keyAssigner != null) {
            if (!keyAssigner.assignPrimaryKey(entity, keyEntry)) {
                entityBinding.objectToKey(entity, keyEntry);
            }
        } else {
            entityBinding.objectToKey(entity, keyEntry);
        }
    }

    /*
     * Of the EntityIndex methods only get()/map()/sortedMap() are implemented
     * here.  All other methods are implemented by BasicIndex.
     */

    public E get(PK key)
        throws DatabaseException {

        return get(null, key, null);
    }

    public E get(Transaction txn, PK key, LockMode lockMode)
        throws DatabaseException {

        DatabaseEntry keyEntry = new DatabaseEntry();
        DatabaseEntry dataEntry = new DatabaseEntry();
        keyBinding.objectToEntry(key, keyEntry);

        OperationStatus status = db.get(txn, keyEntry, dataEntry, lockMode);

        if (status == OperationStatus.SUCCESS) {
            return (E) entityBinding.entryToObject(keyEntry, dataEntry);
        } else {
            return null;
        }
    }

    public Map<PK,E> map() {
        return sortedMap();
    }

    public synchronized SortedMap<PK,E> sortedMap() {
        if (map == null) {
            map = new StoredSortedMap(db, keyBinding, entityBinding, true);
        }
        return map;
    }

    boolean isUpdateAllowed() {
        return true;
    }
}

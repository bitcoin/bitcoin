/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.model;

import static java.lang.annotation.ElementType.FIELD;
import static java.lang.annotation.RetentionPolicy.RUNTIME;

import java.lang.annotation.Documented;
import java.lang.annotation.Retention;
import java.lang.annotation.Target;

import com.sleepycat.db.DatabaseException;
import com.sleepycat.persist.PrimaryIndex;
import com.sleepycat.persist.SecondaryIndex; // for javadoc
import com.sleepycat.persist.StoreConfig;

/**
 * Indicates a secondary key field of an entity class.  The value of the
 * secondary key field is a unique or non-unique identifier for the entity and
 * is accessed via a {@link SecondaryIndex}.
 *
 * <p>{@code SecondaryKey} may appear on any number of fields in an entity
 * class, subclasses and superclasses.  For a secondary key field in the entity
 * class or one of its superclasses, all entity instances will be indexed by
 * that field (if it is non-null).  For a secondary key field in an entity
 * subclass, only instances of that subclass will be indexed by that field (if
 * it is non-null).</p>
 *
 * <p>If a secondary key field is null, the entity will not be indexed by that
 * key.  In other words, the entity cannot be queried by that secondary key nor
 * can the entity be found by iterating through the secondary index.</p>
 *
 * <p>For a given entity class and its superclasses and subclasses, no two
 * secondary keys may have the same name.  By default, the field name
 * identifies the secondary key and the secondary index for a given entity
 * class.  {@link #name} may be specified to override this default.</p>
 *
 * <p>Using {@link #relate}, instances of the entity class are related to
 * secondary keys in a many-to-one, one-to-many, many-to-many, or one-to-one
 * relationship.  This required property specifies the <em>cardinality</em> of
 * each side of the relationship.</p>
 *
 * <p>A secondary key may optionally be used to form a relationship with
 * instances of another entity class using {@link #relatedEntity} and {@link
 * #onRelatedEntityDelete}.  This establishes <em>foreign key constraints</em>
 * for the secondary key.</p>
 *
 * <p>The secondary key field type must be an array or collection type when a
 * <em>x-to-many</em> relationship is used or a singular type when an
 * <em>x-to-one</em> relationship is used; see {@link #relate}.</p>
 *
 * <p>The field type (or element type, when an array or collection type is
 * used) of a secondary key field must follow the same rules as for a {@link
 * <a href="PrimaryKey.html#keyTypes">primary key type</a>}.  The {@link <a
 * href="PrimaryKey.html#sortOrder">key sort order</a>} is also the same.</p>
 *
 * <p>For a secondary key field with a collection type, a type parameter must
 * be used to specify the element type.  For example {@code Collection<String>}
 * is allowed but {@code Collection} is not.</p>
 *
 * @author Mark Hayes
 */
@Documented @Retention(RUNTIME) @Target(FIELD)
public @interface SecondaryKey {

    /**
     * Defines the relationship between instances of the entity class and the
     * secondary keys.
     *
     * <p>The table below summarizes how to create all four variations of
     * relationships.</p>
     * <div>
     * <table border="yes">
     *     <tr><th>Relationship</th>
     *         <th>Field type</th>
     *         <th>Key type</th>
     *         <th>Example</th>
     *     </tr>
     *     <tr><td>{@link Relationship#ONE_TO_ONE}</td>
     *         <td>Singular</td>
     *         <td>Unique</td>
     *         <td>A person record with a unique social security number
     *             key.</td>
     *     </tr>
     *     <tr><td>{@link Relationship#MANY_TO_ONE}</td>
     *         <td>Singular</td>
     *         <td>Duplicates</td>
     *         <td>A person record with a non-unique employer key.</td>
     *     </tr>
     *     <tr><td>{@link Relationship#ONE_TO_MANY}</td>
     *         <td>Array/Collection</td>
     *         <td>Unique</td>
     *         <td>A person record with multiple unique email address keys.</td>
     *     </tr>
     *     <tr><td>{@link Relationship#MANY_TO_MANY}</td>
     *         <td>Array/Collection</td>
     *         <td>Duplicates</td>
     *         <td>A person record with multiple non-unique organization
     *             keys.</td>
     *     </tr>
     * </table>
     * </div>
     *
     * <p>For a <em>many-to-x</em> relationship, the secondary index will
     * have non-unique keys; in other words, duplicates will be allowed.
     * Conversely, for <em>one-to-x</em> relationship, the secondary index
     * will have unique keys.</p>
     *
     * <p>For a <em>x-to-one</em> relationship, the secondary key field is
     * singular; in other words, it may not be an array or collection type.
     * Conversely, for a <em>x-to-many</em> relationship, the secondary key
     * field must be an array or collection type.  A collection type is any
     * implementation of {@link java.util.Collection}.</p>
     */
    Relationship relate();

    /**
     * Specifies the entity to which this entity is related, for establishing
     * foreign key constraints.  Values of this secondary key will be
     * constrained to the set of primary key values for the given entity class.
     *
     * <p>The given class must be an entity class.  This class is called the
     * <em>related entity</em> or <em>foreign entity</em>.</p>
     *
     * <p>When a related entity class is specified, a check (foreign key
     * constraint) is made every time a new secondary key value is stored for
     * this entity, and every time a related entity is deleted.</p>
     *
     * <p>Whenever a new secondary key value is stored for this entity, it is
     * checked to ensure it exists as a primary key value of the related
     * entity.  If it does not, a {@link DatabaseException} will be thrown
     * by the {@link PrimaryIndex} {@code put} method.</p>
     *
     * <p>Whenever a related entity is deleted and its primary key value exists
     * as a secondary key value for this entity, the action is taken that is
     * specified using the {@link #onRelatedEntityDelete} property.</p>
     *
     * <p>Together, these two checks guarantee that a secondary key value for
     * this entity will always exist as a primary key value for the related
     * entity.  Note, however, that a transactional store must be configured
     * to guarantee this to be true in the face of a crash; see {@link
     * StoreConfig#setTransactional}.</p>
     */
    Class relatedEntity() default void.class;

    /**
     * Specifies the action to take when a related entity is deleted having a
     * primary key value that exists as a secondary key value for this entity.
     *
     * <p><em>Note:</em> This property only applies when {@link #relatedEntity}
     * is specified to define the related entity.</p>
     *
     * <p>The default action, {@link DeleteAction#ABORT ABORT}, means that a
     * {@link DatabaseException} is thrown in order to abort the current
     * transaction.</p>
     *
     * <p>If {@link DeleteAction#CASCADE CASCADE} is specified, then this
     * entity will be deleted also.  This in turn could trigger further
     * deletions, causing a cascading effect.</p>
     *
     * <p>If {@link DeleteAction#NULLIFY NULLIFY} is specified, then the
     * secondary key in this entity is set to null and this entity is updated.
     * If the key field type is singular, the field value is set to null;
     * therefore, to specify {@code NULLIFY} for a singular key field type, a
     * primitive wrapper type must be used instead of a primitive type.  If the
     * key field type is an array or collection type, the key is deleted from
     * the array (the array is resized) or from the collection (using {@link
     * java.util.Collection#remove Collection.remove}).</p>
     */
    DeleteAction onRelatedEntityDelete() default DeleteAction.ABORT;

    /**
     * Specifies the name of the key in order to use a name that is different
     * than the field name.
     *
     * <p>This is convenient when prefixes or suffices are used on field names.
     * For example:</p>
     * <pre class="code">
     *  class Person {
     *      {@literal @SecondaryKey(relate=MANY_TO_ONE, relatedEntity=Person.class, name="parentSsn")}
     *      String m_parentSsn;
     *  }</pre>
     *
     * <p>It can also be used to uniquely name a key when multiple secondary
     * keys for a single entity class have the same field name.  For example,
     * an entity class and its subclass may both have a field named 'date',
     * and both fields are used as secondary keys.  The {@code name} property
     * can be specified for one or both fields to give each key a unique
     * name.</p>
     */
    String name() default "";
}

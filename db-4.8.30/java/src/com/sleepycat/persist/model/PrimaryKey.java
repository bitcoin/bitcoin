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

import com.sleepycat.persist.EntityStore;
import com.sleepycat.persist.PrimaryIndex;

/**
 * Indicates the primary key field of an entity class.  The value of the
 * primary key field is the unique identifier for the entity in a {@link
 * PrimaryIndex}.
 *
 * <p>{@link PrimaryKey} may appear on at most one declared field per
 * class.</p>
 *
 * <p>Primary key values may be automatically assigned as sequential integers
 * using a {@link #sequence}.  In this case the type of the key field is
 * restricted to a simple integer type.</p>
 *
 * <p>A primary key field may not be null, unless it is being assigned from a
 * sequence.</p>
 *
 * <p><a name="keyTypes"><strong>Key Field Types</strong></a></p>
 *
 * <p>The type of a key field must either be one of the following:</p>
 * <ul>
 * <li>Any of the {@link <a href="Entity.html#simpleTypes">simple
 * types</a>}.</li>
 * <li>An enum type.</p>
 * <li>A composite key class containing one or more simple type or enum
 * fields.</li>
 * </ul>
 * <p>Array types are not allowed.</p>
 *
 * <p>When using a composite key class, each field of the composite key class
 * must be annotated with {@link KeyField} to identify the storage order and
 * default sort order.  See {@link KeyField} for an example and more
 * information on composite keys.</p>
 *
 * <p><a name="sortOrder"><strong>Key Sort Order</strong></a></p>
 *
 * <p>Key field types, being simple types, have a well defined and reasonable
 * default sort order, described below.  This sort order is based on a storage
 * encoding that allows a fast byte-by-byte comparison.</p>
 * <ul>
 * <li>All simple types except for {@code String} are encoded so that they are
 * sorted as expected, that is, as if the {@link Comparable#compareTo} method
 * of their class (or, for primitives, their wrapper class) is called.</li>
 * <br>
 * <li>Strings are encoded as UTF-8 byte arrays.  Zero (0x0000) character
 * values are UTF encoded as non-zero values, and therefore embedded zeros in
 * the string are supported.  The sequence {@literal {0xC0,0x80}} is used to
 * encode a zero character.  This UTF encoding is the same one used by native
 * Java UTF libraries.  However, this encoding of zero does impact the
 * lexicographical ordering, and zeros will not be sorted first (the natural
 * order) or last.  For all character values other than zero, the default UTF
 * byte ordering is the same as the Unicode lexicographical character
 * ordering.</li>
 * </ul>
 *
 * <p>When using a composite key class with more than one field, the sorting
 * order among fields is determined by the {@link KeyField} annotations.  To
 * override the default sort order, you can use a composite key class that
 * implements {@link Comparable}.  This allows overriding the sort order and is
 * therefore useful even when there is only one key field in the composite key
 * class.  See {@link <a href="KeyField.html#comparable">Custom Sort Order</a>}
 * for more information on sorting of composite keys.</p>
 *
 * <p><a name="inherit"><strong>Inherited Primary Key</strong></a></p>
 *
 * <p>If it does not appear on a declared field in the entity class, {@code
 * PrimaryKey} must appear on a field of an entity superclass.  In the
 * following example, the primary key on the base class is used:</p>
 *
 * <pre class="code">
 * {@literal @Persistent}
 * class BaseClass {
 *     {@literal @PrimaryKey}
 *     long id;
 *     ...
 * }
 * {@literal @Entity}
 * class Employee extends BaseClass {
 *     // inherits id primary key
 *     ...
 * }</pre>
 *
 * <p>If more than one class with {@code PrimaryKey} is present in a class
 * hierarchy, the key in the most derived class is used.  In this case, primary
 * key fields in superclasses are "shadowed" and are not persistent.  In the
 * following example, the primary key in the base class is not used and is not
 * persistent:</p>
 * <pre class="code">
 * {@literal @Persistent}
 * class BaseClass {
 *     {@literal @PrimaryKey}
 *     long id;
 *     ...
 * }
 * {@literal @Entity}
 * class Employee extends BaseClass {
 *     // overrides id primary key
 *     {@literal @PrimaryKey}
 *     String uuid;
 *     ...
 * }</pre>
 *
 * <p>Note that a {@code PrimaryKey} is not allowed on entity subclasses.  The
 * following is illegal and will cause an {@code IllegalArgumentException} when
 * trying to store an {@code Employee} instance:</p>
 * <pre class="code">
 * {@literal @Entity}
 * class Person {
 *     {@literal @PrimaryKey}
 *     long id;
 *     ...
 * }
 * {@literal @Persistent}
 * class Employee extends Person {
 *     {@literal @PrimaryKey}
 *     String uuid;
 *     ...
 * }</pre>
 *
 * @author Mark Hayes
 */
@Documented @Retention(RUNTIME) @Target(FIELD)
public @interface PrimaryKey {

    /**
     * The name of a sequence from which to assign primary key values
     * automatically.  If a non-empty string is specified, sequential integers
     * will be assigned from the named sequence.
     *
     * <p>A single sequence may be used for more than one entity class by
     * specifying the same sequence name for each {@code PrimaryKey}.  For
     * each named sequence, a {@link com.sleepycat.db.Sequence} will be used to
     * assign key values.  For more information on configuring sequences, see
     * {@link EntityStore#setSequenceConfig EntityStore.setSequenceConfig}.</p>
     *
     * <p>To use a sequence, the type of the key field must be a primitive
     * integer type ({@code byte}, {@code short}, {@code int} or {@code long})
     * or the primitive wrapper class for one of these types.  A composite key
     * class may also be used to override sort order, but it may contain only a
     * single key field that has one of the types previously mentioned.</p>
     *
     * <p>When an entity with a primary key sequence is stored using one of the
     * <code>put</code> methods in the {@link PrimaryIndex}, a new key will be
     * assigned if the primary key field in the entity instance is null (for a
     * reference type) or zero (for a primitive integer type).  Specifying zero
     * for a primitive integer key field is allowed because the initial value
     * of the sequence is one (not zero) by default.  If the sequence
     * configuration is changed such that zero is part of the sequence, then
     * the field type must be a primitive wrapper class and the field value
     * must be null to cause a new key to be assigned.</p>
     *
     * <p>When one of the <code>put</code> methods in the {@link PrimaryIndex}
     * is called and a new key is assigned, the assigned value is returned to
     * the caller via the key field of the entity object that is passed as a
     * parameter.</p>
     */
    String sequence() default "";
}

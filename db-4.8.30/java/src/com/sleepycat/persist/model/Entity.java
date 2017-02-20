/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.model;

import static java.lang.annotation.ElementType.TYPE;
import static java.lang.annotation.RetentionPolicy.RUNTIME;

import java.lang.annotation.Documented;
import java.lang.annotation.Retention;
import java.lang.annotation.Target;

import com.sleepycat.persist.EntityStore;
import com.sleepycat.persist.PrimaryIndex;
import com.sleepycat.persist.SecondaryIndex;
import com.sleepycat.persist.evolve.IncompatibleClassException;
import com.sleepycat.persist.evolve.Mutations;

/**
 * Indicates a persistent entity class.  For each entity class, a {@link
 * PrimaryIndex} can be used to store and access instances of that class.
 * Optionally, one or more {@link SecondaryIndex} objects may be used to access
 * entity instances by secondary key.
 *
 * <p><strong>Entity Subclasses and Superclasses</strong></p>
 *
 * <p>An entity class may have any number of subclasses and superclasses;
 * however, none of these may themselves be entity classes (annotated with
 * {@code Entity}).</p>
 *
 * <p>Entity superclasses are used to share common definitions and instance
 * data.  For example, fields in an entity superclass may be defined as primary
 * or secondary keys.</p>
 *
 * <p>Entity subclasses are used to provide polymorphism within a single {@code
 * PrimaryIndex}.  Instances of the entity class and its subclasses are stored
 * in the same {@code PrimaryIndex}.  Fields in an entity subclass may be
 * defined as secondary keys.</p>
 *
 * <p>For example, the following {@code BaseClass} defines the primary key for
 * any number of entity classes, using a single sequence to assign primary key
 * values.  The entity class {@code Pet} extends the base class, implicitly
 * defining a primary index that will contain instances of it and its
 * subclasses, including {@code Cat} which is defined below.  The primary key
 * ({@code id}) and secondary key ({@code name}) can be used to retrieve any
 * {@code Pet} instance.</p>
 * <pre class="code">
 *  {@literal @Persistent}
 *  class BaseClass {
 *      {@literal @PrimaryKey(sequence="ID")}
 *      long id;
 *  }
 *
 *  {@literal @Entity}
 *  class Pet extends BaseClass {
 *      {@literal @SecondaryKey(relate=ONE_TO_ONE)}
 *      String name;
 *      float height;
 *      float weight;
 *  }</pre>
 *
 * <p>The entity subclass {@code Cat} defines a secondary key ({@code
 * finickyness}) that only applies to {@code Cat} instances.  Querying by this
 * key will never retrieve a {@code Dog} instance, if such a subclass existed,
 * because a {@code Dog} instance will never contain a {@code finickyness}
 * key.</p>
 * <pre class="code">
 *  {@literal @Persistent}
 *  class Cat extends Pet {
 *      {@literal @SecondaryKey(relate=MANY_TO_ONE)}
 *      int finickyness;
 *  }</pre>
 *
 * <p><em>WARNING:</em> Entity subclasses that define secondary keys must be
 * registered prior to storing an instance of the class.  This can be done in
 * two ways:</p>
 * <ol>
 * <li>The {@link EntityModel#registerClass registerClass} method may be called
 * to register the subclass before opening the entity store.</li>
 * <li>The {@link EntityStore#getSubclassIndex getSubclassIndex} method may be
 * called to implicitly register the subclass after opening the entity
 * store.</li>
 * </ol>
 *
 * <p><strong>Persistent Fields and Types</strong></p>
 *
 * <p>All non-transient instance fields of an entity class, as well as its
 * superclasses and subclasses, are persistent.  {@code static} and {@code
 * transient} fields are not persistent.  The persistent fields of a class may
 * be {@code private}, package-private (default access), {@code protected} or
 * {@code public}.</p>
 *
 * <p>It is worthwhile to note the reasons that object persistence is defined
 * in terms of fields rather than properties (getters and setters).  This
 * allows business methods (getters and setters) to be defined independently of
 * the persistent state of an object; for example, a setter method may perform
 * validation that could not be performed if it were called during object
 * deserialization.  Similarly, this allows public methods to evolve somewhat
 * independently of the (typically non-public) persistent fields.</p>
 *
 * <p><a name="simpleTypes"><strong>Simple Types</strong></a></p>
 *
 * <p>Persistent types are divided into simple types, enum types, complex
 * types, and array types.  Simple types and enum types are single valued,
 * while array types may contain multiple elements and complex types may
 * contain one or more named fields.</p>
 *
 * <p>Simple types include:</p>
 * <ul>
 * <li>Java primitive types: {@code boolean, char, byte, short, int, long,
 * float, double}</p>
 * <li>The wrapper classes for Java primitive types</p>
 * <!--
 * <li>{@link java.math.BigDecimal}</p>
 * -->
 * <li>{@link java.math.BigInteger}</p>
 * <li>{@link java.lang.String}</p>
 * <li>{@link java.util.Date}</p>
 * </ul>
 *
 * <p>When null values are required (for optional key fields, for example),
 * primitive wrapper classes must be used instead of primitive types.</p>
 *
 * <p>Simple types, enum types and array types do not require annotations to
 * make them persistent.</p>
 *
 * <p><a name="proxyTypes"><strong>Complex and Proxy Types</strong></a></p>
 *
 * <p>Complex persistent classes must be annotated with {@link Entity} or
 * {@link Persistent}, or must be proxied by a persistent proxy class
 * (described below).  This includes entity classes, subclasses and
 * superclasses, and all other complex classes referenced via fields of these
 * classes.</p>
 *
 * <p>All complex persistent classes must have a default constructor.  The
 * default constructor may be {@code private}, package-private (default
 * access), {@code protected}, or {@code public}.  Other constructors are
 * allowed but are not used by the persistence mechanism.</p>
 *
 * <p>It is sometimes desirable to store instances of a type that is externally
 * defined and cannot be annotated or does not have a default constructor; for
 * example, a class defined in the Java standard libraries or a 3rd party
 * library.  In this case, a {@link PersistentProxy} class may be used to
 * represent the stored values for the externally defined type.  The proxy
 * class itself must be annotated with {@link Persistent} like other persistent
 * classes, and the {@link Persistent#proxyFor} property must be specified.</p>
 *
 * <p>For convenience, built-in proxy classes are included for several common
 * classes (listed below) in the Java library.  If you wish, you may define
 * your own {@link PersistentProxy} to override these built-in proxies.</p>
 * <ul>
 * <li>{@link java.util.HashSet}</li>
 * <li>{@link java.util.TreeSet}</li>
 * <li>{@link java.util.HashMap}</li>
 * <li>{@link java.util.TreeMap}</li>
 * <li>{@link java.util.ArrayList}</li>
 * <li>{@link java.util.LinkedList}</li>
 * </ul>
 *
 * <p>Complex persistent types should in general be application-defined
 * classes.  This gives the application control over the persistent state and
 * its evolution over time.</p>
 *
 * <p><strong>Other Type Restrictions</strong></p>
 *
 * <p>Entity classes and subclasses may not be used in field declarations for
 * persistent types.  Fields of entity classes and subclasses must be simple
 * types or non-entity persistent types (annotated with {@link Persistent} not
 * with {@link Entity}).</p>
 *
 * <p>Entity classes, subclasses and superclasses may be {@code abstract} and
 * may implement arbitrary interfaces.  Interfaces do not need to be annotated
 * with {@link Persistent} in order to be used in a persistent class, since
 * interfaces do not contain instance fields.</p>
 *
 * <p>Persistent instances of static nested classes are allowed, but the nested
 * class must be annotated with {@link Persistent} or {@link Entity}.  Inner
 * classes (non-static nested classes, including anonymous classes) are not
 * currently allowed as persistent types.</p>
 *
 * <p>Arrays of simple and persistent complex types are allowed as fields of
 * persistent types.  Arrays may be multidimensional.  However, an array may
 * not be stored as a top level instance in a primary index.  Only instances of
 * entity classes and subclasses may be top level instances in a primary
 * index.</p>
 *
 * <p><strong>Embedded Objects</strong></p>
 *
 * <p>As stated above, the embedded (or member) non-transient non-static fields
 * of an entity class are themselves persistent and are stored along with their
 * parent entity object.  This allows embedded objects to be stored in an
 * entity to an arbitrary depth.</p>
 *
 * <p>There is no arbitrary limit to the nesting depth of embedded objects
 * within an entity; however, there is a practical limit.  When an entity is
 * marshalled, each level of nesting is implemented internally via recursive
 * method calls.  If the nesting depth is large enough, a {@code
 * StackOverflowError} can occur.  In practice, this has been observed with a
 * nesting depth of 12,000, using the default Java stack size.</p>
 *
 * <p>This restriction on the nesting depth of embedded objects does not apply
 * to cyclic references, since these are handled specially as described
 * below.</p>
 *
 * <p><strong>Object Graphs</strong></p>
 *
 * <p>When an entity instance is stored, the graph of objects referenced via
 * its fields is stored and retrieved as a graph.  In other words, if a single
 * instance is referenced by two or more fields when the entity is stored, the
 * same will be true when the entity is retrieved.</p>
 *
 * <p>When a reference to a particular object is stored as a member field
 * inside that object or one of its embedded objects, this is called a cyclic
 * reference.  Because multiple references to a single object are stored as
 * such, cycles are also represented correctly and do not cause infinite
 * recursion or infinite processing loops.  If an entity containing a cyclic
 * reference is stored, the cyclic reference will be present when the entity is
 * retrieved.</p>
 *
 * <p>Note that the stored object graph is restricted in scope to a single
 * entity instance.  This is because each entity instance is stored separately.
 * If two entities have a reference to the same object when stored, they will
 * refer to two separate instances when the entities are retrieved.</p>
 *
 * @see Persistent
 * @see PrimaryKey
 * @see SecondaryKey
 * @see KeyField
 *
 * @author Mark Hayes
 */
@Documented @Retention(RUNTIME) @Target(TYPE)
public @interface Entity {

    /**
     * Identifies a new version of a class when an incompatible class change
     * has been made.  Prior versions of a class are referred to by version
     * number to perform class evolution and conversion using {@link
     * Mutations}.
     *
     * <p>The first version of a class is version zero, if {@link #version} is
     * not specified.  When an incompatible class change is made, a version
     * number must be assigned using {@link #version} that is higher than the
     * previous version number for the class.  If this is not done, an {@link
     * IncompatibleClassException} will be thrown when the store is opened.</p>
     */
    int version() default 0;
}

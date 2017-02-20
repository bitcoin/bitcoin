/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.evolve;

import java.lang.reflect.Method;

/**
 * A mutation for converting an old version of an object value to conform to
 * the current class or field definition.  For example:
 *
 * <pre class="code">
 *  package my.package;
 *
 *  // The old class.  Version 0 is implied.
 *  //
 *  {@literal @Entity}
 *  class Person {
 *      // ...
 *  }
 *
 *  // The new class.  A new version number must be assigned.
 *  //
 *  {@literal @Entity(version=1)}
 *  class Person {
 *      // Incompatible changes were made here...
 *  }
 *
 *  // Add a converter mutation.
 *  //
 *  Mutations mutations = new Mutations();
 *
 *  mutations.addConverter(new Converter(Person.class.getName(), 0,
 *                                       new MyConversion()));
 *
 *  // Configure the mutations as described {@link Mutations here}.</pre>
 *
 * <p>See {@link Conversion} for more information.</p>
 *
 * @see com.sleepycat.persist.evolve Class Evolution
 * @author Mark Hayes
 */
public class Converter extends Mutation {

    private static final long serialVersionUID = 4558176842096181863L;

    private Conversion conversion;

    /**
     * Creates a mutation for converting all instances of the given class
     * version to the current version of the class.
     */
    public Converter(String className,
                     int classVersion,
                     Conversion conversion) {
        this(className, classVersion, null, conversion);
    }

    /**
     * Creates a mutation for converting all values of the given field in the
     * given class version to a type compatible with the current declared type
     * of the field.
     */
    public Converter(String declaringClassName,
                     int declaringClassVersion,
                     String fieldName,
                     Conversion conversion) {
        super(declaringClassName, declaringClassVersion, fieldName);
        this.conversion = conversion;

        /* Require explicit implementation of the equals method. */
        Class cls = conversion.getClass();
        try {
            Method m = cls.getMethod("equals", Object.class);
            if (m.getDeclaringClass() == Object.class) {
                throw new IllegalArgumentException
                    ("Conversion class does not implement the equals method " +
                     "explicitly (Object.equals is not sufficient): " +
                     cls.getName());
            }
        } catch (NoSuchMethodException e) {
            throw new IllegalStateException(e);
        }
    }

    /**
     * Returns the converter instance specified to the constructor.
     */
    public Conversion getConversion() {
        return conversion;
    }

    /**
     * Returns true if the conversion objects are equal in this object and
     * given object, and if the {@link Mutation#equals} superclass method
     * returns true.
     */
    @Override
    public boolean equals(Object other) {
        if (other instanceof Converter) {
            Converter o = (Converter) other;
            return conversion.equals(o.conversion) &&
                   super.equals(other);
        } else {
            return false;
        }
    }

    @Override
    public int hashCode() {
        return conversion.hashCode() + super.hashCode();
    }

    @Override
    public String toString() {
        return "[Converter " + super.toString() +
               " Conversion: " + conversion + ']';
    }
}

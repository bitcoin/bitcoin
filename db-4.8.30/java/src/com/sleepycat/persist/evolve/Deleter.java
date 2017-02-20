/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.evolve;

/**
 * A mutation for deleting an entity class or field.
 *
 * <p><strong>WARNING:</strong> The data for the deleted class or field will be
 * destroyed and will be recoverable only by restoring from backup.  If you
 * wish to convert the instance data to a different type or format, use a
 * {@link Conversion} mutation instead.</p>
 *
 * <p>For example, to delete a field:</p>
 *
 * <pre class="code">
 *  package my.package;
 *
 *  // The old class.  Version 0 is implied.
 *  //
 *  {@literal @Entity}
 *  class Person {
 *      String name;
 *      String favoriteColors;
 *  }
 *
 *  // The new class.  A new version number must be assigned.
 *  //
 *  {@literal @Entity(version=1)}
 *  class Person {
 *      String name;
 *  }
 *
 *  // Add the mutation for deleting a field.
 *  //
 *  Mutations mutations = new Mutations();
 *
 *  mutations.addDeleter(new Deleter(Person.class.getName(), 0,
 *                                   "favoriteColors");
 *
 *  // Configure the mutations as described {@link Mutations here}.</pre>
 *
 * <p>To delete an entity class:</p>
 *
 * <pre class="code">
 *  package my.package;
 *
 *  // The old class.  Version 0 is implied.
 *  //
 *  {@literal @Entity}
 *  class Statistics {
 *      ...
 *  }
 *
 *  // Add the mutation for deleting a class.
 *  //
 *  Mutations mutations = new Mutations();
 *
 *  mutations.addDeleter(new Deleter("my.package.Statistics", 0));
 *
 *  // Configure the mutations as described {@link Mutations here}.</pre>
 *
 * @see com.sleepycat.persist.evolve Class Evolution
 * @author Mark Hayes
 */
public class Deleter extends Mutation {

    private static final long serialVersionUID = 446348511871654947L;

    /**
     * Creates a mutation for deleting an entity class.
     */
    public Deleter(String className, int classVersion) {
        super(className, classVersion, null);
    }

    /**
     * Creates a mutation for deleting the given field from all instances of
     * the given class version.
     */
    public Deleter(String declaringClass, int declaringClassVersion,
                   String fieldName) {
        super(declaringClass, declaringClassVersion, fieldName);
    }

    @Override
    public String toString() {
        return "[Deleter " + super.toString() + ']';
    }
}

/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.evolve;

import java.util.Collections;
import java.util.HashSet;
import java.util.Set;

/**
 * A subclass of Converter that allows specifying keys to be deleted.
 *
 * <p>When a Converter is used with an entity class, secondary keys cannot be
 * automatically deleted based on field deletion, because field Deleter objects
 * are not used in conjunction with a Converter mutation.  The EntityConverter
 * can be used instead of a plain Converter to specify the key names to be
 * deleted.</p>
 *
 * <p>It is not currently possible to rename or insert secondary keys when
 * using a Converter mutation with an entity class.</p>
 *
 * @see Converter
 * @see com.sleepycat.persist.evolve Class Evolution
 * @author Mark Hayes
 */
public class EntityConverter extends Converter {

    private static final long serialVersionUID = -988428985370593743L;

    private Set<String> deletedKeys;

    /**
     * Creates a mutation for converting all instances of the given entity
     * class version to the current version of the class.
     */
    public EntityConverter(String entityClassName,
                           int classVersion,
                           Conversion conversion,
                           Set<String> deletedKeys) {
        super(entityClassName, classVersion, null, conversion);

        /* Eclipse objects to assigning with a ternary operator. */
        if (deletedKeys != null) {
            this.deletedKeys = new HashSet(deletedKeys);
        } else {
            this.deletedKeys = Collections.emptySet();
        }
    }

    /**
     * Returns the set of key names that are to be deleted.
     */
    public Set<String> getDeletedKeys() {
        return Collections.unmodifiableSet(deletedKeys);
    }

    /**
     * Returns true if the deleted and renamed keys are equal in this object
     * and given object, and if the {@link Converter#equals} superclass method
     * returns true.
     */
    @Override
    public boolean equals(Object other) {
        if (other instanceof EntityConverter) {
            EntityConverter o = (EntityConverter) other;
            return deletedKeys.equals(o.deletedKeys) &&
                   super.equals(other);
        } else {
            return false;
        }
    }

    @Override
    public int hashCode() {
        return deletedKeys.hashCode() + super.hashCode();
    }

    @Override
    public String toString() {
        return "[EntityConverter " + super.toString() +
               " DeletedKeys: " + deletedKeys + ']';
    }
}

/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.model;

/**
 * The metadata for a primary key field.  A primary key may be specified with
 * the {@link PrimaryKey} annotation.
 *
 * <p>{@code PrimaryKeyMetadata} objects are thread-safe.  Multiple threads may
 * safely call the methods of a shared {@code PrimaryKeyMetadata} object.</p>
 *
 * @author Mark Hayes
 */
public class PrimaryKeyMetadata extends FieldMetadata {

    private static final long serialVersionUID = 2946863622972437018L;

    private String sequenceName;

    /**
     * Used by an {@code EntityModel} to construct primary key metadata.
     */
    public PrimaryKeyMetadata(String name,
                              String className,
                              String declaringClassName,
                              String sequenceName) {
        super(name, className, declaringClassName);
        this.sequenceName = sequenceName;
    }

    /**
     * Returns the name of the sequence for assigning key values.  This may be
     * specified using the {@link PrimaryKey#sequence} annotation.
     */
    public String getSequenceName() {
        return sequenceName;
    }

    @Override
    public boolean equals(Object other) {
        if (other instanceof PrimaryKeyMetadata) {
            PrimaryKeyMetadata o = (PrimaryKeyMetadata) other;
            return super.equals(o) &&
                   ClassMetadata.nullOrEqual(sequenceName, o.sequenceName);
        } else {
            return false;
        }
    }

    @Override
    public int hashCode() {
        return super.hashCode() + ClassMetadata.hashCode(sequenceName);
    }
}

/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.model;

/**
 * Defines the relationship between instances of the entity class and the
 * secondary keys.  This can be specified using a {@link SecondaryKey#relate}
 * annotation.
 *
 * @author Mark Hayes
 */
public enum Relationship {

    /**
     * Relates many entities to one secondary key.
     *
     * <p>The secondary index will have non-unique keys; in other words,
     * duplicates will be allowed.</p>
     *
     * <p>The secondary key field is singular, in other words, it may not be an
     * array or collection type.</p>
     */
    MANY_TO_ONE,

    /**
     * Relates one entity to many secondary keys.
     *
     * <p>The secondary index will have unique keys, in other words, duplicates
     * will not be allowed.</p>
     *
     * <p>The secondary key field must be an array or collection type.</p>
     */
    ONE_TO_MANY,

    /**
     * Relates many entities to many secondary keys.
     *
     * <p>The secondary index will have non-unique keys, in other words,
     * duplicates will be allowed.</p>
     *
     * <p>The secondary key field must be an array or collection type.</p>
     */
    MANY_TO_MANY,

    /**
     * Relates one entity to one secondary key.
     *
     * <p>The secondary index will have unique keys, in other words, duplicates
     * will not be allowed.</p>
     *
     * <p>The secondary key field is singular, in other words, it may not be an
     * array or collection type.</p>
     */
    ONE_TO_ONE;
}

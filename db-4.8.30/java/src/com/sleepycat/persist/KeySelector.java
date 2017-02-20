/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist;

/**
 * This is package-private to hide it until we implemented unsorted access.
 *
 * Implemented to select keys to be returned by an unsorted {@code
 * ForwardCursor}.
 *
 * <p>The reason for implementing a selector, rather than filtering the objects
 * returned by the {@link ForwardCursor}, is to improve performance when not
 * all keys are to be processed.  Keys are passed to this interface without
 * retrieving record data or locking, so it is less expensive to return false
 * from this method than to retrieve the object from the cursor.</p>
 *
 * see EntityIndex#unsortedKeys
 * see EntityIndex#unsortedEntities
 *
 * @author Mark Hayes
 */
interface KeySelector<K> {

    /**
     * Returns whether a given key should be returned via the cursor.
     *
     * <p>This method should not assume that the given key is for a committed
     * record or not, nor should it assume that the key will be returned via
     * the cursor if this method returns true.  The record for this key will
     * not be locked until this method returns.  If, when the record is locked,
     * the record is found to be uncommitted or deleted, the key will not be
     * returned via the cursor.</p>
     */
    boolean selectKey(K key);
}

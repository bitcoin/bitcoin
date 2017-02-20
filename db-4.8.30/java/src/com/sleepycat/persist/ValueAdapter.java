/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist;

import com.sleepycat.db.DatabaseEntry;

/**
 * An adapter that translates between database entries (key, primary key, data)
 * and a "value", which may be either the key, primary key, or entity.  This
 * interface is used to implement a generic index and cursor (BasicIndex and
 * BasicCursor).  If we didn't use this approach, we would need separate index
 * and cursor implementations for each type of value that can be returned.  In
 * other words, this interface is used to reduce class explosion.
 *
 * @author Mark Hayes
 */
interface ValueAdapter<V> {

    /**
     * Creates a DatabaseEntry for the key or returns null if the key is not
     * needed.
     */
    DatabaseEntry initKey();

    /**
     * Creates a DatabaseEntry for the primary key or returns null if the
     * primary key is not needed.
     */
    DatabaseEntry initPKey();

    /**
     * Creates a DatabaseEntry for the data or returns null if the data is not
     * needed.  BasicIndex.NO_RETURN_ENTRY may be returned if the data argument
     * is required but we don't need it.
     */
    DatabaseEntry initData();

    /**
     * Sets the data array of the given entries to null, based on knowledge of
     * which entries are non-null and are not NO_RETURN_ENTRY.
     */
    void clearEntries(DatabaseEntry key,
                      DatabaseEntry pkey,
                      DatabaseEntry data);

    /**
     * Returns the appropriate "value" (key, primary key, or entity) using the
     * appropriate bindings for that purpose.
     */
    V entryToValue(DatabaseEntry key,
                   DatabaseEntry pkey,
                   DatabaseEntry data);

    /**
     * Converts an entity value to a data entry using an entity binding, or
     * throws UnsupportedOperationException if this is not appropriate.  Called
     * by BasicCursor.update.
     */
    void valueToData(V value, DatabaseEntry data);
}

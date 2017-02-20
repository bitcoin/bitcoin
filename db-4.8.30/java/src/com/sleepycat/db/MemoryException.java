/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */
package com.sleepycat.db;

import com.sleepycat.db.internal.DbEnv;

/**
This exception is thrown when a {@link com.sleepycat.db.DatabaseEntry DatabaseEntry}
passed to a {@link com.sleepycat.db.Database Database} or {@link com.sleepycat.db.Cursor Cursor} method is not large
enough to hold a value being returned.  This only applies to
{@link com.sleepycat.db.DatabaseEntry DatabaseEntry} objects configured with the
{@link com.sleepycat.db.DatabaseEntry#setUserBuffer DatabaseEntry.setUserBuffer} method.
In a Java Virtual Machine, there are usually separate heaps for memory
allocated by native code and for objects allocated in Java code.  If the
Java heap is exhausted, the JVM will throw an
{@link java.lang.OutOfMemoryError}, so you may see that exception
rather than this one.
*/
public class MemoryException extends DatabaseException {
    private DatabaseEntry dbt = null;
    private String message;

    /* package */ MemoryException(final String s,
                              final DatabaseEntry dbt,
                              final int errno,
                              final DbEnv dbenv) {
        super(s, errno, dbenv);
        this.message = s;
        this.dbt = dbt;
    }

    /**
    Returns the {@link com.sleepycat.db.DatabaseEntry DatabaseEntry} object with insufficient memory
    to complete the operation to complete the operation.
    */
    public DatabaseEntry getDatabaseEntry() {
        return dbt;
    }

    /** {@inheritDoc} */
    public String toString() {
        return message;
    }

    void updateDatabaseEntry(final DatabaseEntry newEntry) {
        if (this.dbt == null) {
            this.message = "DatabaseEntry not large enough for available data";
            this.dbt = newEntry;
        }
    }
}

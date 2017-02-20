/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbConstants;

/** Database types. */
public final class DatabaseType {
    /**
    The database is a Btree.  The Btree format is a representation of a
    sorted, balanced tree structure.
    */
    public static final DatabaseType BTREE =
        new DatabaseType("BTREE", DbConstants.DB_BTREE);

    /**
    The database is a Hash.  The Hash format is an extensible, dynamic
    hashing scheme.
    */
    public static final DatabaseType HASH =
        new DatabaseType("HASH", DbConstants.DB_HASH);

    /**
    The database is a Queue.  The Queue format supports fast access to
    fixed-length records accessed sequentially or by logical record
    number.
    */
    public static final DatabaseType QUEUE =
        new DatabaseType("QUEUE", DbConstants.DB_QUEUE);

    /**
    The database is a Recno.  The Recno format supports fixed- or
    variable-length records, accessed sequentially or by logical
    record number, and optionally backed by a flat text file.
    */
    public static final DatabaseType RECNO =
        new DatabaseType("RECNO", DbConstants.DB_RECNO);

    /**
    The database type is unknown.
    */
    public static final DatabaseType UNKNOWN =
        new DatabaseType("UNKNOWN", DbConstants.DB_UNKNOWN);

    /* package */
    static DatabaseType fromInt(int type) {
        switch(type) {
        case DbConstants.DB_BTREE:
            return BTREE;
        case DbConstants.DB_HASH:
            return HASH;
        case DbConstants.DB_QUEUE:
            return QUEUE;
        case DbConstants.DB_RECNO:
            return DatabaseType.RECNO;
        case DbConstants.DB_UNKNOWN:
            return DatabaseType.UNKNOWN;
        default:
            throw new IllegalArgumentException(
                "Unknown database type: " + type);
        }
    }

    private String statusName;
    private int id;

    private DatabaseType(final String statusName, final int id) {
        this.statusName = statusName;
        this.id = id;
    }

    /* package */
    int getId() {
        return id;
    }

    /** {@inheritDoc} */
    public String toString() {
        return "DatabaseType." + statusName;
    }
}

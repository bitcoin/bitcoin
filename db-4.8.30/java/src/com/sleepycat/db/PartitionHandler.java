/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

/**
An interface specifying how Btree prefixes should be calculated.
*/
public interface PartitionHandler {
    /**
    The application-specific database partitioning callback.
    <p>
    @param db
    The enclosing database handle.
    @param key
    A database entry representing a database key.
    */
    int partition(Database db, DatabaseEntry key);
}

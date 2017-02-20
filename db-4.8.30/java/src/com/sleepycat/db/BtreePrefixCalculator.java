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
public interface BtreePrefixCalculator {
    /**
    The application-specific Btree prefix callback.
    <p>
    @param db
    The enclosing database handle.
    @param dbt1
    A database entry representing a database key.
    @param dbt2
    A database entry representing a database key.
    */
    int prefix(Database db, DatabaseEntry dbt1, DatabaseEntry dbt2);
}

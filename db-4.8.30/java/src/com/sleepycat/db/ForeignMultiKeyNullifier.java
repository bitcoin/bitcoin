/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 20008-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

public interface ForeignMultiKeyNullifier {
    boolean nullifyForeignKey(SecondaryDatabase secondary, DatabaseEntry key, DatabaseEntry data, DatabaseEntry secKey)
	    throws DatabaseException;
}

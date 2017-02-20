/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2008-2009 Oracle.  All rights reserved.
 *
 * $Id$ 
 */

package com.sleepycat.db;

public interface ForeignKeyNullifier {
    boolean nullifyForeignKey(SecondaryDatabase secondary, DatabaseEntry data)
	    throws DatabaseException;
}

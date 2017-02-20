/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbConstants;

public class ForeignKeyDeleteAction {
    private String name;
    private int id;

    private ForeignKeyDeleteAction(String name, int id) {
	this.name = name;
	this.id = id;
    }

    public String toString() {
	return "ForeignKeyDeleteAction." + name;
    }

    /* package */
    int getId() {
	return id;
    }

    static ForeignKeyDeleteAction fromInt(int type) {
	switch(type) {
	case DbConstants.DB_FOREIGN_ABORT:
	    return ABORT;
	case DbConstants.DB_FOREIGN_CASCADE:
	    return CASCADE;
	case DbConstants.DB_FOREIGN_NULLIFY:
	    return NULLIFY;
	default:
	    throw new IllegalArgumentException("Unknown action type: " + type);
	}
    }

    public static ForeignKeyDeleteAction ABORT =
	new ForeignKeyDeleteAction("ABORT", DbConstants.DB_FOREIGN_ABORT);
    public static ForeignKeyDeleteAction CASCADE =
	new ForeignKeyDeleteAction("CASCADE", DbConstants.DB_FOREIGN_CASCADE);
    public static ForeignKeyDeleteAction NULLIFY =
	new ForeignKeyDeleteAction("NULLIFY", DbConstants.DB_FOREIGN_NULLIFY);
}

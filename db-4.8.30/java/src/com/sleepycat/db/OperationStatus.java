/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbConstants;
import com.sleepycat.db.internal.DbEnv;

/**
Status values from database operations.
*/
public final class OperationStatus {
    /**
    The operation was successful.
    */
    public static final OperationStatus SUCCESS =
        new OperationStatus("SUCCESS", 0);
    /**
    The operation to insert data was configured to not allow overwrite
    and the key already exists in the database.
    */
    public static final OperationStatus KEYEXIST =
        new OperationStatus("KEYEXIST", DbConstants.DB_KEYEXIST);
    /**
    The cursor operation was unsuccessful because the current record
    was deleted.
    */
    public static final OperationStatus KEYEMPTY =
        new OperationStatus("KEYEMPTY", DbConstants.DB_KEYEMPTY);
    /**
    The requested key/data pair was not found.
    */
    public static final OperationStatus NOTFOUND =
        new OperationStatus("NOTFOUND", DbConstants.DB_NOTFOUND);

    /* package */
    static OperationStatus fromInt(final int errCode) {
        switch(errCode) {
        case 0:
            return SUCCESS;
        case DbConstants.DB_KEYEXIST:
            return KEYEXIST;
        case DbConstants.DB_KEYEMPTY:
            return KEYEMPTY;
        case DbConstants.DB_NOTFOUND:
            return NOTFOUND;
        default:
            throw new IllegalArgumentException(
                "Unknown error code: " + DbEnv.strerror(errCode));
        }
    }

    /* For toString */
    private String statusName;
    private int errCode;

    private OperationStatus(final String statusName, int errCode) {
        this.statusName = statusName;
        this.errCode = errCode;
    }

    /** {@inheritDoc} */
    public String toString() {
        return "OperationStatus." + statusName;
    }
}

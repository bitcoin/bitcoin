/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */
package com.sleepycat.db;

import com.sleepycat.db.internal.DbEnv;

/**
The root of all database exceptions.
<p>
Note that in some cases, certain methods return status values without issuing
an exception. This occurs in situations that are not normally considered an
error, but when some informational status is returned.  For example,
{@link com.sleepycat.db.Database#get Database.get} returns {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} when a
requested key does not appear in the database.
**/
public class DatabaseException extends Exception {
    private Environment dbenv;
    private int errno;

    /** Construct an exception with the specified cause exception. **/
    public DatabaseException(Throwable t) {
        super(t);
        errno = 0;
        dbenv = null;
    }

    /** Construct an exception with the specified message. **/
    public DatabaseException(final String s) {
        this(s, 0, (Environment)null);
    }

    /** Construct an exception with the specified message and error number. **/
    public DatabaseException(final String s, final int errno) {
        this(s, errno, (Environment)null);
    }

    /**
    Construct an exception with the specified message and error number
    associated with a database environment handle.
    **/
    public DatabaseException(final String s,
                             final int errno,
                             final Environment dbenv) {
        super(s);
        this.errno = errno;
        this.dbenv = dbenv;
    }

    /* package */ DatabaseException(final String s,
                                final int errno,
                                final DbEnv dbenv) {
        this(s, errno, (dbenv == null) ? null : dbenv.wrapper);
    }

    /**
Return the environment in which the exception occurred.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The environment in which the exception occurred.
    **/
    public Environment getEnvironment() {
        return dbenv;
    }

    /**
    Get the error number associated with this exception.
    <p>
    Note that error numbers can be returned from system calls
    and are system-specific.
    **/
    public int getErrno() {
        return errno;
    }

    /** {@inheritDoc} */
    public String toString() {
        String s = super.toString();
        if (errno != 0)
            s += ": " + DbEnv.strerror(errno);
        return s;
    }
}

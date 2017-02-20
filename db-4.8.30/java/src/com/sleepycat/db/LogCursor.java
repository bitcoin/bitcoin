/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbConstants;
import com.sleepycat.db.internal.DbLogc;

/**
The LogCursor object is the handle for a cursor into the log files,
supporting sequential access to the records stored in log files.
<p>
The handle is not free-threaded.  Once the {@link com.sleepycat.db.LogCursor#close LogCursor.close}
method is called, the handle may not be accessed again, regardless of
that method's success or failure.
*/
public class LogCursor {
    /* package */ DbLogc logc;

    /* package */ LogCursor(final DbLogc logc) {
        this.logc = logc;
    }

    /* package */
    static LogCursor wrap(DbLogc logc) {
        return (logc == null) ? null : new LogCursor(logc);
    }

    public synchronized void close()
        throws DatabaseException {

        logc.close(0);
    }

    /**
    Return the LogSequenceNumber and log record to which the log cursor
    currently refers.
    <p>
    @param lsn
    The returned LogSequenceNumber.
    <p>
    @param data
    The returned log record.  The data field is set to the record
    retrieved, and the size field indicates the number of bytes in
    the record.
    <p>
    @return
    The status of the operation.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public OperationStatus getCurrent(final LogSequenceNumber lsn,
                                      final DatabaseEntry data)
        throws DatabaseException {

        return OperationStatus.fromInt(
            logc.get(lsn, data, DbConstants.DB_CURRENT));
    }

    /**
    Return the next LogSequenceNumber and log record.
    <p>
    The current log position is advanced to the next record in the log,
    and its LogSequenceNumber and data are returned.  If the cursor has
    not been initialized, the first available log record in the log will
    be returned.
    <p>
    @param lsn
    The returned LogSequenceNumber.
    <p>
    @param data
    The returned log record.
    <p>
    @return
    The status of the operation; a return of NOTFOUND indicates the last
    log record has already been returned or the log is empty.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public OperationStatus getNext(final LogSequenceNumber lsn,
                                   final DatabaseEntry data)
        throws DatabaseException {

        return OperationStatus.fromInt(
            logc.get(lsn, data, DbConstants.DB_NEXT));
    }

    /**
    Return the first LogSequenceNumber and log record.
    <p>
    The current log position is set to the first record in the log,
    and its LogSequenceNumber and data are returned.
    <p>
    @param lsn
    The returned LogSequenceNumber.
    <p>
    @param data
    The returned log record.
    <p>
    @return
    The status of the operation; a return of NOTFOUND indicates the log
    is empty.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public OperationStatus getFirst(final LogSequenceNumber lsn,
                                    final DatabaseEntry data)
        throws DatabaseException {

        return OperationStatus.fromInt(
            logc.get(lsn, data, DbConstants.DB_FIRST));
    }

    /**
    Return the last LogSequenceNumber and log record.
    <p>
    The current log position is set to the last record in the log,
    and its LogSequenceNumber and data are returned.
    <p>
    @param lsn
    The returned LogSequenceNumber.
    <p>
    @param data
    The returned log record.
    <p>
    @return
    The status of the operation; a return of NOTFOUND indicates the log
    is empty.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public OperationStatus getLast(final LogSequenceNumber lsn,
                                   final DatabaseEntry data)
        throws DatabaseException {

        return OperationStatus.fromInt(
            logc.get(lsn, data, DbConstants.DB_LAST));
    }

    /**
    Return the previous LogSequenceNumber and log record.
    <p>
    The current log position is advanced to the previous record in the log,
    and its LogSequenceNumber and data are returned.  If the cursor has
    not been initialized, the last available log record in the log will
    be returned.
    <p>
    @param lsn
    The returned LogSequenceNumber.
    <p>
    @param data
    The returned log record.
    <p>
    @return
    The status of the operation; a return of NOTFOUND indicates the first
    log record has already been returned or the log is empty.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public OperationStatus getPrev(final LogSequenceNumber lsn,
                                   final DatabaseEntry data)
        throws DatabaseException {

        return OperationStatus.fromInt(
            logc.get(lsn, data, DbConstants.DB_PREV));
    }

    /**
    Return a specific log record.
    <p>
    The current log position is set to the specified record in the log,
    and its data is returned.
    <p>
    @param lsn
    The specified LogSequenceNumber.
    <p>
    @param data
    The returned log record.
    <p>
    @return
    The status of the operation.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public OperationStatus set(final LogSequenceNumber lsn,
                               final DatabaseEntry data)
        throws DatabaseException {

        return OperationStatus.fromInt(
            logc.get(lsn, data, DbConstants.DB_SET));
    }

    /**
    Get the log file version.
    <p>
    @return
    The log file version.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public int version()
        throws DatabaseException {

        return logc.version(0);
    }
}

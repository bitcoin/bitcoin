/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbEnv;

/**
The LogSequenceNumber object is a <em>log sequence number</em> which
specifies a unique location in a log file.  A LogSequenceNumber consists
of two unsigned 32-bit integers -- one specifies the log file number,
and the other specifies the offset in the log file.
*/
public class LogSequenceNumber {
    private int file;
    private int offset;

    /**
    Construct a LogSequenceNumber with the specified file and offset.
    <p>
    @param file
    The log file number.
    <p>
    @param offset
    The log file offset.
    */
    public LogSequenceNumber(final int file, final int offset) {
        this.file = file;
        this.offset = offset;
    }

    /**
    Construct an uninitialized LogSequenceNumber.
    */
    public LogSequenceNumber() {
        this(0, 0);
    }

    /**
    Return the file number component of the LogSequenceNumber.
    <p>
    @return
    The file number component of the LogSequenceNumber.
    */
    public int getFile() {
        return file;
    }

    /**
    Return the file offset component of the LogSequenceNumber.
    <p>
    @return
    The file offset component of the LogSequenceNumber.
    */
    public int getOffset() {
        return offset;
    }

    /**
    Compare two LogSequenceNumber objects.
    <p>
    This method returns 0 if the two LogSequenceNumber objects are
    equal, 1 if lsn0 is greater than lsn1, and -1 if lsn0 is less than
    lsn1.
    <p>
    @param lsn1
    The first LogSequenceNumber object to be compared.
    <p>
    @param lsn2
    The second LogSequenceNumber object to be compared.
    <p>
    @return
    0 if the two LogSequenceNumber objects are equal, 1 if lsn1 is
    greater than lsn2, and -1 if lsn1 is less than lsn2.
    */
    public static int compare(LogSequenceNumber lsn1, LogSequenceNumber lsn2) {
        return DbEnv.log_compare(lsn1, lsn2);
    }
}

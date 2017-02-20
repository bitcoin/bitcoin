/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbConstants;
import com.sleepycat.db.internal.DbUtil;

/**
A DatabaseEntry that holds multiple record number/data pairs returned by a
single {@link com.sleepycat.db.Database Database} or {@link com.sleepycat.db.Cursor Cursor} get call.
*/
public class MultipleRecnoDataEntry extends MultipleEntry {
    /**
    Construct an entry with no data. The object must be configured
    before use with the {@link com.sleepycat.db.MultipleEntry#setUserBuffer MultipleEntry.setUserBuffer} method.
    */
    public MultipleRecnoDataEntry() {
        super(null, 0, 0);
    }

    /**
    Construct an entry with a given byte array.  The offset is
    set to zero; the size is set to the length of the array.  If null
    is passed, the object must be configured before use with the
    {@link com.sleepycat.db.MultipleEntry#setUserBuffer MultipleEntry.setUserBuffer} method.
    <p>
    @param data
    Byte array wrapped by the entry.
    */
    public MultipleRecnoDataEntry(final byte[] data) {
        super(data, 0, (data == null) ? 0 : data.length);
    }

    /**
    Constructs a DatabaseEntry with a given byte array, offset and size.
    <p>
    @param data
    Byte array wrapped by the DatabaseEntry.
    @param offset
    Offset in the first byte in the byte array to be included.
    @param size
    Number of bytes in the byte array to be included.
    */
    public MultipleRecnoDataEntry(final byte[] data,
                                  final int offset,
                                  final int size) {
        super(data, offset, size);
    }

    /**
     * Return the bulk retrieval flag and reset the entry position so that the
     * next set of key/data can be returned.
     */
    /* package */
    int getMultiFlag() {
        pos = 0;
        return DbConstants.DB_MULTIPLE_KEY;
    }

    /**
    Get the next record number/data pair in the returned set.  This method
    may only be called after a successful call to a {@link com.sleepycat.db.Database Database} or
    {@link com.sleepycat.db.Cursor Cursor} get method with this object as the data parameter.
    <p>
    When used with the Queue and Recno access methods,
    <code>data.getData()<code> will return <code>null</code> for deleted
    records.
    <p>
    @param recnoEntry
    an entry that is set to refer to the next record number in the returned
    set.
    <p>
    @param data
    an entry that is set to refer to the next data element in the returned
    set.
    <p>
    @return
    indicates whether a value was found.  A return of <code>false</code>
    indicates that the end of the set was reached.
    */
    public boolean next(final DatabaseEntry recnoEntry, final DatabaseEntry data) {
        if (pos == 0)
            pos = ulen - INT32SZ;

        final int recno = DbUtil.array2int(this.data, pos);

        // crack out the key offset and the data offset and length.
        if (recno == 0)
            return false;

        pos -= INT32SZ;
        final int dataoff = DbUtil.array2int(this.data, pos);
        pos -= INT32SZ;
        final int datasz = DbUtil.array2int(this.data, pos);
        pos -= INT32SZ;

        recnoEntry.setRecordNumber(recno);
        data.setData(this.data);
        data.setOffset(dataoff);
        data.setSize(datasz);

        return true;
    }

    /**
    Append a record number / data item pair to the bulk buffer.
    <p>
    @param recno
    the record number of the record to be added.
    @param data
    an array containing the value to be added.
    @param offset
    the position in the <b>data</b> array where the record starts.
    @param len
    the length of the record, in bytes, to be copied from the <b>data</b> array.
    <p>
    @return
    indicates whether there was space.  A return of <code>false</code>
    indicates that the specified entry could not fit in the buffer.
    */
    public boolean append(int recno, final byte[] data, int offset, int len)
        throws DatabaseException {

        return append_internal(data, doff, dlen, recno);
    }

    /**
    Append an entry to the bulk buffer.
    <p>
    @param recno
    the record number of the record to be added.
    @param data
    the value to be appended, using the offset and size specified in the
    {@link com.sleepycat.db.DatabaseEntry DatabaseEntry}.
    <p>
    @return
    indicates whether there was space.  A return of <code>false</code>
    indicates that the specified entry could not fit in the buffer.
    */
    public boolean append(int recno, final DatabaseEntry data)
        throws DatabaseException {

        return append(recno, data.data, data.offset, data.size);
    }

    /**
    Append an entry to the bulk buffer.
    <p>
    @param recno
    the record number of the record to be added.
    @param data
    an array containing the value to be added.
    <p>
    @return
    indicates whether there was space.  A return of <code>false</code>
    indicates that the specified entry could not fit in the buffer.
    */
    public boolean append(int recno, final byte[] data)
        throws DatabaseException {

        return append(recno, data, 0, data.length);
    }
}

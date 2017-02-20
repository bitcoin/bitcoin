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

import java.nio.ByteBuffer;

/**
A DatabaseEntry that holds multiple data items returned by a single
{@link com.sleepycat.db.Database Database} or {@link com.sleepycat.db.Cursor Cursor} get call.
*/
public class MultipleNIODataEntry extends MultipleEntry {
    /**
    Construct an entry with no data. The object must be configured
    before use with the {@link com.sleepycat.db.DatabaseEntry#setDataNIO DatabaseEntry.setDataNIO} method.
    */
    public MultipleNIODataEntry() {
        super(null);
    }

    /**
    Construct an entry with a given java.nio.ByteBuffer.  The offset is
    set to zero; the size is set to the length of the java.nio.ByteBuffer.
    <p>
    @param data
    java.nio.ByteBuffer wrapped by the entry.
    */
    public MultipleNIODataEntry(final ByteBuffer data) {
        super(data);
    }

    /**
     * Return the bulk retrieval flag and reset the entry position so that the
     * next set of key/data can be returned.
     */
    /* package */
    int getMultiFlag() {
        pos = 0;
        return DbConstants.DB_MULTIPLE;
    }

    /**
    Get the next data element in the returned set.  This method may only
    be called after a successful call to a {@link com.sleepycat.db.Database Database} or
    {@link com.sleepycat.db.Cursor Cursor} get method with this object as the data parameter.
    <p>
    When used with the Queue and Recno access methods,
    <code>data.getData()<code> will return <code>null</code> for deleted
    records.
    <p>
    @param data
    an entry that is set to refer to the next data element in the returned
    set.
    <p>
    @return
    indicates whether a value was found.  A return of <code>false</code>
    indicates that the end of the set was reached.
    */
    public boolean next(final DatabaseEntry data) {
        byte[] intarr;
        int saveoffset;
        if (pos == 0)
            pos = ulen - INT32SZ;

        // pull the offsets out of the ByteBuffer.
        if(this.data_nio.capacity() < 8)
            return false;
        intarr = new byte[8];
        saveoffset = this.data_nio.position();
        this.data_nio.position(pos - INT32SZ);
        this.data_nio.get(intarr, 0, 8);
        this.data_nio.position(saveoffset);

        final int dataoff = DbUtil.array2int(intarr, 4);

        // crack out the data offset and length.
        if (dataoff < 0) {
            return (false);
        }

        final int datasz = DbUtil.array2int(intarr, 0);

        // move the position to one before the last offset read.
        pos -= INT32SZ*2;

        data.setDataNIO(this.data_nio);
        data.setSize(datasz);
        data.setOffset(dataoff);

        return (true);
    }
}

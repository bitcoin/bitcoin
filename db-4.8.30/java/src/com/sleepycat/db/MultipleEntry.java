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
An abstract class representing a DatabaseEntry that holds multiple results
returned by a single {@link com.sleepycat.db.Cursor Cursor} get method.  Use one of the concrete
subclasses depending on whether you need:
<ul>
<li>multiple data items only: {@link com.sleepycat.db.MultipleDataEntry MultipleDataEntry}</li>
<li>multiple key / data item pairs: {@link com.sleepycat.db.MultipleKeyDataEntry MultipleKeyDataEntry}</li>
<li>multiple record number / data item pairs:
{@link com.sleepycat.db.MultipleRecnoDataEntry MultipleRecnoDataEntry}</li>
</ul>
*/
public abstract class MultipleEntry extends DatabaseEntry {
    /* package */ int pos;

    /* package */ MultipleEntry(final byte[] data, final int offset, final int size) {
        super(data, offset, size);
        setUserBuffer((data != null) ? (data.length - offset) : 0, true);
        flags |= DbConstants.DB_DBT_BULK;
    }

    /* package */ MultipleEntry(final ByteBuffer data) {
        super(data);
        flags |= DbConstants.DB_DBT_BULK;
    }

    public void setUserBuffer(final int length, final boolean usermem) {
        if (!usermem)
            throw new IllegalArgumentException("User buffer required");
        super.setUserBuffer(length, usermem);
    }

    /*
     * A helper function for building DataEntry classes that contain multiple
     * entries.
     *
     * ASCII art representation of the memory layout of multi-dbts:
     *
     * |-----------------------------------------------------------
     * |    |    |    |                   |   |   |   |   |   |   |
     * | D1 | D2 | D3 |------->   <-------|D3 |D3 |D2 |D2 |D1 |D1 |
     * |    |    |    |                   | sz|off| sz|off| sz|off|
     * |-----------------------------------------------------------
     *
     * When they are key/data pairs. The keys are inserted first, followed by
     * the data.
     */
    protected boolean append_internal(
        final byte[] newdata, int offset, int len, int recno) 
        throws DatabaseException {
        int curr_off;

        // If this is the first item start inserting indeces at the end.
        if (pos == 0) {
            pos = ulen;
            curr_off = 0;
        } else {
            /* Read the offset and size from the last entry. */
            curr_off = DbUtil.array2int(this.data, pos + INT32SZ);
            curr_off += DbUtil.array2int(this.data, pos);
        }
        if (curr_off + len > pos - 2 * INT32SZ - (recno > 0 ? INT32SZ : 0))
            return (false);
        if (recno > 0) {
            pos -= INT32SZ;
            DbUtil.int2array(curr_off, this.data, pos);
        }
        pos -= INT32SZ;
        DbUtil.int2array(curr_off, this.data, pos);
        pos -= INT32SZ;
        DbUtil.int2array(len, this.data, pos);

        /* Add a terminator. */
        DbUtil.int2array((recno > 0) ? 0 : -1, this.data, pos - INT32SZ);

        for (int i = 0; i < len; i++)
            this.data[curr_off + i] = newdata[i + offset];
        return (true);
    }

    /*
     * A helper function for building DataEntry classes that contain multiple
     * entries.  Simplifies the common case without a record number.
     */
    protected boolean append_internal(
        final byte[] newdata, int offset, int len)
        throws DatabaseException {
        
        return append_internal(newdata, offset, len, 0);
    }
}

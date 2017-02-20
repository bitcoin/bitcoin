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
A DatabaseEntry that holds multiple key/data pairs returned by a single
{@link com.sleepycat.db.Database Database} or {@link com.sleepycat.db.Cursor Cursor} get call.
*/
public class MultipleKeyDataEntry extends MultipleEntry {
    /**
    Construct an entry with no data. The object must be configured
    before use with the {@link com.sleepycat.db.MultipleEntry#setUserBuffer MultipleEntry.setUserBuffer} method.
    */
    public MultipleKeyDataEntry() {
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
    public MultipleKeyDataEntry(final byte[] data) {
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
    public MultipleKeyDataEntry(final byte[] data,
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
    Get the next key/data pair in the returned set.  This method may only
    be called after a successful call to a {@link com.sleepycat.db.Database Database} or
    {@link com.sleepycat.db.Cursor Cursor} get method with this object as the data parameter.
    <p>
    @param key
    an entry that is set to refer to the next key element in the returned
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
   public boolean next(final DatabaseEntry key, final DatabaseEntry data) {
        if (pos == 0)
            pos = ulen - INT32SZ;

        final int keyoff = DbUtil.array2int(this.data, pos);

        // crack out the key and data offsets and lengths.
        if (keyoff < 0)
            return false;

        pos -= INT32SZ;
        final int keysz = DbUtil.array2int(this.data, pos);
        pos -= INT32SZ;
        final int dataoff = DbUtil.array2int(this.data, pos);
        pos -= INT32SZ;
        final int datasz = DbUtil.array2int(this.data, pos);
        pos -= INT32SZ;

        key.setData(this.data);
        key.setOffset(keyoff);
        key.setSize(keysz);

        data.setData(this.data);
        data.setOffset(dataoff);
        data.setSize(datasz);

        return true;
    }

    /**
    Append an entry to the bulk buffer.
    <p>
    @param key
    an array containing the key to be added.
    @param koff
    the position in the <b>key</b> array where the record starts.
    @param klen
    the length of the record, in bytes, to be copied from the <b>key</b> array.
    @param data
    an array containing the value to be added.
    @param doff
    the position in the <b>data</b> array where the record starts.
    @param dlen
    the length of the record, in bytes, to be copied from the <b>data</b> array.
    <p>
    @return
    indicates whether there was space.  A return of <code>false</code>
    indicates that the specified entry could not fit in the buffer.
    */
    public boolean append(final byte[] key, int koff, int klen,
                          final byte[] data, int doff, int dlen)
        throws DatabaseException {

        return append_internal(key, koff, klen) &&
            this.append_internal(data, doff, dlen);
    }

    /**
    Append an entry to the bulk buffer.
    <p>
    @param key
    the key to be appended, using the offset and size specified in the
    {@link com.sleepycat.db.DatabaseEntry DatabaseEntry}.
    @param data
    the value to be appended, using the offset and size specified in the
    {@link com.sleepycat.db.DatabaseEntry DatabaseEntry}.
    <p>
    @return
    indicates whether there was space.  A return of <code>false</code>
    indicates that the specified entry could not fit in the buffer.
    */
    public boolean append(final DatabaseEntry key, final DatabaseEntry data)
        throws DatabaseException {
        
        return append(key.data, key.offset, key.size,
            data.data, data.offset, data.size);
    }

    /**
    Append an entry to the bulk buffer.
    <p>
    @param key
    an array containing the key to be added.
    @param data
    an array containing the value to be added.
    <p>
    @return
    indicates whether there was space.  A return of <code>false</code>
    indicates that the specified entry could not fit in the buffer.
    */
    public boolean append(final byte[] key, final byte[] data)
        throws DatabaseException {
        return append(key, 0, key.length, data, 0, data.length);
    }
}


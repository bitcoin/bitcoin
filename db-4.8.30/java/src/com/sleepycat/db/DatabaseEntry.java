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
import java.lang.IllegalArgumentException;

/**
Encodes database key and data items as a byte array.
<p>
Storage and retrieval for the {@link com.sleepycat.db.Database Database} and {@link com.sleepycat.db.Cursor Cursor} methods
are based on key/data pairs. Both key and data items are represented by
DatabaseEntry objects.  Key and data byte arrays may refer to arrays of zero
length up to arrays of essentially unlimited length.
<p>
The DatabaseEntry class provides simple access to an underlying object whose
elements can be examined or changed.  DatabaseEntry objects can be
subclassed, providing a way to associate with it additional data or
references to other structures.
<p>
Access to DatabaseEntry objects is not re-entrant. In particular, if
multiple threads simultaneously access the same DatabaseEntry object using
{@link com.sleepycat.db.Database Database} or {@link com.sleepycat.db.Cursor Cursor} methods, the results are undefined.
<p>
DatabaseEntry objects may be used in conjunction with the object mapping
support provided in the {@link com.sleepycat.bind} package.
<p>
<h3>Input and Output Parameters</h3>
<p>
DatabaseEntry objects are used for both input data (when writing to a
database or specifying a search parameter) and output data (when reading
from a database).  For certain methods, one parameter may be an input
parameter and another may be an output parameter.  For example, the
{@link Database#get} method has an input key parameter and an output
data parameter.  The documentation for each method describes whether its
parameters are input or output parameters.
<p>
For DatabaseEntry input parameters, the caller is responsible for
initializing the data array of the DatabaseEntry.  For DatabaseEntry
output parameters, the method called will initialize the data array.
<p>
For DatabaseEntry output parameters, by default the method called will
reuse the byte array in the DatabaseEntry, if the data returned fits in
the byte array.  This behavior can be configured with {@link
#setReuseBuffer} or {@link #setUserBuffer}. If an entry is configured to
reuse the byte array (the default behavior), the length of the underlying
byte array should not be used to determine the amount of data returned each
time the entry is used as an output parameter, rather the {@link #getSize}
call should be used. If an entry is configured to not reuse the byte array,
a new array is allocated each time the entry is used as an output parameter,
 so
the application can safely keep a reference to the byte array returned
by {@link #getData} without danger that the array will be overwritten in
a subsequent call.
<p>
<h3>Offset and Size Properties</h3>
<p>
By default the Offset property is zero and the Size property is the length
of the byte array.  However, to allow for optimizations involving the
partial use of a byte array, the Offset and Size may be set to non-default
values.
<p>
For DatabaseEntry output parameters, the Size will always be set to the
length of the returned data and
the Offset will always be set to zero.
<p>
However, for DatabaseEntry input parameters the Offset and Size are set to
non-default values by the built-in tuple and serial bindings.  For example,
with a tuple or serial binding the byte array is grown dynamically as data
is output, and the Size is set to the number of bytes actually used.  For a
serial binding, the Offset is set to a non-zero value in order to implement
an optimization having to do with the serialization stream header.
<p>
Therefore, for output DatabaseEntry parameters the application can assume
that the Offset is zero and the Size is the length of the byte array.
However, for input DatabaseEntry parameters the application should not make
this assumption.  In general, it is safest for the application to always
honor the Size and Offset properties, rather than assuming they have default
values.
<p>
<h3>Partial Offset and Length Properties</h3>
<p>
By default the specified data (byte array, offset and size) corresponds to
the full stored key or data item.  Optionally, the Partial property can be
set to true, and the PartialOffset and PartialLength properties are used to
specify the portion of the key or data item to be read or written.  For
details, see the {@link #setPartial(int,int,boolean)} method.
<p>
Note that the Partial properties are set only by the caller.  They will
never be set by a Database or Cursor method, nor will they every be set by
bindings.  Therefore, the application can assume that the Partial properties
are not set, unless the application itself sets them explicitly.
*/
public class DatabaseEntry {

    /* Currently, JE stores all data records as byte array */
    /* package */ byte[] data;
    /* package */ ByteBuffer data_nio;
    /* package */ int dlen = 0;
    /* package */ int doff = 0;
    /* package */ int flags = 0;
    /* package */ int offset = 0;
    /* package */ int size = 0;
    /* package */ int ulen = 0;

    /*
     * IGNORE is used to avoid returning data that is not needed.  It may not
     * be used as the key DBT in a put since the PARTIAL flag is not allowed;
     * use UNUSED for that instead.
     */

    /* package */
    static final DatabaseEntry IGNORE = new DatabaseEntry();
    static {
        IGNORE.setUserBuffer(0, true);
        IGNORE.setPartial(0, 0, true); // dlen == 0, so no data ever returned
    }
    /* package */
    static final DatabaseEntry UNUSED = new DatabaseEntry();

    /* package */ static final int INT32SZ = 4;

    /*
     * Constructors
     */

    /**
    Construct a DatabaseEntry with null data. The offset and size are set to
    zero.
    */
    public DatabaseEntry() {
    }

    /**
    Construct a DatabaseEntry with a given byte array.  The offset is
    set to zero; the size is set to the length of the array, or to zero if
    null is passed.
    <p>
    @param data
    Byte array wrapped by the DatabaseEntry.
    */
    public DatabaseEntry(final byte[] data) {
        this.data = data;
        if (data != null) {
            this.size = data.length;
        }
        this.data_nio = null;
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
    public DatabaseEntry(final byte[] data, final int offset, final int size) {
        this.data = data;
        this.offset = offset;
        this.size = size;
        this.data_nio = null;
    }

    /**
    Construct a DatabaseEntry with a given native I/O buffer.  If the buffer is
    non-direct, the buffer's backing array is used.  If the buffer is direct,
    the DatabaseEntry is configured with an application-owned buffer whose
    length is set to the ByteBuffer's capacity less its position.  The
    DatabaseEntry's size is set to the ByteBuffer's limit less its position and
    the offset is set to buffer's position (adjusted by arrayOffset if the
    buffer is non-direct.)
    <p>
    @param data
    NIO byte buffer wrapped by the DatabaseEntry.
    */
    public DatabaseEntry(ByteBuffer data) {
        if (data == null) {
            this.data = null;
            this.data_nio = null;
        } else if (data.isDirect()) {
            this.data = null;
            this.data_nio = data;
            this.offset = data.position();
            this.size = data.limit() - data.position();
            setUserBuffer(data.capacity() - data.position(), true);
        } else if (data.hasArray()) {
            /* The same as calling the DatabaseEntry(byte[]) constructor. */
            this.data = data.array();
            this.offset = data.arrayOffset() + data.position();
            this.size = data.limit() - data.position();
            this.data_nio = null;
        } else {
            throw new IllegalArgumentException("Attempting to use a " + 
                "non-direct ByteBuffer without a backing byte array.");
        }
    }

    /*
     * Accessors
     */

    /**
    Return the byte array.
    <p>
    For a DatabaseEntry that is used as an output parameter, the byte
    array will always be a newly allocated array.  The byte array specified
    by the caller will not be used and may be null.
    <p>
    @return
    The byte array.
    */
    public byte[] getData() {
        return data;
    }

    /**
    Return the java.nio.ByteBuffer.
    <p>
    Used to access the underlying data when the DatabaseEntry is
    configured to utilize a java.nio.ByteBuffer.
    <p>
    @return
    The underlying java.nio.ByteBuffer.
    */
    public ByteBuffer getDataNIO() {
        return data_nio;
    }

    /**
    Sets the byte array, offset and size.
    <p>
    @param data
    Byte array wrapped by the DatabaseEntry.
    @param offset
    Offset in the first byte in the byte array to be included.
    @param size
    Number of bytes in the byte array to be included.
    */
    public void setData(final byte[] data, final int offset, final int size) {
        this.data = data;
        this.offset = offset;
        this.size = size;

        this.data_nio = null;
    }

    /**
    Sets the byte array.  The offset is set to zero; the size is set to the
    length of the array, or to zero if null is passed.
    <p>
    @param data
    Byte array wrapped by the DatabaseEntry.
    */
    public void setData(final byte[] data) {
        setData(data, 0, (data == null) ? 0 : data.length);
    }

    /**
    *
    Sets the java.nio.ByteBuffer (or the backing array if passed a non-direct
    ByteBuffer,) offset and size.  If passed a direct ByteBuffer, the entry is
    configured with an application-owned buffer whose length is set to the 
    ByteBuffer's capacity less the offset.
    <p>
    @param data
    java.nio.ByteBuffer wrapped by the DatabaseEntry.
    @param offset
    int offset into the ByteBuffer where the DatabaseEntry data begins.
    @param size
    int size of the ByteBuffer available.
    */
    public void setDataNIO(final ByteBuffer data, final int offset, final int size) {
        if (data == null) {
            data_nio = null;
            this.data = null;
            this.offset = 0;
            this.size = 0;
            flags = 0;
        } else if (data.hasArray()) {
            setData(data.array(), offset + data.arrayOffset(), size);
        } else {
            data_nio = data;
            this.offset = offset;
            this.size = size;
            flags = 0;
            setUserBuffer(data.capacity() - offset, true);

            this.data = null;
        }
    }
    
    /**
    *
    Sets the java.nio.ByteBuffer.  The offset is set to the ByteBuffer's
    position; the size is set to the ByteBuffer's limit less its position, or to
    zero if null is passed.  If the ByteBuffer is non-direct, the backing array
    will be used.  If passed a direct ByteBuffer, the entry is configured with
    an application-owned buffer whose length is set to the ByteBuffer's capacity
    less its current position.  No call to {@link #setUserBuffer} is required
    after setting the ByteBuffer.
    <p>
    @param data
    java.nio.ByteBuffer wrapped by the DatabaseEntry.
    */
    public void setDataNIO(final ByteBuffer data) {
        if (data == null)
            setDataNIO(null, 0, 0);
        else
            setDataNIO(data, data.position(), data.limit() - data.position());
    }

    /**
     * This method is called just before performing a get operation.  It is
     * overridden by Multiple*Entry classes to return the flags used for bulk
     * retrieval.  If non-zero is returned, this method should reset the entry
     * position so that the next set of key/data can be returned.
     */
    /* package */
    int getMultiFlag() {
        return 0;
    }

    /**
    Return the byte offset into the data array.
    <p>
    For a DatabaseEntry that is used as an output parameter, the offset
    will always be zero.
    <p>
    @return
    Offset in the first byte in the byte array to be included.
    */
    public int getOffset() {
        return offset;
    }

    /**
    Set the byte offset into the data array.
    <p>
    @param offset
    Offset in the first byte in the byte array to be included.
    */
    public void setOffset(final int offset) {
        this.offset = offset;
    }

    /**
    Return the byte length of the partial record being read or written by
    the application, in bytes.
    <p>
    Note that the Partial properties are set only by the caller.  They
    will never be set by a Database or Cursor method.
    <p>
    @return
    The byte length of the partial record being read or written by the
    application, in bytes.
    <p>
    @see #setPartial(int,int,boolean)
    */
    public int getPartialLength() {
        return dlen;
    }

    /**
    Return the offset of the partial record being read or written by the
    application, in bytes.
    <p>
    Note that the Partial properties are set only by the caller.  They
    will never be set by a Database or Cursor method.
    <p>
    @return
    The offset of the partial record being read or written by the
    application, in bytes.
    <p>
    @see #setPartial(int,int,boolean)
    */
    public int getPartialOffset() {
        return doff;
    }

    /**
    Return whether this DatabaseEntry is configured to read or write partial
    records.
    <p>
    Note that the Partial properties are set only by the caller.  They
    will never be set by a Database or Cursor method.
    <p>
    @return
    Whether this DatabaseEntry is configured to read or write partial
    records.
    <p>
    @see #setPartial(int,int,boolean)
    */
    public boolean getPartial() {
        return (flags & DbConstants.DB_DBT_PARTIAL) != 0;
    }

    /**
    Set the offset of the partial record being read or written by the
    application, in bytes.
    <p>
    Note that the Partial properties are set only by the caller.  They
    will never be set by a Database or Cursor method.
    <p>
    @param doff
    The offset of the partial record being read or written by the
    application, in bytes.
    <p>
    @see #setPartial(int,int,boolean)
    */
    public void setPartialOffset(final int doff) {
        this.doff = doff;
    }

    /**
    Set the byte length of the partial record being read or written by
    the application, in bytes.
    <p>
    Note that the Partial properties are set only by the caller.  They
    will never be set by a Database or Cursor method.
    <p>
    @param dlen
    The byte length of the partial record being read or written by the
    <p>
    @see #setPartial(int,int,boolean)
    application, in bytes.
    */
    public void setPartialLength(final int dlen) {
        this.dlen = dlen;
    }

    /**
    Configure this DatabaseEntry to read or write partial records.
    <p>
    Note that the Partial properties are set only by the caller.  They
    will never be set by a Database or Cursor method.
    <p>
    @param partial
    Whether this DatabaseEntry is configured to read or write partial
    records.
    <p>
    @see #setPartial(int,int,boolean)
    */
    public void setPartial(final boolean partial) {
        if (partial)
            flags |= DbConstants.DB_DBT_PARTIAL;
        else
            flags &= ~DbConstants.DB_DBT_PARTIAL;
    }

    /**
    Configures this DatabaseEntry to read or write partial records.
    <p>
    Do partial retrieval or storage of an item.  If the calling
    application is doing a retrieval, length bytes specified by
    <tt>dlen</tt>, starting at the offset set by <tt>doff</tt> bytes from
    the beginning of the retrieved data record are returned as if they
    comprised the entire record.  If any or all of the specified bytes do
    not exist in the record, the get is successful, and any existing bytes
    are returned.
    <p>
    For example, if the data portion of a retrieved record was 100 bytes,
    and a partial retrieval was done using a DatabaseEntry having a partial
    length of 20 and a partial offset of 85, the retrieval would succeed and
    the retrieved data would be the last 15 bytes of the record.
    <p>
    If the calling application is storing an item, length bytes specified
    by <tt>dlen</tt>, starting at the offset set by <tt>doff</tt>
    bytes from the beginning of the specified key's data item are replaced
    by the data specified by the DatabaseEntry.  If the partial length is
    smaller than the data, the record will grow; if the partial length is
    larger than the data, the record will shrink.  If the specified bytes do
    not exist, the record will be extended using nul bytes as necessary, and
    the store will succeed.
    <p>
    It is an error to specify a partial key when performing a put
    operation of any kind.
    <p>
    It is an error to attempt a partial store using the {@link com.sleepycat.db.Database#put Database.put} method in a database that supports duplicate records. Partial
    stores in databases supporting duplicate records must be done using a
    cursor method.
    <p>
    Note that the Partial properties are set only by the caller.  They
    will never be set by a Database or Cursor method.
    <p>
    @param doff
    The offset of the partial record being read or written by the
    application, in bytes.
    <p>
    @param dlen
    The byte length of the partial record being read or written by the
    application, in bytes.
    <p>
    @param partial
    Whether this DatabaseEntry is configured to read or write partial
    records.
    */
    public void setPartial(final int doff,
                           final int dlen,
                           final boolean partial) {
        setPartialOffset(doff);
        setPartialLength(dlen);
        setPartial(partial);
    }

    /**
Return the record number encoded in this entry's buffer.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The record number encoded in this entry's buffer.
     *
    @return the decoded record number.
    */
    public int getRecordNumber() {
        return DbUtil.array2int(data, offset);
    }

    /**
    Initialize the entry from a logical record number.  Record numbers
    are integer keys starting at 1.  When this method is called the data,
    size and offset fields are implicitly set to hold a byte array
    representation of the integer key.
     *
    @param recno the record number to be encoded
    */
    public void setRecordNumber(final int recno) {
        if (data == null || data.length < INT32SZ) {
            data = new byte[INT32SZ];
            size = INT32SZ;
            ulen = 0;
            offset = 0;
        }
        DbUtil.int2array(recno, data, 0);
    }

    /**
Return true if the whether the entry is configured to reuse the buffer.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the whether the entry is configured to reuse the buffer.
    */
    public boolean getReuseBuffer() {
        return 0 ==
            (flags & (DbConstants.DB_DBT_MALLOC | DbConstants.DB_DBT_USERMEM));
    }

    /**
    Configures the entry to try to reuse the buffer before allocating a new
    one.
    <p>
    @param reuse
    whether to reuse the buffer
    */
    public void setReuseBuffer(boolean reuse) {
        if (data_nio != null)
            throw new IllegalArgumentException("Can only set the reuse flag on" +
                   " DatabaseEntry classes with a underlying byte[] data");

        if (reuse)
            flags &= ~(DbConstants.DB_DBT_MALLOC | DbConstants.DB_DBT_USERMEM);
        else {
            flags &= ~DbConstants.DB_DBT_USERMEM;
            flags |= DbConstants.DB_DBT_MALLOC;
        }
    }

    /**
    Return the byte size of the data array.
    <p>
    For a DatabaseEntry that is used as an output parameter, the size
    will always be the length of the data array.
    <p>
    @return
    Number of bytes in the byte array to be included.
    */
    public int getSize() {
        return size;
    }

    /**
    Set the byte size of the data array.
    <p>
    @param size
    Number of bytes in the byte array to be included.
    */
    public void setSize(final int size) {
        this.size = size;
    }

    /**
Return true if the whether the buffer in this entry is owned by the
    application.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the whether the buffer in this entry is owned by the
    application.
    */
    public boolean getUserBuffer() {
        return (flags & DbConstants.DB_DBT_USERMEM) != 0;
    }

    /**
Return the length of the application's buffer.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The length of the application's buffer.
    */
    public int getUserBufferLength() {
        return ulen;
    }

    /**
    Configures the entry with an application-owned buffer.
    <p>
    The <code>data</code> field of the entry must refer to a buffer that is
    at least <code>length</code> bytes in length.
    <p>
    If the length of the requested item is less than or equal to that number
    of bytes, the item is copied into the memory to which the
    <code>data</code> field refers.  Otherwise, the <code>size</code> field
    is set to the length needed for the requested item, and a
    {@link com.sleepycat.db.MemoryException MemoryException} is thrown.
    <p>
    Applications can determine the length of a record by setting
    <code>length</code> to 0 and calling {@link com.sleepycat.db.DatabaseEntry#getSize DatabaseEntry.getSize}
    on the return value.
    <p>
    @param length
    the length of the buffer
    <p>
    @param usermem
    whether the buffer is owned by the application
    */
    public void setUserBuffer(final int length, final boolean usermem) {
        this.ulen = length;
        if (usermem) {
            flags &= ~DbConstants.DB_DBT_MALLOC;
            flags |= DbConstants.DB_DBT_USERMEM;
        } else
            flags &= ~DbConstants.DB_DBT_USERMEM;
    }

    /**
     * Compares the data of two entries for byte-by-byte equality.
     *
     * <p>In either entry, if the offset is non-zero or the size is not equal
     * to the data array length, then only the data bounded by these values is
     * compared.  The data array length and offset need not be the same in both
     * entries for them to be considered equal.</p>
     *
     * <p>If the data array is null in one entry, then to be considered equal
     * both entries must have a null data array.</p>
     *
     * <p>If the partial property is set in either entry, then to be considered
     * equal both entries must have the same partial properties: partial,
     * partialOffset and partialLength.
     */
    public boolean equals(Object o) {
        if (!(o instanceof DatabaseEntry)) {
            return false;
        }
        DatabaseEntry e = (DatabaseEntry) o;
        if (getPartial() || e.getPartial()) {
            if (getPartial() != e.getPartial() ||
                dlen != e.dlen ||
                doff != e.doff) {
                return false;
            }
        }
        if (data == null && e.data == null) {
            return true;
        }
        if (data == null || e.data == null) {
            return false;
        }
        if (size != e.size) {
            return false;
        }
        for (int i = 0; i < size; i += 1) {
            if (data[offset + i] != e.data[e.offset + i]) {
                return false;
            }
        }
        return true;
    }

    /**
     * Returns a hash code based on the data value.
     */
    public int hashCode() {
        int hash = 0;
        if (data != null) {
            for (int i = 0; i < size; i += 1) {
                hash += data[offset + i];
            }
        }
        return hash;
    }
}

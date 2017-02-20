/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.bind.tuple;

import java.math.BigInteger;

import com.sleepycat.util.FastOutputStream;
import com.sleepycat.util.PackedInteger;
import com.sleepycat.util.UtfOps;

/**
 * An <code>OutputStream</code> with <code>DataOutput</code>-like methods for
 * writing tuple fields.  It is used by <code>TupleBinding</code>.
 *
 * <p>This class has many methods that have the same signatures as methods in
 * the {@link java.io.DataOutput} interface.  The reason this class does not
 * implement {@link java.io.DataOutput} is because it would break the interface
 * contract for those methods because of data format differences.</p>
 *
 * <p>Signed numbers are stored in the buffer in MSB (most significant byte
 * first) order with their sign bit (high-order bit) inverted to cause negative
 * numbers to be sorted first when comparing values as unsigned byte arrays,
 * as done in a database.  Unsigned numbers, including characters, are stored
 * in MSB order with no change to their sign bit.  BigInteger values are stored
 * with a preceding length having the same sign as the value.</p>
 *
 * <p>Strings and character arrays are stored either as a fixed length array of
 * unicode characters, where the length must be known by the application, or as
 * a null-terminated UTF byte array.</p>
 * <ul>
 * <li>Null strings are UTF encoded as { 0xFF }, which is not allowed in a
 * standard UTF encoding.  This allows null strings, as distinct from empty or
 * zero length strings, to be represented in a tuple.  Using the default
 * comparator, null strings will be ordered last.</li>
 * <li>Zero (0x0000) character values are UTF encoded as non-zero values, and
 * therefore embedded zeros in the string are supported.  The sequence { 0xC0,
 * 0x80 } is used to encode a zero character.  This UTF encoding is the same
 * one used by native Java UTF libraries.  However, this encoding of zero does
 * impact the lexicographical ordering, and zeros will not be sorted first (the
 * natural order) or last.  For all character values other than zero, the
 * default UTF byte ordering is the same as the Unicode lexicographical
 * character ordering.</li>
 * </ul>
 *
 * <p>Floats and doubles are stored using two different representations: sorted
 * representation and integer-bit (IEEE 754) representation.  If you use
 * negative floating point numbers in a key, you should use sorted
 * representation; alternatively you may use integer-bit representation but you
 * will need to implement and configure a custom comparator to get correct
 * numeric ordering for negative numbers.</p>
 *
 * <p>To use sorted representation use this set of methods:</p>
 * <ul>
 * <li>{@link TupleOutput#writeSortedFloat}</li>
 * <li>{@link TupleInput#readSortedFloat}</li>
 * <li>{@link TupleOutput#writeSortedDouble}</li>
 * <li>{@link TupleInput#readSortedDouble}</li>
 * </ul>
 *
 * <p>To use integer-bit representation use this set of methods:</p>
 * <ul>
 * <li>{@link TupleOutput#writeFloat}</li>
 * <li>{@link TupleInput#readFloat}</li>
 * <li>{@link TupleOutput#writeDouble}</li>
 * <li>{@link TupleInput#readDouble}</li>
 * </ul>
 *
 * @author Mark Hayes
 */
public class TupleOutput extends FastOutputStream {

    /**
     * We represent a null string as a single FF UTF character, which cannot
     * occur in a UTF encoded string.
     */
    static final int NULL_STRING_UTF_VALUE = ((byte) 0xFF);

    /**
     * Creates a tuple output object for writing a byte array of tuple data.
     */
    public TupleOutput() {

        super();
    }

    /**
     * Creates a tuple output object for writing a byte array of tuple data,
     * using a given buffer.  A new buffer will be allocated only if the number
     * of bytes needed is greater than the length of this buffer.  A reference
     * to the byte array will be kept by this object and therefore the byte
     * array should not be modified while this object is in use.
     *
     * @param buffer is the byte array to use as the buffer.
     */
    public TupleOutput(byte[] buffer) {

        super(buffer);
    }

    // --- begin DataOutput compatible methods ---

    /**
     * Writes the specified bytes to the buffer, converting each character to
     * an unsigned byte value.
     * Writes values that can be read using {@link TupleInput#readBytes}.
     * Only characters with values below 0x100 may be written using this
     * method, since the high-order 8 bits of all characters are discarded.
     *
     * @param val is the string containing the values to be written.
     *
     * @return this tuple output object.
     *
     * @throws NullPointerException if the val parameter is null.
     */
    public final TupleOutput writeBytes(String val) {

        writeBytes(val.toCharArray());
        return this;
    }

    /**
     * Writes the specified characters to the buffer, converting each character
     * to a two byte unsigned value.
     * Writes values that can be read using {@link TupleInput#readChars}.
     *
     * @param val is the string containing the characters to be written.
     *
     * @return this tuple output object.
     *
     * @throws NullPointerException if the val parameter is null.
     */
    public final TupleOutput writeChars(String val) {

        writeChars(val.toCharArray());
        return this;
    }

    /**
     * Writes the specified characters to the buffer, converting each character
     * to UTF format, and adding a null terminator byte.
     * Note that zero (0x0000) character values are encoded as non-zero values
     * and a null String parameter is encoded as 0xFF.
     * Writes values that can be read using {@link TupleInput#readString()}.
     *
     * @param val is the string containing the characters to be written.
     *
     * @return this tuple output object.
     */
    public final TupleOutput writeString(String val) {

        if (val != null) {
            writeString(val.toCharArray());
        } else {
            writeFast(NULL_STRING_UTF_VALUE);
        }
        writeFast(0);
        return this;
    }

    /**
     * Writes a char (two byte) unsigned value to the buffer.
     * Writes values that can be read using {@link TupleInput#readChar}.
     *
     * @param val is the value to write to the buffer.
     *
     * @return this tuple output object.
     */
    public final TupleOutput writeChar(int val) {

        writeFast((byte) (val >>> 8));
        writeFast((byte) val);
        return this;
    }

    /**
     * Writes a boolean (one byte) unsigned value to the buffer, writing one
     * if the value is true and zero if it is false.
     * Writes values that can be read using {@link TupleInput#readBoolean}.
     *
     * @param val is the value to write to the buffer.
     *
     * @return this tuple output object.
     */
    public final TupleOutput writeBoolean(boolean val) {

        writeFast(val ? (byte)1 : (byte)0);
        return this;
    }

    /**
     * Writes an signed byte (one byte) value to the buffer.
     * Writes values that can be read using {@link TupleInput#readByte}.
     *
     * @param val is the value to write to the buffer.
     *
     * @return this tuple output object.
     */
    public final TupleOutput writeByte(int val) {

        writeUnsignedByte(val ^ 0x80);
        return this;
    }

    /**
     * Writes an signed short (two byte) value to the buffer.
     * Writes values that can be read using {@link TupleInput#readShort}.
     *
     * @param val is the value to write to the buffer.
     *
     * @return this tuple output object.
     */
    public final TupleOutput writeShort(int val) {

        writeUnsignedShort(val ^ 0x8000);
        return this;
    }

    /**
     * Writes an signed int (four byte) value to the buffer.
     * Writes values that can be read using {@link TupleInput#readInt}.
     *
     * @param val is the value to write to the buffer.
     *
     * @return this tuple output object.
     */
    public final TupleOutput writeInt(int val) {

        writeUnsignedInt(val ^ 0x80000000);
        return this;
    }

    /**
     * Writes an signed long (eight byte) value to the buffer.
     * Writes values that can be read using {@link TupleInput#readLong}.
     *
     * @param val is the value to write to the buffer.
     *
     * @return this tuple output object.
     */
    public final TupleOutput writeLong(long val) {

        writeUnsignedLong(val ^ 0x8000000000000000L);
        return this;
    }

    /**
     * Writes an signed float (four byte) value to the buffer.
     * Writes values that can be read using {@link TupleInput#readFloat}.
     * <code>Float.floatToIntBits</code> is used to convert the signed float
     * value.
     *
     * <p><em>Note:</em> This method produces byte array values that by default
     * (without a custom comparator) do <em>not</em> sort correctly for
     * negative values.  Only non-negative values are sorted correctly by
     * default.  To sort all values correctly by default, use {@link
     * #writeSortedFloat}.</p>
     *
     * @param val is the value to write to the buffer.
     *
     * @return this tuple output object.
     */
    public final TupleOutput writeFloat(float val) {

        writeUnsignedInt(Float.floatToIntBits(val));
        return this;
    }

    /**
     * Writes an signed double (eight byte) value to the buffer.
     * Writes values that can be read using {@link TupleInput#readDouble}.
     * <code>Double.doubleToLongBits</code> is used to convert the signed
     * double value.
     *
     * <p><em>Note:</em> This method produces byte array values that by default
     * (without a custom comparator) do <em>not</em> sort correctly for
     * negative values.  Only non-negative values are sorted correctly by
     * default.  To sort all values correctly by default, use {@link
     * #writeSortedDouble}.</p>
     *
     * @param val is the value to write to the buffer.
     *
     * @return this tuple output object.
     */
    public final TupleOutput writeDouble(double val) {

        writeUnsignedLong(Double.doubleToLongBits(val));
        return this;
    }

    /**
     * Writes a signed float (four byte) value to the buffer, with support for
     * correct default sorting of all values.
     * Writes values that can be read using {@link TupleInput#readSortedFloat}.
     *
     * <p><code>Float.floatToIntBits</code> and the following bit manipulations
     * are used to convert the signed float value to a representation that is
     * sorted correctly by default.</p>
     * <pre>
     *  int intVal = Float.floatToIntBits(val);
     *  intVal ^= (intVal &lt; 0) ? 0xffffffff : 0x80000000;
     * </pre>
     *
     * @param val is the value to write to the buffer.
     *
     * @return this tuple output object.
     */
    public final TupleOutput writeSortedFloat(float val) {

        int intVal = Float.floatToIntBits(val);
        intVal ^= (intVal < 0) ? 0xffffffff : 0x80000000;
        writeUnsignedInt(intVal);
        return this;
    }

    /**
     * Writes a signed double (eight byte) value to the buffer, with support
     * for correct default sorting of all values.
     * Writes values that can be read using {@link TupleInput#readSortedDouble}.
     *
     * <p><code>Float.doubleToLongBits</code> and the following bit
     * manipulations are used to convert the signed double value to a
     * representation that is sorted correctly by default.</p>
     * <pre>
     *  long longVal = Double.doubleToLongBits(val);
     *  longVal ^= (longVal &lt; 0) ? 0xffffffffffffffffL : 0x8000000000000000L;
     * </pre>
     *
     * @param val is the value to write to the buffer.
     *
     * @return this tuple output object.
     */
    public final TupleOutput writeSortedDouble(double val) {

        long longVal = Double.doubleToLongBits(val);
        longVal ^= (longVal < 0) ? 0xffffffffffffffffL : 0x8000000000000000L;
        writeUnsignedLong(longVal);
        return this;
    }

    // --- end DataOutput compatible methods ---

    /**
     * Writes the specified bytes to the buffer, converting each character to
     * an unsigned byte value.
     * Writes values that can be read using {@link TupleInput#readBytes}.
     * Only characters with values below 0x100 may be written using this
     * method, since the high-order 8 bits of all characters are discarded.
     *
     * @param chars is the array of values to be written.
     *
     * @return this tuple output object.
     *
     * @throws NullPointerException if the chars parameter is null.
     */
    public final TupleOutput writeBytes(char[] chars) {

        for (int i = 0; i < chars.length; i++) {
            writeFast((byte) chars[i]);
        }
        return this;
    }

    /**
     * Writes the specified characters to the buffer, converting each character
     * to a two byte unsigned value.
     * Writes values that can be read using {@link TupleInput#readChars}.
     *
     * @param chars is the array of characters to be written.
     *
     * @return this tuple output object.
     *
     * @throws NullPointerException if the chars parameter is null.
     */
    public final TupleOutput writeChars(char[] chars) {

        for (int i = 0; i < chars.length; i++) {
            writeFast((byte) (chars[i] >>> 8));
            writeFast((byte) chars[i]);
        }
        return this;
    }

    /**
     * Writes the specified characters to the buffer, converting each character
     * to UTF format.
     * Note that zero (0x0000) character values are encoded as non-zero values.
     * Writes values that can be read using {@link TupleInput#readString(int)}
     * or {@link TupleInput#readString(char[])}.
     *
     * @param chars is the array of characters to be written.
     *
     * @return this tuple output object.
     *
     * @throws NullPointerException if the chars parameter is null.
     */
    public final TupleOutput writeString(char[] chars) {

        if (chars.length == 0) return this;

        int utfLength = UtfOps.getByteLength(chars);

        makeSpace(utfLength);
        UtfOps.charsToBytes(chars, 0, getBufferBytes(), getBufferLength(),
                            chars.length);
        addSize(utfLength);
        return this;
    }

    /**
     * Writes an unsigned byte (one byte) value to the buffer.
     * Writes values that can be read using {@link
     * TupleInput#readUnsignedByte}.
     *
     * @param val is the value to write to the buffer.
     *
     * @return this tuple output object.
     */
    public final TupleOutput writeUnsignedByte(int val) {

        writeFast(val);
        return this;
    }

    /**
     * Writes an unsigned short (two byte) value to the buffer.
     * Writes values that can be read using {@link
     * TupleInput#readUnsignedShort}.
     *
     * @param val is the value to write to the buffer.
     *
     * @return this tuple output object.
     */
    public final TupleOutput writeUnsignedShort(int val) {

        writeFast((byte) (val >>> 8));
        writeFast((byte) val);
        return this;
    }

    /**
     * Writes an unsigned int (four byte) value to the buffer.
     * Writes values that can be read using {@link
     * TupleInput#readUnsignedInt}.
     *
     * @param val is the value to write to the buffer.
     *
     * @return this tuple output object.
     */
    public final TupleOutput writeUnsignedInt(long val) {

        writeFast((byte) (val >>> 24));
        writeFast((byte) (val >>> 16));
        writeFast((byte) (val >>> 8));
        writeFast((byte) val);
        return this;
    }

    /**
     * This method is private since an unsigned long cannot be treated as
     * such in Java, nor converted to a BigInteger of the same value.
     */
    private final TupleOutput writeUnsignedLong(long val) {

        writeFast((byte) (val >>> 56));
        writeFast((byte) (val >>> 48));
        writeFast((byte) (val >>> 40));
        writeFast((byte) (val >>> 32));
        writeFast((byte) (val >>> 24));
        writeFast((byte) (val >>> 16));
        writeFast((byte) (val >>> 8));
        writeFast((byte) val);
        return this;
    }

    /**
     * Writes a packed integer.  Note that packed integers are not appropriate
     * for sorted values (keys) unless a custom comparator is used.
     *
     * @see PackedInteger
     */
    public final void writePackedInt(int val) {

        makeSpace(PackedInteger.MAX_LENGTH);

        int oldLen = getBufferLength();
        int newLen = PackedInteger.writeInt(getBufferBytes(), oldLen, val);

        addSize(newLen - oldLen);
    }

    /**
     * Writes a packed long integer.  Note that packed integers are not
     * appropriate for sorted values (keys) unless a custom comparator is used.
     *
     * @see PackedInteger
     */
    public final void writePackedLong(long val) {

        makeSpace(PackedInteger.MAX_LONG_LENGTH);

        int oldLen = getBufferLength();
        int newLen = PackedInteger.writeLong(getBufferBytes(), oldLen, val);

        addSize(newLen - oldLen);
    }

    /**
     * Writes a {@code BigInteger}.  Supported {@code BigInteger} values are
     * limited to those with a byte array ({@link BigInteger#toByteArray})
     * representation with a size of 0x7fff bytes or less.  The maximum {@code
     * BigInteger} value is (2<sup>0x3fff7</sup> - 1) and the minimum value is
     * (-2<sup>0x3fff7</sup>).
     *
     * <p>The byte format for a {@code BigInteger} value is:</p>
     * <ul>
     * <li>Byte 0 and 1: The length of the following bytes, negated if the
     * {@code BigInteger} value is negative, and written as a sorted value as
     * if {@link #writeShort} were called.</li>
     * <li>Byte 2: The first byte of the {@link BigInteger#toByteArray} array,
     * written as a sorted value as if {@link #writeByte} were called.</li>
     * <li>Byte 3 to N: The second and remaining bytes, if any, of the {@link
     * BigInteger#toByteArray} array, written without modification.</li>
     * </ul>
     * <p>This format provides correct default sorting when the default
     * byte-by-byte comparison is used.</p>
     *
     * @throws NullPointerException if val is null.
     *
     * @throws IllegalArgumentException if the byte array representation of val
     * is larger than 0x7fff bytes.
     */
    public final TupleOutput writeBigInteger(BigInteger val) {
        byte[] a = val.toByteArray();
        if (a.length > Short.MAX_VALUE) {
            throw new IllegalArgumentException
                ("BigInteger byte array is larger than 0x7fff bytes");
        }
        int firstByte = a[0];
        writeShort((firstByte < 0) ? (- a.length) : a.length);
        writeByte(firstByte);
        writeFast(a, 1, a.length - 1);
        return this;
    }

    /**
     * Returns the byte length of a given {@code BigInteger} value.
     *
     * @see TupleOutput#writeBigInteger
     */
    public static int getBigIntegerByteLength(BigInteger val) {
        return 2 /* length bytes */ +
               (val.bitLength() + 1 /* sign bit */ + 7 /* round up */) / 8;
    }
}

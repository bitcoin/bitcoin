/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.util;

/**
 * Static methods for reading and writing packed integers.
 *
 * <p>Note that packed integers are not sorted naturally for a byte-by-byte
 * comparison because they have a preceding length and are little endian;
 * therefore, they are typically not used for keys.  However, it so happens
 * that packed integers in the range {@code 0} to {@code 630} (inclusive) are
 * sorted correctly in a byte-by-byte comparison, and this may be useful for
 * some applications.</p>
 *
 * <p>Values in the inclusive range [-119,119] are stored in a single byte.
 * For values outside that range, the first byte stores the sign and the number
 * of additional bytes.  The additional bytes store (abs(value) - 119) as an
 * unsigned little endian integer.</p>
 *
 * <p>To read and write packed integer values, call {@link #readInt} and {@link
 * #writeInt} or for long values {@link #readLong} and {@link #writeLong}.  To
 * get the length of a packed integer without reading it, call {@link
 * #getReadIntLength} or {@link #getReadLongLength}.  To get the length of an
 * unpacked integer without writing it, call {@link #getWriteIntLength} or
 * {@link #getWriteLongLength}.</p>
 *
 * <p>Because the same packed format is used for int and long values, stored
 * int values may be expanded to long values without introducing a format
 * incompatibility.  You can treat previously stored packed int values as long
 * values by calling {@link #readLong} and {@link #getReadLongLength}.</p>
 *
 * @author Mark Hayes
 */
public class PackedInteger {

    /**
     * The maximum number of bytes needed to store an int value (5).
     */
    public static final int MAX_LENGTH = 5;

    /**
     * The maximum number of bytes needed to store a long value (9).
     */
    public static final int MAX_LONG_LENGTH = 9;

    /**
     * Reads a packed integer at the given buffer offset and returns it.
     *
     * @param buf the buffer to read from.
     *
     * @param off the offset in the buffer at which to start reading.
     *
     * @return the integer that was read.
     */
    public static int readInt(byte[] buf, int off) {

        boolean negative;
        int byteLen;

        int b1 = buf[off++];
        if (b1 < -119) {
            negative = true;
            byteLen = -b1 - 119;
        } else if (b1 > 119) {
            negative = false;
            byteLen = b1 - 119;
        } else {
            return b1;
        }

        int value = buf[off++] & 0xFF;
        if (byteLen > 1) {
            value |= (buf[off++] & 0xFF) << 8;
            if (byteLen > 2) {
                value |= (buf[off++] & 0xFF) << 16;
                if (byteLen > 3) {
                    value |= (buf[off++] & 0xFF) << 24;
                }
            }
        }

        return negative ? (-value - 119) : (value + 119);
    }

    /**
     * Reads a packed long integer at the given buffer offset and returns it.
     *
     * @param buf the buffer to read from.
     *
     * @param off the offset in the buffer at which to start reading.
     *
     * @return the long integer that was read.
     */
    public static long readLong(byte[] buf, int off) {

        boolean negative;
        int byteLen;

        int b1 = buf[off++];
        if (b1 < -119) {
            negative = true;
            byteLen = -b1 - 119;
        } else if (b1 > 119) {
            negative = false;
            byteLen = b1 - 119;
        } else {
            return b1;
        }

        long value = buf[off++] & 0xFFL;
        if (byteLen > 1) {
            value |= (buf[off++] & 0xFFL) << 8;
            if (byteLen > 2) {
                value |= (buf[off++] & 0xFFL) << 16;
                if (byteLen > 3) {
                    value |= (buf[off++] & 0xFFL) << 24;
                    if (byteLen > 4) {
                        value |= (buf[off++] & 0xFFL) << 32;
                        if (byteLen > 5) {
                            value |= (buf[off++] & 0xFFL) << 40;
                            if (byteLen > 6) {
                                value |= (buf[off++] & 0xFFL) << 48;
                                if (byteLen > 7) {
                                    value |= (buf[off++] & 0xFFL) << 56;
                                }
                            }
                        }
                    }
                }
            }
        }

        return negative ? (-value - 119) : (value + 119);
    }

    /**
     * Returns the number of bytes that would be read by {@link #readInt}.
     *
     * <p>Because the length is stored in the first byte, this method may be
     * called with only the first byte of the packed integer in the given
     * buffer.  This method only accesses one byte at the given offset.</p>
     *
     * @param buf the buffer to read from.
     *
     * @param off the offset in the buffer at which to start reading.
     *
     * @return the number of bytes that would be read.
     */
    public static int getReadIntLength(byte[] buf, int off) {

        int b1 = buf[off];
        if (b1 < -119) {
            return -b1 - 119 + 1;
        } else if (b1 > 119) {
            return b1 - 119 + 1;
        } else {
            return 1;
        }
    }

    /**
     * Returns the number of bytes that would be read by {@link #readLong}.
     *
     * <p>Because the length is stored in the first byte, this method may be
     * called with only the first byte of the packed integer in the given
     * buffer.  This method only accesses one byte at the given offset.</p>
     *
     * @param buf the buffer to read from.
     *
     * @param off the offset in the buffer at which to start reading.
     *
     * @return the number of bytes that would be read.
     */
    public static int getReadLongLength(byte[] buf, int off) {

        /* The length is stored in the same way for int and long. */
        return getReadIntLength(buf, off);
    }

    /**
     * Writes a packed integer starting at the given buffer offset and returns
     * the next offset to be written.
     *
     * @param buf the buffer to write to.
     *
     * @param offset the offset in the buffer at which to start writing.
     *
     * @param value the integer to be written.
     *
     * @return the offset past the bytes written.
     */
    public static int writeInt(byte[] buf, int offset, int value) {

        int byte1Off = offset;
        boolean negative;

        if (value < -119) {
            negative = true;
            value = -value - 119;
        } else if (value > 119) {
            negative = false;
            value = value - 119;
        } else {
            buf[offset++] = (byte) value;
            return offset;
        }
        offset++;

        buf[offset++] = (byte) value;
        if ((value & 0xFFFFFF00) == 0) {
            buf[byte1Off] = negative ? (byte) -120 : (byte) 120;
            return offset;
        }

        buf[offset++] = (byte) (value >>> 8);
        if ((value & 0xFFFF0000) == 0) {
            buf[byte1Off] = negative ? (byte) -121 : (byte) 121;
            return offset;
        }

        buf[offset++] = (byte) (value >>> 16);
        if ((value & 0xFF000000) == 0) {
            buf[byte1Off] = negative ? (byte) -122 : (byte) 122;
            return offset;
        }

        buf[offset++] = (byte) (value >>> 24);
        buf[byte1Off] = negative ? (byte) -123 : (byte) 123;
        return offset;
    }

    /**
     * Writes a packed long integer starting at the given buffer offset and
     * returns the next offset to be written.
     *
     * @param buf the buffer to write to.
     *
     * @param offset the offset in the buffer at which to start writing.
     *
     * @param value the long integer to be written.
     *
     * @return the offset past the bytes written.
     */
    public static int writeLong(byte[] buf, int offset, long value) {

        int byte1Off = offset;
        boolean negative;

        if (value < -119) {
            negative = true;
            value = -value - 119;
        } else if (value > 119) {
            negative = false;
            value = value - 119;
        } else {
            buf[offset++] = (byte) value;
            return offset;
        }
        offset++;

        buf[offset++] = (byte) value;
        if ((value & 0xFFFFFFFFFFFFFF00L) == 0) {
            buf[byte1Off] = negative ? (byte) -120 : (byte) 120;
            return offset;
        }

        buf[offset++] = (byte) (value >>> 8);
        if ((value & 0xFFFFFFFFFFFF0000L) == 0) {
            buf[byte1Off] = negative ? (byte) -121 : (byte) 121;
            return offset;
        }

        buf[offset++] = (byte) (value >>> 16);
        if ((value & 0xFFFFFFFFFF000000L) == 0) {
            buf[byte1Off] = negative ? (byte) -122 : (byte) 122;
            return offset;
        }

        buf[offset++] = (byte) (value >>> 24);
        if ((value & 0xFFFFFFFF00000000L) == 0) {
            buf[byte1Off] = negative ? (byte) -123 : (byte) 123;
            return offset;
        }

        buf[offset++] = (byte) (value >>> 32);
        if ((value & 0xFFFFFF0000000000L) == 0) {
            buf[byte1Off] = negative ? (byte) -124 : (byte) 124;
            return offset;
        }

        buf[offset++] = (byte) (value >>> 40);
        if ((value & 0xFFFF000000000000L) == 0) {
            buf[byte1Off] = negative ? (byte) -125 : (byte) 125;
            return offset;
        }

        buf[offset++] = (byte) (value >>> 48);
        if ((value & 0xFF00000000000000L) == 0) {
            buf[byte1Off] = negative ? (byte) -126 : (byte) 126;
            return offset;
        }

        buf[offset++] = (byte) (value >>> 56);
        buf[byte1Off] = negative ? (byte) -127 : (byte) 127;
        return offset;
    }

    /**
     * Returns the number of bytes that would be written by {@link #writeInt}.
     *
     * @param value the integer to be written.
     *
     * @return the number of bytes that would be used to write the given
     * integer.
     */
    public static int getWriteIntLength(int value) {

        if (value < -119) {
            value = -value - 119;
        } else if (value > 119) {
            value = value - 119;
        } else {
            return 1;
        }

        if ((value & 0xFFFFFF00) == 0) {
            return 2;
        }
        if ((value & 0xFFFF0000) == 0) {
            return 3;
        }
        if ((value & 0xFF000000) == 0) {
            return 4;
        }
        return 5;
    }

    /**
     * Returns the number of bytes that would be written by {@link #writeLong}.
     *
     * @param value the long integer to be written.
     *
     * @return the number of bytes that would be used to write the given long
     * integer.
     */
    public static int getWriteLongLength(long value) {

        if (value < -119) {
            value = -value - 119;
        } else if (value > 119) {
            value = value - 119;
        } else {
            return 1;
        }

        if ((value & 0xFFFFFFFFFFFFFF00L) == 0) {
            return 2;
        }
        if ((value & 0xFFFFFFFFFFFF0000L) == 0) {
            return 3;
        }
        if ((value & 0xFFFFFFFFFF000000L) == 0) {
            return 4;
        }
        if ((value & 0xFFFFFFFF00000000L) == 0) {
            return 5;
        }
        if ((value & 0xFFFFFF0000000000L) == 0) {
            return 6;
        }
        if ((value & 0xFFFF000000000000L) == 0) {
            return 7;
        }
        if ((value & 0xFF00000000000000L) == 0) {
            return 8;
        }
        return 9;
    }
}

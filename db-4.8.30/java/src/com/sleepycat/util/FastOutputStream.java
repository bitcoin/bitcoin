/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.util;

import java.io.IOException;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;

/**
 * A replacement for ByteArrayOutputStream that does not synchronize every
 * byte read.
 *
 * <p>This class extends {@link OutputStream} and its <code>write()</code>
 * methods allow it to be used as a standard output stream.  In addition, it
 * provides <code>writeFast()</code> methods that are not declared to throw
 * <code>IOException</code>.  <code>IOException</code> is never thrown by this
 * class.</p>
 *
 * @author Mark Hayes
 */
public class FastOutputStream extends OutputStream {

    /**
     * The default initial size of the buffer if no initialSize parameter is
     * specified.  This constant is 100 bytes.
     */
    public static final int DEFAULT_INIT_SIZE = 100;

    /**
     * The default amount that the buffer is increased when it is full.  This
     * constant is zero, which means to double the current buffer size.
     */
    public static final int DEFAULT_BUMP_SIZE = 0;

    private int len;
    private int bumpLen;
    private byte[] buf;

    /*
     * We can return the same byte[] for 0 length arrays.
     */
    private static byte[] ZERO_LENGTH_BYTE_ARRAY = new byte[0];

    /**
     * Creates an output stream with default sizes.
     */
    public FastOutputStream() {

        initBuffer(DEFAULT_INIT_SIZE, DEFAULT_BUMP_SIZE);
    }

    /**
     * Creates an output stream with a default bump size and a given initial
     * size.
     *
     * @param initialSize the initial size of the buffer.
     */
    public FastOutputStream(int initialSize) {

	initBuffer(initialSize, DEFAULT_BUMP_SIZE);
    }

    /**
     * Creates an output stream with a given bump size and initial size.
     *
     * @param initialSize the initial size of the buffer.
     *
     * @param bumpSize the amount to increment the buffer.
     */
    public FastOutputStream(int initialSize, int bumpSize) {

	initBuffer(initialSize, bumpSize);
    }

    /**
     * Creates an output stream with a given initial buffer and a default
     * bump size.
     *
     * @param buffer the initial buffer; will be owned by this object.
     */
    public FastOutputStream(byte[] buffer) {

        buf = buffer;
        bumpLen = DEFAULT_BUMP_SIZE;
    }

    /**
     * Creates an output stream with a given initial buffer and a given
     * bump size.
     *
     * @param buffer the initial buffer; will be owned by this object.
     *
     * @param bumpSize the amount to increment the buffer.  If zero (the
     * default), the current buffer size will be doubled when the buffer is
     * full.
     */
    public FastOutputStream(byte[] buffer, int bumpSize) {

        buf = buffer;
        bumpLen = bumpSize;
    }

    private void initBuffer(int bufferSize, int bumpLen) {
	buf = new byte[bufferSize];
	this.bumpLen = bumpLen;
    }

    // --- begin ByteArrayOutputStream compatible methods ---

    public int size() {

        return len;
    }

    public void reset() {

        len = 0;
    }

    @Override
    public void write(int b) {

        writeFast(b);
    }

    @Override
    public void write(byte[] fromBuf) {

        writeFast(fromBuf);
    }

    @Override
    public void write(byte[] fromBuf, int offset, int length) {

        writeFast(fromBuf, offset, length);
    }

    public void writeTo(OutputStream out) throws IOException {

        out.write(buf, 0, len);
    }

    @Override
    public String toString() {

        return new String(buf, 0, len);
    }

    public String toString(String encoding)
        throws UnsupportedEncodingException {

        return new String(buf, 0, len, encoding);
    }

    public byte[] toByteArray() {

	if (len == 0) {
	    return ZERO_LENGTH_BYTE_ARRAY;
	} else {
	    byte[] toBuf = new byte[len];
	    System.arraycopy(buf, 0, toBuf, 0, len);

	    return toBuf;
	}
    }

    // --- end ByteArrayOutputStream compatible methods ---

    /**
     * Equivalent to <code>write(int)<code> but does not throw
     * <code>IOException</code>.
     * @see #write(int)
     */
    public final void writeFast(int b) {

        if (len + 1 > buf.length)
            bump(1);

        buf[len++] = (byte) b;
    }

    /**
     * Equivalent to <code>write(byte[])<code> but does not throw
     * <code>IOException</code>.
     * @see #write(byte[])
     */
    public final void writeFast(byte[] fromBuf) {

        int needed = len + fromBuf.length - buf.length;
        if (needed > 0)
            bump(needed);

        System.arraycopy(fromBuf, 0, buf, len, fromBuf.length);
        len += fromBuf.length;
    }

    /**
     * Equivalent to <code>write(byte[],int,int)<code> but does not throw
     * <code>IOException</code>.
     * @see #write(byte[],int,int)
     */
    public final void writeFast(byte[] fromBuf, int offset, int length) {

        int needed = len + length - buf.length;
        if (needed > 0)
            bump(needed);

        System.arraycopy(fromBuf, offset, buf, len, length);
        len += length;
    }

    /**
     * Returns the buffer owned by this object.
     *
     * @return the buffer.
     */
    public byte[] getBufferBytes() {

        return buf;
    }

    /**
     * Returns the offset of the internal buffer.
     *
     * @return always zero currently.
     */
    public int getBufferOffset() {

        return 0;
    }

    /**
     * Returns the length used in the internal buffer, i.e., the offset at
     * which data will be written next.
     *
     * @return the buffer length.
     */
    public int getBufferLength() {

        return len;
    }

    /**
     * Ensure that at least the given number of bytes are available in the
     * internal buffer.
     *
     * @param sizeNeeded the number of bytes desired.
     */
    public void makeSpace(int sizeNeeded) {

        int needed = len + sizeNeeded - buf.length;
        if (needed > 0)
            bump(needed);
    }

    /**
     * Skip the given number of bytes in the buffer.
     *
     * @param sizeAdded number of bytes to skip.
     */
    public void addSize(int sizeAdded) {

        len += sizeAdded;
    }

    private void bump(int needed) {

        /* Double the buffer if the bumpLen is zero. */
        int bump = (bumpLen > 0) ? bumpLen : buf.length;

        byte[] toBuf = new byte[buf.length + needed + bump];

        System.arraycopy(buf, 0, toBuf, 0, len);

        buf = toBuf;
    }
}

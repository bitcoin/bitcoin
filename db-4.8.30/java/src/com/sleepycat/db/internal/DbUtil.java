/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */
package com.sleepycat.db.internal;

/**
 *  DbUtil is a simple class that holds a few static utility functions other
 *  parts of the package share and that don't have a good home elsewhere. (For
 *  now, that's limited to byte-array-to-int conversion and back.)
 */

public class DbUtil {
    /**
     *  Get the u_int32_t stored beginning at offset "offset" into
     *  array "arr". We have to do the conversion manually since it's
     *  a C-native int, and we're not really supposed to make this
     *  kind of cast in Java.
     *
     * @return    Description of the Return Value
     */
    public static int array2int(byte[] arr, int offset) {
        int b1;
        int b2;
        int b3;
        int b4;
        int pos = offset;

        // Get the component bytes;  b4 is most significant, b1 least.
        if (big_endian) {
            b4 = arr[pos++];
            b3 = arr[pos++];
            b2 = arr[pos++];
            b1 = arr[pos];
        } else {
            b1 = arr[pos++];
            b2 = arr[pos++];
            b3 = arr[pos++];
            b4 = arr[pos];
        }

        // Bytes are signed.  Convert [-128, -1] to [128, 255].
        if (b1 < 0) {
            b1 += 256;
        }
        if (b2 < 0) {
            b2 += 256;
        }
        if (b3 < 0) {
            b3 += 256;
        }
        if (b4 < 0) {
            b4 += 256;
        }

        // Put the bytes in their proper places in an int.
        b2 <<= 8;
        b3 <<= 16;
        b4 <<= 24;

        // Return their sum.
        return (b1 + b2 + b3 + b4);
    }


    /**
     *  Store the specified u_int32_t, with endianness appropriate to
     *  the platform we're running on, into four consecutive bytes of
     *  the specified byte array, starting from the specified offset.
     */
    public static void int2array(int n, byte[] arr, int offset) {
        int b1;
        int b2;
        int b3;
        int b4;
        int pos = offset;

        b1 = n & 0xff;
        b2 = (n >> 8) & 0xff;
        b3 = (n >> 16) & 0xff;
        b4 = (n >> 24) & 0xff;

        // Bytes are signed.  Convert [128, 255] to [-128, -1].
        if (b1 >= 128) {
            b1 -= 256;
        }
        if (b2 >= 128) {
            b2 -= 256;
        }
        if (b3 >= 128) {
            b3 -= 256;
        }
        if (b4 >= 128) {
            b4 -= 256;
        }

        // Put the bytes in the appropriate place in the array.
        if (big_endian) {
            arr[pos++] = (byte) b4;
            arr[pos++] = (byte) b3;
            arr[pos++] = (byte) b2;
            arr[pos] = (byte) b1;
        } else {
            arr[pos++] = (byte) b1;
            arr[pos++] = (byte) b2;
            arr[pos++] = (byte) b3;
            arr[pos] = (byte) b4;
        }
    }


    /**
     *  Convert a byte array to a concise, readable string suitable
     *  for use in toString methods of the *Stat classes.
     *
     * @return    Description of the Return Value
     */
    public static String byteArrayToString(byte[] barr) {
        if (barr == null) {
            return "null";
        }

        StringBuffer sb = new StringBuffer();
        int len = barr.length;
        for (int i = 0; i < len; i++) {
            sb.append('x');
            int val = (barr[i] >> 4) & 0xf;
            if (val < 10) {
                sb.append((char) ('0' + val));
            } else {
                sb.append((char) ('a' + val - 10));
            }
            val = barr[i] & 0xf;
            if (val < 10) {
                sb.append((char) ('0' + val));
            } else {
                sb.append((char) ('a' + val - 10));
            }
        }
        return sb.toString();
    }


    /**
     *  Convert an object array to a string, suitable for use in
     *  toString methods of the *Stat classes.
     *
     * @return    Description of the Return Value
     */
    public static String objectArrayToString(Object[] arr, String name) {
        if (arr == null) {
            return "null";
        }

        StringBuffer sb = new StringBuffer();
        int len = arr.length;
        for (int i = 0; i < len; i++) {
            sb.append("\n    " + name + "[" + i + "]:\n");
            sb.append("    " + arr[i].toString());
        }
        return sb.toString();
    }

    public static int default_lorder() {
        return big_endian ? 4321 : 1234;
    }

    private final static boolean big_endian = is_big_endian();

    /**
     * @return    Description of the Return Value
     */
    private native static boolean is_big_endian();
}

/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.impl;

import java.math.BigInteger;

/**
 * Widens a value returned by another input when any readXxx method is called.
 * Used to cause an Accessor to read a widened value.
 *
 * For non-key fields we support all Java primitive widening:
 * - byte to short, int, long, float, double or BigInteger
 * - short to int, long, float, double or BigInteger
 * - char to int, long, float, double or BigInteger
 * - int to long, float, double or BigInteger
 * - long to float, double or BigInteger
 * - float to double
 *
 * For non-key fields we also support:
 * - Java reference widening
 * - primitive to primitive wrapper
 * - Java primitive widening to corresponding primitive wrappers
 * - Java widening of primitive wrapper to primitive wrapper
 *
 * For secondary keys fields we ONLY support:
 * - primitive to primitive wrapper
 *
 * But for primary keys and composite key fields we ONLY support:
 * - primitive to primitive wrapper
 * - primitive wrapper to primitive
 * These conversions don't require any converter, since the stored format is
 * not changed.  A WidenerInput is not used for these changes.
 *
 * @author Mark Hayes
 */
class WidenerInput extends AbstractInput {

    private EntityInput input;
    private int fromFormatId;
    private int toFormatId;

    /**
     * Returns whether widening is supported by this class.  If false is
     * returned by this method, then widening is disallowed and a field
     * converter or deleter is necessary.
     */
    static boolean isWideningSupported(Format fromFormat,
                                       Format toFormat,
                                       boolean isSecKeyField) {
        int fromFormatId = fromFormat.getId();
        int toFormatId = toFormat.getId();

        switch (fromFormatId) {
        case Format.ID_BOOL:
            switch (toFormatId) {
            case Format.ID_BOOL_W:
                return true;
            default:
                return false;
            }
        case Format.ID_BYTE:
            switch (toFormatId) {
            case Format.ID_BYTE_W:
                return true;
            case Format.ID_SHORT:
            case Format.ID_SHORT_W:
            case Format.ID_INT:
            case Format.ID_INT_W:
            case Format.ID_LONG:
            case Format.ID_LONG_W:
            case Format.ID_FLOAT:
            case Format.ID_FLOAT_W:
            case Format.ID_DOUBLE:
            case Format.ID_DOUBLE_W:
            case Format.ID_BIGINT:
                return !isSecKeyField;
            default:
                return false;
            }
        case Format.ID_BYTE_W:
            switch (toFormatId) {
            case Format.ID_SHORT_W:
            case Format.ID_INT_W:
            case Format.ID_LONG_W:
            case Format.ID_FLOAT_W:
            case Format.ID_DOUBLE_W:
            case Format.ID_BIGINT:
                return !isSecKeyField;
            default:
                return false;
            }
        case Format.ID_SHORT:
            switch (toFormatId) {
            case Format.ID_SHORT_W:
                return true;
            case Format.ID_INT:
            case Format.ID_INT_W:
            case Format.ID_LONG:
            case Format.ID_LONG_W:
            case Format.ID_FLOAT:
            case Format.ID_FLOAT_W:
            case Format.ID_DOUBLE:
            case Format.ID_DOUBLE_W:
            case Format.ID_BIGINT:
                return !isSecKeyField;
            default:
                return false;
            }
        case Format.ID_SHORT_W:
            switch (toFormatId) {
            case Format.ID_INT_W:
            case Format.ID_LONG_W:
            case Format.ID_FLOAT_W:
            case Format.ID_DOUBLE_W:
            case Format.ID_BIGINT:
                return !isSecKeyField;
            default:
                return false;
            }
        case Format.ID_INT:
            switch (toFormatId) {
            case Format.ID_INT_W:
                return true;
            case Format.ID_LONG:
            case Format.ID_LONG_W:
            case Format.ID_FLOAT:
            case Format.ID_FLOAT_W:
            case Format.ID_DOUBLE:
            case Format.ID_DOUBLE_W:
            case Format.ID_BIGINT:
                return !isSecKeyField;
            default:
                return false;
            }
        case Format.ID_INT_W:
            switch (toFormatId) {
            case Format.ID_LONG_W:
            case Format.ID_FLOAT_W:
            case Format.ID_DOUBLE_W:
            case Format.ID_BIGINT:
                return !isSecKeyField;
            default:
                return false;
            }
        case Format.ID_LONG:
            switch (toFormatId) {
            case Format.ID_LONG_W:
                return true;
            case Format.ID_FLOAT:
            case Format.ID_FLOAT_W:
            case Format.ID_DOUBLE:
            case Format.ID_DOUBLE_W:
            case Format.ID_BIGINT:
                return !isSecKeyField;
            default:
                return false;
            }
        case Format.ID_LONG_W:
            switch (toFormatId) {
            case Format.ID_FLOAT_W:
            case Format.ID_DOUBLE_W:
            case Format.ID_BIGINT:
                return !isSecKeyField;
            default:
                return false;
            }
        case Format.ID_FLOAT:
            switch (toFormatId) {
            case Format.ID_FLOAT_W:
                return true;
            case Format.ID_DOUBLE:
            case Format.ID_DOUBLE_W:
                return !isSecKeyField;
            default:
                return false;
            }
        case Format.ID_FLOAT_W:
            switch (toFormatId) {
            case Format.ID_DOUBLE_W:
                return !isSecKeyField;
            default:
                return false;
            }
        case Format.ID_DOUBLE:
            switch (toFormatId) {
            case Format.ID_DOUBLE_W:
                return true;
            default:
                return false;
            }
        case Format.ID_CHAR:
            switch (toFormatId) {
            case Format.ID_CHAR_W:
                return true;
            case Format.ID_INT:
            case Format.ID_INT_W:
            case Format.ID_LONG:
            case Format.ID_LONG_W:
            case Format.ID_FLOAT:
            case Format.ID_FLOAT_W:
            case Format.ID_DOUBLE:
            case Format.ID_DOUBLE_W:
            case Format.ID_BIGINT:
                return !isSecKeyField;
            default:
                return false;
            }
        case Format.ID_CHAR_W:
            switch (toFormatId) {
            case Format.ID_INT_W:
            case Format.ID_LONG_W:
            case Format.ID_FLOAT_W:
            case Format.ID_DOUBLE_W:
            case Format.ID_BIGINT:
                return !isSecKeyField;
            default:
                return false;
            }
        default:
            return false;
        }
    }

    WidenerInput(EntityInput input, int fromFormatId, int toFormatId) {
        super(input.getCatalog(), input.isRawAccess());
        this.input = input;
        this.fromFormatId = fromFormatId;
        this.toFormatId = toFormatId;
    }

    public void registerPriKeyObject(Object o) {
        input.registerPriKeyObject(o);
    }

    public int readArrayLength() {
        throw new UnsupportedOperationException();
    }

    public int readEnumConstant(String[] names) {
        throw new UnsupportedOperationException();
    }

    public void skipField(Format declaredFormat) {
        throw new UnsupportedOperationException();
    }

    public String readString() {
        throw new UnsupportedOperationException();
    }

    public Object readKeyObject(Format fromFormat) {
        return readObject();
    }

    public Object readObject() {
        switch (fromFormatId) {
        case Format.ID_BOOL:
            checkToFormat(Format.ID_BOOL_W);
            return input.readBoolean();
        case Format.ID_BYTE:
            return byteToObject(input.readByte());
        case Format.ID_BYTE_W:
            Byte b = (Byte) input.readObject();
            return (b != null) ? byteToObject(b) : null;
        case Format.ID_SHORT:
            return shortToObject(input.readShort());
        case Format.ID_SHORT_W:
            Short s = (Short) input.readObject();
            return (s != null) ? shortToObject(s) : null;
        case Format.ID_INT:
            return intToObject(input.readInt());
        case Format.ID_INT_W:
            Integer i = (Integer) input.readObject();
            return (i != null) ? intToObject(i) : null;
        case Format.ID_LONG:
            return longToObject(input.readLong());
        case Format.ID_LONG_W:
            Long l = (Long) input.readObject();
            return (l != null) ? longToObject(l) : null;
        case Format.ID_FLOAT:
            return floatToObject(input.readSortedFloat());
        case Format.ID_FLOAT_W:
            Float f = (Float) input.readObject();
            return (f != null) ? floatToObject(f) : null;
        case Format.ID_DOUBLE:
            checkToFormat(Format.ID_DOUBLE_W);
            return input.readSortedDouble();
        case Format.ID_CHAR:
            return charToObject(input.readChar());
        case Format.ID_CHAR_W:
            Character c = (Character) input.readObject();
            return (c != null) ? charToObject(c) : null;
        default:
            throw new IllegalStateException(String.valueOf(fromFormatId));
        }
    }

    private Object byteToObject(byte v) {
        switch (toFormatId) {
        case Format.ID_BYTE:
        case Format.ID_BYTE_W:
            return Byte.valueOf(v);
        case Format.ID_SHORT:
        case Format.ID_SHORT_W:
            return Short.valueOf(v);
        case Format.ID_INT:
        case Format.ID_INT_W:
            return Integer.valueOf(v);
        case Format.ID_LONG:
        case Format.ID_LONG_W:
            return Long.valueOf(v);
        case Format.ID_FLOAT:
        case Format.ID_FLOAT_W:
            return Float.valueOf(v);
        case Format.ID_DOUBLE:
        case Format.ID_DOUBLE_W:
            return Double.valueOf(v);
        case Format.ID_BIGINT:
            return BigInteger.valueOf(v);
        default:
            throw new IllegalStateException(String.valueOf(toFormatId));
        }
    }

    private Object shortToObject(short v) {
        switch (toFormatId) {
        case Format.ID_SHORT:
        case Format.ID_SHORT_W:
            return Short.valueOf(v);
        case Format.ID_INT:
        case Format.ID_INT_W:
            return Integer.valueOf(v);
        case Format.ID_LONG:
        case Format.ID_LONG_W:
            return Long.valueOf(v);
        case Format.ID_FLOAT:
        case Format.ID_FLOAT_W:
            return Float.valueOf(v);
        case Format.ID_DOUBLE:
        case Format.ID_DOUBLE_W:
            return Double.valueOf(v);
        case Format.ID_BIGINT:
            return BigInteger.valueOf(v);
        default:
            throw new IllegalStateException(String.valueOf(toFormatId));
        }
    }

    private Object intToObject(int v) {
        switch (toFormatId) {
        case Format.ID_INT:
        case Format.ID_INT_W:
            return Integer.valueOf(v);
        case Format.ID_LONG:
        case Format.ID_LONG_W:
            return Long.valueOf(v);
        case Format.ID_FLOAT:
        case Format.ID_FLOAT_W:
            return Float.valueOf(v);
        case Format.ID_DOUBLE:
        case Format.ID_DOUBLE_W:
            return Double.valueOf(v);
        case Format.ID_BIGINT:
            return BigInteger.valueOf(v);
        default:
            throw new IllegalStateException(String.valueOf(toFormatId));
        }
    }

    private Object longToObject(long v) {
        switch (toFormatId) {
        case Format.ID_LONG:
        case Format.ID_LONG_W:
            return Long.valueOf(v);
        case Format.ID_FLOAT:
        case Format.ID_FLOAT_W:
            return Float.valueOf(v);
        case Format.ID_DOUBLE:
        case Format.ID_DOUBLE_W:
            return Double.valueOf(v);
        case Format.ID_BIGINT:
            return BigInteger.valueOf(v);
        default:
            throw new IllegalStateException(String.valueOf(toFormatId));
        }
    }

    private Object floatToObject(float v) {
        switch (toFormatId) {
        case Format.ID_FLOAT:
        case Format.ID_FLOAT_W:
            return Float.valueOf(v);
        case Format.ID_DOUBLE:
        case Format.ID_DOUBLE_W:
            return Double.valueOf(v);
        default:
            throw new IllegalStateException(String.valueOf(toFormatId));
        }
    }

    private Object charToObject(char v) {
        switch (toFormatId) {
        case Format.ID_CHAR:
        case Format.ID_CHAR_W:
            return Character.valueOf(v);
        case Format.ID_INT:
        case Format.ID_INT_W:
            return Integer.valueOf(v);
        case Format.ID_LONG:
        case Format.ID_LONG_W:
            return Long.valueOf(v);
        case Format.ID_FLOAT:
        case Format.ID_FLOAT_W:
            return Float.valueOf(v);
        case Format.ID_DOUBLE:
        case Format.ID_DOUBLE_W:
            return Double.valueOf(v);
        case Format.ID_BIGINT:
            return BigInteger.valueOf(v);
        default:
            throw new IllegalStateException(String.valueOf(toFormatId));
        }
    }

    public char readChar() {
        throw new IllegalStateException(String.valueOf(fromFormatId));
    }

    public boolean readBoolean() {
        throw new IllegalStateException(String.valueOf(fromFormatId));
    }

    public byte readByte() {
        throw new IllegalStateException(String.valueOf(fromFormatId));
    }

    public short readShort() {
        checkToFormat(Format.ID_SHORT);
        switch (fromFormatId) {
        case Format.ID_BYTE:
            return input.readByte();
        default:
            throw new IllegalStateException(String.valueOf(fromFormatId));
        }
    }

    public int readInt() {
        checkToFormat(Format.ID_INT);
        switch (fromFormatId) {
        case Format.ID_BYTE:
            return input.readByte();
        case Format.ID_SHORT:
            return input.readShort();
        case Format.ID_CHAR:
            return input.readChar();
        default:
            throw new IllegalStateException(String.valueOf(fromFormatId));
        }
    }

    public long readLong() {
        checkToFormat(Format.ID_LONG);
        switch (fromFormatId) {
        case Format.ID_BYTE:
            return input.readByte();
        case Format.ID_SHORT:
            return input.readShort();
        case Format.ID_INT:
            return input.readInt();
        case Format.ID_CHAR:
            return input.readChar();
        default:
            throw new IllegalStateException(String.valueOf(fromFormatId));
        }
    }

    public float readSortedFloat() {
        checkToFormat(Format.ID_FLOAT);
        switch (fromFormatId) {
        case Format.ID_BYTE:
            return input.readByte();
        case Format.ID_SHORT:
            return input.readShort();
        case Format.ID_INT:
            return input.readInt();
        case Format.ID_LONG:
            return input.readLong();
        case Format.ID_CHAR:
            return input.readChar();
        default:
            throw new IllegalStateException(String.valueOf(fromFormatId));
        }
    }

    public double readSortedDouble() {
        checkToFormat(Format.ID_DOUBLE);
        switch (fromFormatId) {
        case Format.ID_BYTE:
            return input.readByte();
        case Format.ID_SHORT:
            return input.readShort();
        case Format.ID_INT:
            return input.readInt();
        case Format.ID_LONG:
            return input.readLong();
        case Format.ID_FLOAT:
            return input.readSortedFloat();
        case Format.ID_CHAR:
            return input.readChar();
        default:
            throw new IllegalStateException(String.valueOf(fromFormatId));
        }
    }

    public BigInteger readBigInteger() {
        checkToFormat(Format.ID_BIGINT);
        switch (fromFormatId) {
        case Format.ID_BYTE:
            return BigInteger.valueOf(input.readByte());
        case Format.ID_SHORT:
            return BigInteger.valueOf(input.readShort());
        case Format.ID_INT:
            return BigInteger.valueOf(input.readInt());
        case Format.ID_LONG:
            return BigInteger.valueOf(input.readLong());
        case Format.ID_CHAR:
            return BigInteger.valueOf(input.readChar());
        default:
            throw new IllegalStateException(String.valueOf(fromFormatId));
        }
    }

    private void checkToFormat(int id) {
        if (toFormatId != id) {
            throw new IllegalStateException(String.valueOf(toFormatId));
        }
    }
}

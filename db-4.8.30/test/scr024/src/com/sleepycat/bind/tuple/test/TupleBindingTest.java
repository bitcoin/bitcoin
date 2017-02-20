/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.bind.tuple.test;

import java.math.BigInteger;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;

import com.sleepycat.bind.EntityBinding;
import com.sleepycat.bind.EntryBinding;
import com.sleepycat.bind.tuple.BigIntegerBinding;
import com.sleepycat.bind.tuple.BooleanBinding;
import com.sleepycat.bind.tuple.ByteBinding;
import com.sleepycat.bind.tuple.CharacterBinding;
import com.sleepycat.bind.tuple.DoubleBinding;
import com.sleepycat.bind.tuple.FloatBinding;
import com.sleepycat.bind.tuple.IntegerBinding;
import com.sleepycat.bind.tuple.LongBinding;
import com.sleepycat.bind.tuple.ShortBinding;
import com.sleepycat.bind.tuple.SortedDoubleBinding;
import com.sleepycat.bind.tuple.SortedFloatBinding;
import com.sleepycat.bind.tuple.StringBinding;
import com.sleepycat.bind.tuple.TupleBinding;
import com.sleepycat.bind.tuple.TupleInput;
import com.sleepycat.bind.tuple.TupleInputBinding;
import com.sleepycat.bind.tuple.TupleMarshalledBinding;
import com.sleepycat.bind.tuple.TupleOutput;
import com.sleepycat.bind.tuple.TupleTupleMarshalledBinding;
import com.sleepycat.db.DatabaseEntry;
import com.sleepycat.util.ExceptionUnwrapper;
import com.sleepycat.util.FastOutputStream;
import com.sleepycat.util.test.SharedTestUtils;

/**
 * @author Mark Hayes
 */
public class TupleBindingTest extends TestCase {

    private DatabaseEntry buffer;
    private DatabaseEntry keyBuffer;

    public static void main(String[] args) {
        junit.framework.TestResult tr =
            junit.textui.TestRunner.run(suite());
        if (tr.errorCount() > 0 ||
            tr.failureCount() > 0) {
            System.exit(1);
        } else {
            System.exit(0);
        }
    }

    public static Test suite() {
        TestSuite suite = new TestSuite(TupleBindingTest.class);
        return suite;
    }

    public TupleBindingTest(String name) {

        super(name);
    }

    @Override
    public void setUp() {

        SharedTestUtils.printTestName("TupleBindingTest." + getName());
        buffer = new DatabaseEntry();
        keyBuffer = new DatabaseEntry();
    }

    @Override
    public void tearDown() {

        /* Ensure that GC can cleanup. */
        buffer = null;
        keyBuffer = null;
    }

    @Override
    public void runTest()
        throws Throwable {

        try {
            super.runTest();
        } catch (Exception e) {
            throw ExceptionUnwrapper.unwrap(e);
        }
    }

    private void primitiveBindingTest(Class primitiveCls, Class compareCls,
                                      Object val, int byteSize) {

        TupleBinding binding = TupleBinding.getPrimitiveBinding(primitiveCls);

        /* Test standard object binding. */

        binding.objectToEntry(val, buffer);
        assertEquals(byteSize, buffer.getSize());

        Object val2 = binding.entryToObject(buffer);
        assertSame(compareCls, val2.getClass());
        assertEquals(val, val2);

        Object valWithWrongCls = (primitiveCls == String.class)
                      ? ((Object) new Integer(0)) : ((Object) new String(""));
        try {
            binding.objectToEntry(valWithWrongCls, buffer);
        }
        catch (ClassCastException expected) {}

        /* Test nested tuple binding. */
	forMoreCoverageTest(binding, val);
    }

    private void forMoreCoverageTest(TupleBinding val1,Object val2) {

        TupleOutput output = new TupleOutput();
        output.writeString("abc");
        val1.objectToEntry(val2, output);
        output.writeString("xyz");

        TupleInput input = new TupleInput(output);
        assertEquals("abc", input.readString());
        Object val3 = val1.entryToObject(input);
        assertEquals("xyz", input.readString());

        assertEquals(0, input.available());
        assertSame(val2.getClass(), val3.getClass());
        assertEquals(val2, val3);
    }

    public void testPrimitiveBindings() {

        primitiveBindingTest(String.class, String.class,
                             "abc", 4);

        primitiveBindingTest(Character.class, Character.class,
                             new Character('a'), 2);
        primitiveBindingTest(Boolean.class, Boolean.class,
                             new Boolean(true), 1);
        primitiveBindingTest(Byte.class, Byte.class,
                             new Byte((byte) 123), 1);
        primitiveBindingTest(Short.class, Short.class,
                             new Short((short) 123), 2);
        primitiveBindingTest(Integer.class, Integer.class,
                             new Integer(123), 4);
	primitiveBindingTest(Long.class, Long.class,
                             new Long(123), 8);
        primitiveBindingTest(Float.class, Float.class,
                             new Float(123.123), 4);
        primitiveBindingTest(Double.class, Double.class,
                             new Double(123.123), 8);

        primitiveBindingTest(Character.TYPE, Character.class,
                             new Character('a'), 2);
        primitiveBindingTest(Boolean.TYPE, Boolean.class,
                             new Boolean(true), 1);
        primitiveBindingTest(Byte.TYPE, Byte.class,
                             new Byte((byte) 123), 1);
        primitiveBindingTest(Short.TYPE, Short.class,
                             new Short((short) 123), 2);
        primitiveBindingTest(Integer.TYPE, Integer.class,
                             new Integer(123), 4);
        primitiveBindingTest(Long.TYPE, Long.class,
                             new Long(123), 8);
        primitiveBindingTest(Float.TYPE, Float.class,
                             new Float(123.123), 4);
        primitiveBindingTest(Double.TYPE, Double.class,
                             new Double(123.123), 8);

        DatabaseEntry entry = new DatabaseEntry();

        StringBinding.stringToEntry("abc", entry);
	assertEquals(4, entry.getData().length);
        assertEquals("abc", StringBinding.entryToString(entry));

        new StringBinding().objectToEntry("abc", entry);
	assertEquals(4, entry.getData().length);

        StringBinding.stringToEntry(null, entry);
	assertEquals(2, entry.getData().length);
        assertEquals(null, StringBinding.entryToString(entry));

        new StringBinding().objectToEntry(null, entry);
	assertEquals(2, entry.getData().length);

        CharacterBinding.charToEntry('a', entry);
	assertEquals(2, entry.getData().length);
        assertEquals('a', CharacterBinding.entryToChar(entry));

        new CharacterBinding().objectToEntry(new Character('a'), entry);
	assertEquals(2, entry.getData().length);

        BooleanBinding.booleanToEntry(true, entry);
	assertEquals(1, entry.getData().length);
        assertEquals(true, BooleanBinding.entryToBoolean(entry));

        new BooleanBinding().objectToEntry(Boolean.TRUE, entry);
	assertEquals(1, entry.getData().length);

        ByteBinding.byteToEntry((byte) 123, entry);
	assertEquals(1, entry.getData().length);
        assertEquals((byte) 123, ByteBinding.entryToByte(entry));

        ShortBinding.shortToEntry((short) 123, entry);
	assertEquals(2, entry.getData().length);
        assertEquals((short) 123, ShortBinding.entryToShort(entry));

        new ByteBinding().objectToEntry(new Byte((byte) 123), entry);
	assertEquals(1, entry.getData().length);

        IntegerBinding.intToEntry(123, entry);
	assertEquals(4, entry.getData().length);
        assertEquals(123, IntegerBinding.entryToInt(entry));

        new IntegerBinding().objectToEntry(new Integer(123), entry);
	assertEquals(4, entry.getData().length);

        LongBinding.longToEntry(123, entry);
	assertEquals(8, entry.getData().length);
        assertEquals(123, LongBinding.entryToLong(entry));

        new LongBinding().objectToEntry(new Long(123), entry);
	assertEquals(8, entry.getData().length);

        FloatBinding.floatToEntry((float) 123.123, entry);
	assertEquals(4, entry.getData().length);
        assertTrue(((float) 123.123) == FloatBinding.entryToFloat(entry));

        new FloatBinding().objectToEntry(new Float((float) 123.123), entry);
	assertEquals(4, entry.getData().length);

        DoubleBinding.doubleToEntry(123.123, entry);
	assertEquals(8, entry.getData().length);
        assertTrue(123.123 == DoubleBinding.entryToDouble(entry));

        new DoubleBinding().objectToEntry(new Double(123.123), entry);
	assertEquals(8, entry.getData().length);

        BigIntegerBinding.bigIntegerToEntry
                (new BigInteger("1234567890123456"), entry);
        assertEquals(9, entry.getData().length);
        assertTrue((new BigInteger("1234567890123456")).equals
		   (BigIntegerBinding.entryToBigInteger(entry)));

        new BigIntegerBinding().objectToEntry
                (new BigInteger("1234567890123456"), entry);
        assertEquals(9, entry.getData().length);
        forMoreCoverageTest(new BigIntegerBinding(),
                            new BigInteger("1234567890123456"));

        SortedFloatBinding.floatToEntry((float) 123.123, entry);
	assertEquals(4, entry.getData().length);
        assertTrue(((float) 123.123) ==
                   SortedFloatBinding.entryToFloat(entry));

        new SortedFloatBinding().objectToEntry
            (new Float((float) 123.123), entry);
	assertEquals(4, entry.getData().length);
        forMoreCoverageTest(new SortedFloatBinding(),
                            new Float((float) 123.123));

        SortedDoubleBinding.doubleToEntry(123.123, entry);
	assertEquals(8, entry.getData().length);
        assertTrue(123.123 == SortedDoubleBinding.entryToDouble(entry));

        new SortedDoubleBinding().objectToEntry(new Double(123.123), entry);
	assertEquals(8, entry.getData().length);
        forMoreCoverageTest(new SortedDoubleBinding(),
                            new Double(123.123));
    }

    public void testTupleInputBinding() {

        EntryBinding binding = new TupleInputBinding();

        TupleOutput out = new TupleOutput();
        out.writeString("abc");
        binding.objectToEntry(new TupleInput(out), buffer);
        assertEquals(4, buffer.getSize());

        Object result = binding.entryToObject(buffer);
        assertTrue(result instanceof TupleInput);
        TupleInput in = (TupleInput) result;
        assertEquals("abc", in.readString());
        assertEquals(0, in.available());
    }

    // also tests TupleBinding since TupleMarshalledBinding extends it
    public void testTupleMarshalledBinding() {

        EntryBinding binding =
            new TupleMarshalledBinding(MarshalledObject.class);

        MarshalledObject val = new MarshalledObject("abc", "", "", "");
        binding.objectToEntry(val, buffer);
        assertEquals(val.expectedDataLength(), buffer.getSize());

        Object result = binding.entryToObject(buffer);
        assertTrue(result instanceof MarshalledObject);
        val = (MarshalledObject) result;
        assertEquals("abc", val.getData());
    }

    // also tests TupleTupleBinding since TupleTupleMarshalledBinding extends
    // it
    public void testTupleTupleMarshalledBinding() {

        EntityBinding binding =
            new TupleTupleMarshalledBinding(MarshalledObject.class);

        MarshalledObject val = new MarshalledObject("abc", "primary",
                                                    "index1", "index2");
        binding.objectToData(val, buffer);
        assertEquals(val.expectedDataLength(), buffer.getSize());
        binding.objectToKey(val, keyBuffer);
        assertEquals(val.expectedKeyLength(), keyBuffer.getSize());

        Object result = binding.entryToObject(keyBuffer, buffer);
        assertTrue(result instanceof MarshalledObject);
        val = (MarshalledObject) result;
        assertEquals("abc", val.getData());
        assertEquals("primary", val.getPrimaryKey());
        assertEquals("index1", val.getIndexKey1());
        assertEquals("index2", val.getIndexKey2());
    }

    public void testBufferSize() {

        CaptureSizeBinding binding = new CaptureSizeBinding();

        binding.objectToEntry("x", buffer);
        assertEquals("x", binding.entryToObject(buffer));
        assertEquals(FastOutputStream.DEFAULT_INIT_SIZE, binding.bufSize);

        binding.setTupleBufferSize(1000);
        binding.objectToEntry("x", buffer);
        assertEquals("x", binding.entryToObject(buffer));
        assertEquals(1000, binding.bufSize);
    }

    private class CaptureSizeBinding extends TupleBinding {

        int bufSize;

        CaptureSizeBinding() {
            super();
        }

        @Override
        public TupleOutput getTupleOutput(Object object) {
            TupleOutput out = super.getTupleOutput(object);
            bufSize = out.getBufferBytes().length;
            return out;
        }

        @Override
        public Object entryToObject(TupleInput input) {
            return input.readString();
        }

        @Override
        public void objectToEntry(Object object, TupleOutput output) {
            assertEquals(bufSize, output.getBufferBytes().length);
            output.writeString((String) object);
        }
    }

    public void testBufferOverride() {

        TupleOutput out = new TupleOutput(new byte[10]);
        CachedOutputBinding binding = new CachedOutputBinding(out);

        binding.used = false;
        binding.objectToEntry("x", buffer);
        assertEquals("x", binding.entryToObject(buffer));
        assertTrue(binding.used);

        binding.used = false;
        binding.objectToEntry("aaaaaaaaaaaaaaaaaaaaaa", buffer);
        assertEquals("aaaaaaaaaaaaaaaaaaaaaa", binding.entryToObject(buffer));
        assertTrue(binding.used);

        binding.used = false;
        binding.objectToEntry("x", buffer);
        assertEquals("x", binding.entryToObject(buffer));
        assertTrue(binding.used);
    }

    private class CachedOutputBinding extends TupleBinding {

        TupleOutput out;
        boolean used;

        CachedOutputBinding(TupleOutput out) {
            super();
            this.out = out;
        }

        @Override
        public TupleOutput getTupleOutput(Object object) {
            out.reset();
            used = true;
            return out;
        }

        @Override
        public Object entryToObject(TupleInput input) {
            return input.readString();
        }

        @Override
        public void objectToEntry(Object object, TupleOutput output) {
            assertSame(out, output);
            output.writeString((String) object);
        }
    }
}

/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.bind.serial.test;

import java.io.Serializable;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;

import com.sleepycat.bind.EntityBinding;
import com.sleepycat.bind.serial.ClassCatalog;
import com.sleepycat.bind.serial.SerialBinding;
import com.sleepycat.bind.serial.SerialSerialBinding;
import com.sleepycat.bind.serial.TupleSerialMarshalledBinding;
import com.sleepycat.db.DatabaseEntry;
import com.sleepycat.util.ExceptionUnwrapper;
import com.sleepycat.util.FastOutputStream;
import com.sleepycat.util.test.SharedTestUtils;

/**
 * @author Mark Hayes
 */
public class SerialBindingTest extends TestCase {

    private ClassCatalog catalog;
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
        TestSuite suite = new TestSuite(SerialBindingTest.class);
        return suite;
    }

    public SerialBindingTest(String name) {

        super(name);
    }

    @Override
    public void setUp() {

        SharedTestUtils.printTestName("SerialBindingTest." + getName());
        catalog = new TestClassCatalog();
        buffer = new DatabaseEntry();
        keyBuffer = new DatabaseEntry();
    }

    @Override
    public void tearDown() {

        /* Ensure that GC can cleanup. */
        catalog = null;
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

    private void primitiveBindingTest(Object val) {

        Class cls = val.getClass();
        SerialBinding binding = new SerialBinding(catalog, cls);

        binding.objectToEntry(val, buffer);
        assertTrue(buffer.getSize() > 0);

        Object val2 = binding.entryToObject(buffer);
        assertSame(cls, val2.getClass());
        assertEquals(val, val2);

        Object valWithWrongCls = (cls == String.class)
                      ? ((Object) new Integer(0)) : ((Object) new String(""));
        try {
            binding.objectToEntry(valWithWrongCls, buffer);
        } catch (IllegalArgumentException expected) {}
    }

    public void testPrimitiveBindings() {

        primitiveBindingTest("abc");
        primitiveBindingTest(new Character('a'));
        primitiveBindingTest(new Boolean(true));
        primitiveBindingTest(new Byte((byte) 123));
        primitiveBindingTest(new Short((short) 123));
        primitiveBindingTest(new Integer(123));
        primitiveBindingTest(new Long(123));
        primitiveBindingTest(new Float(123.123));
        primitiveBindingTest(new Double(123.123));
    }

    public void testNullObjects() {

        SerialBinding binding = new SerialBinding(catalog, null);
        buffer.setSize(0);
        binding.objectToEntry(null, buffer);
        assertTrue(buffer.getSize() > 0);
        assertEquals(null, binding.entryToObject(buffer));
    }

    public void testSerialSerialBinding() {

        SerialBinding keyBinding = new SerialBinding(catalog, String.class);
        SerialBinding valueBinding = new SerialBinding(catalog, String.class);
        EntityBinding binding = new MySerialSerialBinding(keyBinding,
                                                          valueBinding);

        String val = "key#value?indexKey";
        binding.objectToData(val, buffer);
        assertTrue(buffer.getSize() > 0);
        binding.objectToKey(val, keyBuffer);
        assertTrue(keyBuffer.getSize() > 0);

        Object result = binding.entryToObject(keyBuffer, buffer);
        assertEquals(val, result);
    }

    // also tests TupleSerialBinding since TupleSerialMarshalledBinding extends
    // it
    public void testTupleSerialMarshalledBinding() {

        SerialBinding valueBinding = new SerialBinding(catalog,
                                                    MarshalledObject.class);
        EntityBinding binding =
            new TupleSerialMarshalledBinding(valueBinding);

        MarshalledObject val = new MarshalledObject("abc", "primary",
                                                    "index1", "index2");
        binding.objectToData(val, buffer);
        assertTrue(buffer.getSize() > 0);
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

        CaptureSizeBinding binding =
            new CaptureSizeBinding(catalog, String.class);

        binding.objectToEntry("x", buffer);
        assertEquals("x", binding.entryToObject(buffer));
        assertEquals(FastOutputStream.DEFAULT_INIT_SIZE, binding.bufSize);

        binding.setSerialBufferSize(1000);
        binding.objectToEntry("x", buffer);
        assertEquals("x", binding.entryToObject(buffer));
        assertEquals(1000, binding.bufSize);
    }

    private static class CaptureSizeBinding extends SerialBinding {

        int bufSize;

        CaptureSizeBinding(ClassCatalog classCatalog, Class baseClass) {
            super(classCatalog, baseClass);
        }

        @Override
        public FastOutputStream getSerialOutput(Object object) {
            FastOutputStream fos = super.getSerialOutput(object);
            bufSize = fos.getBufferBytes().length;
            return fos;
        }
    }

    public void testBufferOverride() {

        FastOutputStream out = new FastOutputStream(10);
        CachedOutputBinding binding =
            new CachedOutputBinding(catalog, String.class, out);

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

    private static class CachedOutputBinding extends SerialBinding {

        FastOutputStream out;
        boolean used;

        CachedOutputBinding(ClassCatalog classCatalog,
                            Class baseClass,
                            FastOutputStream out) {
            super(classCatalog, baseClass);
            this.out = out;
        }

        @Override
        public FastOutputStream getSerialOutput(Object object) {
            out.reset();
            used = true;
            return out;
        }
    }

    private static class MySerialSerialBinding extends SerialSerialBinding {

        private MySerialSerialBinding(SerialBinding keyBinding,
                                      SerialBinding valueBinding) {

            super(keyBinding, valueBinding);
        }

        @Override
        public Object entryToObject(Object keyInput, Object valueInput) {

            return "" + keyInput + '#' + valueInput;
        }

        @Override
        public Object objectToKey(Object object) {

            String s = (String) object;
            int i = s.indexOf('#');
            if (i < 0 || i == s.length() - 1) {
                throw new IllegalArgumentException(s);
            } else {
                return s.substring(0, i);
            }
        }

        @Override
        public Object objectToData(Object object) {

            String s = (String) object;
            int i = s.indexOf('#');
            if (i < 0 || i == s.length() - 1) {
                throw new IllegalArgumentException(s);
            } else {
                return s.substring(i + 1);
            }
        }
    }

    /**
     * Tests that overriding SerialBinding.getClassLoader is possible.  This is
     * a crude test because to create a truly working class loader is a large
     * undertaking.
     */
    public void testClassloaderOverride() {
        DatabaseEntry entry = new DatabaseEntry();

        SerialBinding binding = new CustomLoaderBinding
            (catalog, null, new FailureClassLoader());

        try {
            binding.objectToEntry(new MyClass(), entry);
            binding.entryToObject(entry);
            fail();
        } catch (RuntimeException e) {
            assertTrue(e.getMessage().startsWith("expect failure"));
        }
    }

    private static class CustomLoaderBinding extends SerialBinding {

        private final ClassLoader loader;

        CustomLoaderBinding(ClassCatalog classCatalog,
                            Class baseClass,
                            ClassLoader loader) {

            super(classCatalog, baseClass);
            this.loader = loader;
        }

        @Override
        public ClassLoader getClassLoader() {
            return loader;
        }
    }

    private static class FailureClassLoader extends ClassLoader {

        @Override
        public Class loadClass(String name) {
            throw new RuntimeException("expect failure: " + name);
        }
    }

    @SuppressWarnings("serial")
    private static class MyClass implements Serializable {
    }
}

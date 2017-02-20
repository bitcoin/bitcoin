/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.bind.test;

import java.io.Externalizable;
import java.io.IOException;
import java.io.ObjectInput;
import java.io.ObjectInputStream;
import java.io.ObjectOutput;
import java.io.ObjectOutputStream;
import java.io.OutputStreamWriter;
import java.io.Serializable;
import java.io.Writer;
import java.lang.reflect.Field;
import java.lang.reflect.Method;

import javax.xml.parsers.SAXParserFactory;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;

import org.xml.sax.InputSource;
import org.xml.sax.XMLReader;

import com.sleepycat.bind.serial.SerialInput;
import com.sleepycat.bind.serial.SerialOutput;
import com.sleepycat.bind.serial.test.TestClassCatalog;
import com.sleepycat.bind.tuple.TupleInput;
import com.sleepycat.bind.tuple.TupleOutput;
import com.sleepycat.util.FastInputStream;
import com.sleepycat.util.FastOutputStream;
import com.sleepycat.util.test.SharedTestUtils;

/**
 * @author Mark Hayes
 */
public class BindingSpeedTest extends TestCase {

    static final String JAVA_UNSHARED = "java-unshared".intern();
    static final String JAVA_SHARED = "java-shared".intern();
    static final String JAVA_EXTERNALIZABLE = "java-externalizable".intern();
    static final String XML_SAX = "xml-sax".intern();
    static final String TUPLE = "tuple".intern();
    static final String REFLECT_METHOD = "reflectMethod".intern();
    static final String REFLECT_FIELD = "reflectField".intern();

    static final int RUN_COUNT = 1000;
    static final boolean VERBOSE = false;

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

        TestSuite suite = new TestSuite();
        suite.addTest(new BindingSpeedTest(JAVA_UNSHARED));
        suite.addTest(new BindingSpeedTest(JAVA_SHARED));
        suite.addTest(new BindingSpeedTest(JAVA_EXTERNALIZABLE));
        suite.addTest(new BindingSpeedTest(XML_SAX));
        suite.addTest(new BindingSpeedTest(TUPLE));
        suite.addTest(new BindingSpeedTest(REFLECT_METHOD));
        suite.addTest(new BindingSpeedTest(REFLECT_FIELD));
        return suite;
    }

    private String command;
    private FastOutputStream fo;
    private TupleOutput to;
    private TestClassCatalog jtc;
    private byte[] buf;
    private XMLReader parser;
    private Method[] getters;
    private Method[] setters;
    private Field[] fields;

    public BindingSpeedTest(String name) {

        super("BindingSpeedTest." + name);
        command = name;
    }

    @Override
    public void runTest()
        throws Exception {

        SharedTestUtils.printTestName(getName());

        boolean isTuple = false;
        boolean isReflectMethod = false;
        boolean isReflectField = false;
        boolean isXmlSax = false;
        boolean isSerial = false;
        boolean isShared = false;
        boolean isExternalizable = false;

        if (command == TUPLE) {
            isTuple = true;
        } else if (command == REFLECT_METHOD) {
            isReflectMethod = true;
        } else if (command == REFLECT_FIELD) {
            isReflectField = true;
        } else if (command == XML_SAX) {
            isXmlSax = true;
        } else if (command == JAVA_UNSHARED) {
            isSerial = true;
        } else if (command == JAVA_SHARED) {
            isSerial = true;
            isShared = true;
        } else if (command == JAVA_EXTERNALIZABLE) {
            isSerial = true;
            isShared = true;
            isExternalizable = true;
        } else {
            throw new Exception("invalid command: " + command);
        }

        // Do initialization

        if (isTuple) {
            initTuple();
        } else if (isReflectMethod) {
            initReflectMethod();
        } else if (isReflectField) {
            initReflectField();
        } else if (isXmlSax) {
            initXmlSax();
        } else if (isSerial) {
            if (isShared) {
                initSerialShared();
            } else {
                initSerialUnshared();
            }
        }

        // Prime the Java compiler

        int size = 0;
        for (int i = 0; i < RUN_COUNT; i += 1) {

            if (isTuple) {
                size = runTuple();
            } else if (isReflectMethod) {
                size = runReflectMethod();
            } else if (isReflectField) {
                size = runReflectField();
            } else if (isXmlSax) {
                size = runXmlSax();
            } else if (isSerial) {
                if (isShared) {
                    if (isExternalizable) {
                        size = runSerialExternalizable();
                    } else {
                        size = runSerialShared();
                    }
                } else {
                    size = runSerialUnshared();
                }
            }
        }

        // Then run the timing tests

        long startTime = System.currentTimeMillis();

        for (int i = 0; i < RUN_COUNT; i += 1) {
            if (isTuple) {
                size = runTuple();
            } else if (isReflectMethod) {
                size = runReflectMethod();
            } else if (isReflectField) {
                size = runReflectField();
            } else if (isXmlSax) {
                size = runXmlSax();
            } else if (isSerial) {
                if (isShared) {
                    if (isExternalizable) {
                        size = runSerialExternalizable();
                    } else {
                        size = runSerialShared();
                    }
                } else {
                    size = runSerialUnshared();
                }
            }
        }

        long stopTime = System.currentTimeMillis();

	assertTrue("data size too big", size < 250);

        if (VERBOSE) {
            System.out.println(command);
            System.out.println("data size: " + size);
            System.out.println("run time:  " +
                ((stopTime - startTime) / (double) RUN_COUNT));
        }
    }

    @Override
    public void tearDown() {

        /* Ensure that GC can cleanup. */
        command = null;
        fo = null;
        to = null;
        jtc = null;
        buf = null;
        parser = null;
    }

    void initSerialUnshared() {
        fo = new FastOutputStream();
    }

    int runSerialUnshared()
        throws Exception {

        fo.reset();
        ObjectOutputStream oos = new ObjectOutputStream(fo);
        oos.writeObject(new Data());
        byte[] bytes = fo.toByteArray();
        FastInputStream fi = new FastInputStream(bytes);
        ObjectInputStream ois = new ObjectInputStream(fi);
        ois.readObject();
        return bytes.length;
    }

    void initSerialShared() {
        jtc = new TestClassCatalog();
        fo = new FastOutputStream();
    }

    int runSerialShared()
        throws Exception {

        fo.reset();
        SerialOutput oos = new SerialOutput(fo, jtc);
        oos.writeObject(new Data());
        byte[] bytes = fo.toByteArray();
        FastInputStream fi = new FastInputStream(bytes);
        SerialInput ois = new SerialInput(fi, jtc);
        ois.readObject();
        return (bytes.length - SerialOutput.getStreamHeader().length);
    }

    int runSerialExternalizable()
        throws Exception {

        fo.reset();
        SerialOutput oos = new SerialOutput(fo, jtc);
        oos.writeObject(new Data2());
        byte[] bytes = fo.toByteArray();
        FastInputStream fi = new FastInputStream(bytes);
        SerialInput ois = new SerialInput(fi, jtc);
        ois.readObject();
        return (bytes.length - SerialOutput.getStreamHeader().length);
    }

    void initTuple() {
        buf = new byte[500];
        to = new TupleOutput(buf);
    }

    int runTuple() {
        to.reset();
        new Data().writeTuple(to);

        TupleInput ti = new TupleInput(
                          to.getBufferBytes(), to.getBufferOffset(),
                          to.getBufferLength());
        new Data().readTuple(ti);

        return to.getBufferLength();
    }

    void initReflectMethod()
        throws Exception {

        initTuple();

        Class cls = Data.class;

        getters = new Method[5];
        getters[0] = cls.getMethod("getField1", new Class[0]);
        getters[1] = cls.getMethod("getField2", new Class[0]);
        getters[2] = cls.getMethod("getField3", new Class[0]);
        getters[3] = cls.getMethod("getField4", new Class[0]);
        getters[4] = cls.getMethod("getField5", new Class[0]);

        setters = new Method[5];
        setters[0] = cls.getMethod("setField1", new Class[] {String.class});
        setters[1] = cls.getMethod("setField2", new Class[] {String.class});
        setters[2] = cls.getMethod("setField3", new Class[] {Integer.TYPE});
        setters[3] = cls.getMethod("setField4", new Class[] {Integer.TYPE});
        setters[4] = cls.getMethod("setField5", new Class[] {String.class});
    }

    int runReflectMethod()
        throws Exception {

        to.reset();
        Data data = new Data();
        to.writeString((String) getters[0].invoke(data, (Object[]) null));
        to.writeString((String) getters[1].invoke(data, (Object[]) null));
        to.writeInt(((Integer) getters[2].invoke(data, (Object[]) null)).intValue());
        to.writeInt(((Integer) getters[3].invoke(data, (Object[]) null)).intValue());
        to.writeString((String) getters[4].invoke(data, (Object[]) null));

        TupleInput ti = new TupleInput(
                          to.getBufferBytes(), to.getBufferOffset(),
                          to.getBufferLength());
        data = new Data();
        setters[0].invoke(data, new Object[] {ti.readString()});
        setters[1].invoke(data, new Object[] {ti.readString()});
        setters[2].invoke(data, new Object[] {new Integer(ti.readInt())});
        setters[3].invoke(data, new Object[] {new Integer(ti.readInt())});
        setters[4].invoke(data, new Object[] {ti.readString()});

        return to.getBufferLength();
    }

    void initReflectField()
        throws Exception {

        initTuple();

        Class cls = Data.class;

        fields = new Field[5];
        fields[0] = cls.getField("field1");
        fields[1] = cls.getField("field2");
        fields[2] = cls.getField("field3");
        fields[3] = cls.getField("field4");
        fields[4] = cls.getField("field5");
    }

    int runReflectField()
        throws Exception {

        to.reset();
        Data data = new Data();
        to.writeString((String) fields[0].get(data));
        to.writeString((String) fields[1].get(data));
        to.writeInt(((Integer) fields[2].get(data)).intValue());
        to.writeInt(((Integer) fields[3].get(data)).intValue());
        to.writeString((String) fields[4].get(data));

        TupleInput ti = new TupleInput(
                          to.getBufferBytes(), to.getBufferOffset(),
                          to.getBufferLength());
        data = new Data();
        fields[0].set(data, ti.readString());
        fields[1].set(data, ti.readString());
        fields[2].set(data, new Integer(ti.readInt()));
        fields[3].set(data, new Integer(ti.readInt()));
        fields[4].set(data, ti.readString());

        return to.getBufferLength();
    }

    void initXmlSax()
        throws Exception {

        buf = new byte[500];
        fo = new FastOutputStream();
        SAXParserFactory saxFactory = SAXParserFactory.newInstance();
        saxFactory.setNamespaceAware(true);
        parser = saxFactory.newSAXParser().getXMLReader();
    }

    int runXmlSax()
        throws Exception {

        fo.reset();
        OutputStreamWriter writer = new OutputStreamWriter(fo);
        new Data().writeXmlText(writer);

        byte[] bytes = fo.toByteArray();
        FastInputStream fi = new FastInputStream(bytes);
        InputSource input = new InputSource(fi);
        parser.parse(input);

        //InputStreamReader reader = new InputStreamReader(fi);
        //new Data().readXmlText(??);

        return bytes.length;
    }

    static class Data2 extends Data implements Externalizable {

        public Data2() {}

        public void readExternal(ObjectInput in)
            throws IOException {

            field1 = in.readUTF();
            field2 = in.readUTF();
            field3 = in.readInt();
            field4 = in.readInt();
            field5 = in.readUTF();
        }

        public void writeExternal(ObjectOutput out)
            throws IOException {

            out.writeUTF(field1);
            out.writeUTF(field2);
            out.writeInt(field3);
            out.writeInt(field4);
            out.writeUTF(field5);
        }
    }

    @SuppressWarnings("serial")
    static class Data implements Serializable {

        public String field1 = "field1";
        public String field2 = "field2";
        public int field3 = 333;
        public int field4 = 444;
        public String field5 = "field5";

        public String getField1() { return field1; }
        public String getField2() { return field2; }
        public int getField3() { return field3; }
        public int getField4() { return field4; }
        public String getField5() { return field5; }

        public void setField1(String v) { field1 = v; }
        public void setField2(String v) { field2 = v; }
        public void setField3(int v) { field3 = v; }
        public void setField4(int v) { field4 = v; }
        public void setField5(String v) { field5 = v; }

        void readTuple(TupleInput _input) {

            field1 = _input.readString();
            field2 = _input.readString();
            field3 = _input.readInt();
            field4 = _input.readInt();
            field5 = _input.readString();
        }

        void writeTuple(TupleOutput _output) {

            _output.writeString(field1);
            _output.writeString(field2);
            _output.writeInt(field3);
            _output.writeInt(field4);
            _output.writeString(field5);
        }

        void writeXmlText(Writer writer) throws IOException {

            writer.write("<Data><Field1>");
            writer.write(field1);
            writer.write("</Field1><Field2>");
            writer.write(field2);
            writer.write("</Field2><Field3>");
            writer.write(String.valueOf(field3));
            writer.write("</Field3><Field4>");
            writer.write(String.valueOf(field4));
            writer.write("</Field4><Field5>");
            writer.write(field5);
            writer.write("</Field5></Data>");
            writer.flush();
        }
    }
}

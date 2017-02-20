/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.bind.serial;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.ObjectOutputStream;
import java.io.ObjectStreamClass;
import java.io.ObjectStreamConstants;
import java.io.OutputStream;

import com.sleepycat.db.DatabaseException;
import com.sleepycat.util.RuntimeExceptionWrapper;

/**
 * A specialized <code>ObjectOutputStream</code> that stores class description
 * information in a <code>ClassCatalog</code>.  It is used by
 * <code>SerialBinding</code>.
 *
 * <p>This class is used instead of an {@link ObjectOutputStream}, which it
 * extends, to write a compact object stream.  For writing objects to a
 * database normally one of the serial binding classes is used.  {@link
 * SerialOutput} is used when an {@link ObjectOutputStream} is needed along
 * with compact storage.  A {@link ClassCatalog} must be supplied, however, to
 * stored shared class descriptions.</p>
 *
 * <p>The {@link ClassCatalog} is used to store class definitions rather than
 * embedding these into the stream.  Instead, a class format identifier is
 * embedded into the stream.  This identifier is then used by {@link
 * SerialInput} to load the class format to deserialize the object.</p>
 *
 * @see <a href="SerialBinding.html#evolution">Class Evolution</a>
 *
 * @author Mark Hayes
 */
public class SerialOutput extends ObjectOutputStream {

    /*
     * Serialization version constants. Instead of hardcoding these we get them
     * by creating a SerialOutput, which itself guarantees that we'll always
     * use a PROTOCOL_VERSION_2 header.
     */
    private final static byte[] STREAM_HEADER;
    static {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        try {
            new SerialOutput(baos, null);
        } catch (IOException e) {
            throw new RuntimeExceptionWrapper(e);
        }
        STREAM_HEADER = baos.toByteArray();
    }

    private ClassCatalog classCatalog;

    /**
     * Creates a serial output stream.
     *
     * @param out is the output stream to which the compact serialized objects
     * will be written.
     *
     * @param classCatalog is the catalog to which the class descriptions for
     * the serialized objects will be written.
     */
    public SerialOutput(OutputStream out, ClassCatalog classCatalog)
        throws IOException {

        super(out);
        this.classCatalog = classCatalog;

        /* guarantee that we'll always use the same serialization format */

        useProtocolVersion(ObjectStreamConstants.PROTOCOL_VERSION_2);
    }

    // javadoc is inherited
    protected void writeClassDescriptor(ObjectStreamClass classdesc)
        throws IOException {

        try {
            byte[] id = classCatalog.getClassID(classdesc);
            writeByte(id.length);
            write(id);
        } catch (DatabaseException e) {
            /*
             * Do not throw IOException from here since ObjectOutputStream
             * will write the exception to the stream, which causes another
             * call here, etc.
             */
            throw new RuntimeExceptionWrapper(e);
        } catch (ClassNotFoundException e) {
            throw new RuntimeExceptionWrapper(e);
        }
    }

    /**
     * Returns the fixed stream header used for all serialized streams in
     * PROTOCOL_VERSION_2 format.  To save space this header can be removed and
     * serialized streams before storage and inserted before deserializing.
     * {@link SerialOutput} always uses PROTOCOL_VERSION_2 serialization format
     * to guarantee that this header is fixed.  {@link SerialBinding} removes
     * this header from serialized streams automatically.
     *
     * @return the fixed stream header.
     */
    public static byte[] getStreamHeader() {

        return STREAM_HEADER;
    }
}

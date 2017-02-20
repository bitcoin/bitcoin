/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.bind.serial;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.ObjectStreamClass;
import java.io.Serializable;
import java.math.BigInteger;
import java.util.HashMap;

import com.sleepycat.compat.DbCompat;
import com.sleepycat.db.Cursor;
import com.sleepycat.db.CursorConfig;
import com.sleepycat.db.Database;
import com.sleepycat.db.DatabaseConfig;
import com.sleepycat.db.DatabaseEntry;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.EnvironmentConfig;
import com.sleepycat.db.LockMode;
import com.sleepycat.db.OperationStatus;
import com.sleepycat.db.Transaction;
import com.sleepycat.util.RuntimeExceptionWrapper;
import com.sleepycat.util.UtfOps;

/**
 * A <code>ClassCatalog</code> that is stored in a <code>Database</code>.
 *
 * <p>A single <code>StoredClassCatalog</code> object is normally used along
 * with a set of databases that stored serialized objects.</p>
 *
 * @author Mark Hayes
 *
 * @see <a href="SerialBinding.html#evolution">Class Evolution</a>
 */
public class StoredClassCatalog implements ClassCatalog {

    /*
     * Record types ([key] [data]):
     *
     * [0] [next class ID]
     * [1 / class ID] [ObjectStreamClass (class format)]
     * [2 / class name] [ClassInfo (has 8 byte class ID)]
     */
    private static final byte REC_LAST_CLASS_ID = (byte) 0;
    private static final byte REC_CLASS_FORMAT = (byte) 1;
    private static final byte REC_CLASS_INFO = (byte) 2;

    private static final byte[] LAST_CLASS_ID_KEY = {REC_LAST_CLASS_ID};

    private Database db;
    private HashMap<String, ClassInfo> classMap;
    private HashMap<BigInteger, ObjectStreamClass> formatMap;
    private LockMode writeLockMode;
    private boolean cdbMode;
    private boolean txnMode;

    /**
     * Creates a catalog based on a given database. To save resources, only a
     * single catalog object should be used for each unique catalog database.
     *
     * @param database an open database to use as the class catalog.  It must
     * be a BTREE database and must not allow duplicates.
     *
     * @throws DatabaseException if an error occurs accessing the database.
     *
     * @throws IllegalArgumentException if the database is not a BTREE database
     * or if it configured to allow duplicates.
     */
    public StoredClassCatalog(Database database)
        throws DatabaseException, IllegalArgumentException {

        db = database;
        DatabaseConfig dbConfig = db.getConfig();
        EnvironmentConfig envConfig = db.getEnvironment().getConfig();

        writeLockMode = (DbCompat.getInitializeLocking(envConfig) ||
                         envConfig.getTransactional()) ? LockMode.RMW
                                                       : LockMode.DEFAULT;
        cdbMode = DbCompat.getInitializeCDB(envConfig);
        txnMode = dbConfig.getTransactional();

        if (!DbCompat.isTypeBtree(dbConfig)) {
            throw new IllegalArgumentException(
                    "The class catalog must be a BTREE database.");
        }
        if (DbCompat.getSortedDuplicates(dbConfig) ||
            DbCompat.getUnsortedDuplicates(dbConfig)) {
            throw new IllegalArgumentException(
                    "The class catalog database must not allow duplicates.");
        }

        /*
         * Create the class format and class info maps. Note that these are not
         * synchronized, and therefore the methods that use them are
         * synchronized.
         */
        classMap = new HashMap<String, ClassInfo>();
        formatMap = new HashMap<BigInteger, ObjectStreamClass>();

        DatabaseEntry key = new DatabaseEntry(LAST_CLASS_ID_KEY);
        DatabaseEntry data = new DatabaseEntry();
        if (dbConfig.getReadOnly()) {
            /* Check that the class ID record exists. */
            OperationStatus status = db.get(null, key, data, null);
            if (status != OperationStatus.SUCCESS) {
                throw new IllegalStateException
                    ("A read-only catalog database may not be empty");
            }
        } else {
            /* Add the initial class ID record if it doesn't exist.  */
            data.setData(new byte[1]); // zero ID
            /* Use putNoOverwrite to avoid phantoms. */
            db.putNoOverwrite(null, key, data);
        }
    }

    // javadoc is inherited
    public synchronized void close()
        throws DatabaseException {

        if (db != null) {
            db.close();
        }
        db = null;
        formatMap = null;
        classMap = null;
    }

    // javadoc is inherited
    public synchronized byte[] getClassID(ObjectStreamClass classFormat)
        throws DatabaseException, ClassNotFoundException {

        ClassInfo classInfo = getClassInfo(classFormat);
        return classInfo.getClassID();
    }

    // javadoc is inherited
    public synchronized ObjectStreamClass getClassFormat(byte[] classID)
        throws DatabaseException, ClassNotFoundException {

        return getClassFormat(classID, new DatabaseEntry());
    }

    /**
     * Internal function for getting the class format.  Allows passing the
     * DatabaseEntry object for the data, so the bytes of the class format can
     * be examined afterwards.
     */
    private ObjectStreamClass getClassFormat(byte[] classID,
					     DatabaseEntry data)
        throws DatabaseException, ClassNotFoundException {

        /* First check the map and, if found, add class info to the map. */

        BigInteger classIDObj = new BigInteger(classID);
        ObjectStreamClass classFormat = formatMap.get(classIDObj);
        if (classFormat == null) {

            /* Make the class format key. */

            byte[] keyBytes = new byte[classID.length + 1];
            keyBytes[0] = REC_CLASS_FORMAT;
            System.arraycopy(classID, 0, keyBytes, 1, classID.length);
            DatabaseEntry key = new DatabaseEntry(keyBytes);

            /* Read the class format. */

            OperationStatus status = db.get(null, key, data, LockMode.DEFAULT);
            if (status != OperationStatus.SUCCESS) {
                throw new ClassNotFoundException("Catalog class ID not found");
            }
            try {
                ObjectInputStream ois =
                    new ObjectInputStream(
                        new ByteArrayInputStream(data.getData(),
                                                 data.getOffset(),
                                                 data.getSize()));
                classFormat = (ObjectStreamClass) ois.readObject();
            } catch (IOException e) {
                throw new RuntimeExceptionWrapper(e);
            }

            /* Update the class format map. */

            formatMap.put(classIDObj, classFormat);
        }
        return classFormat;
    }

    /**
     * Get the ClassInfo for a given class name, adding it and its
     * ObjectStreamClass to the database if they are not already present, and
     * caching both of them using the class info and class format maps.  When a
     * class is first loaded from the database, the stored ObjectStreamClass is
     * compared to the current ObjectStreamClass loaded by the Java class
     * loader; if they are different, a new class ID is assigned for the
     * current format.
     */
    private ClassInfo getClassInfo(ObjectStreamClass classFormat)
        throws DatabaseException, ClassNotFoundException {

        /*
         * First check for a cached copy of the class info, which if
         * present always contains the class format object
         */
        String className = classFormat.getName();
        ClassInfo classInfo = classMap.get(className);
        if (classInfo != null) {
            return classInfo;
        } else {
            /* Make class info key.  */
            char[] nameChars = className.toCharArray();
            byte[] keyBytes = new byte[1 + UtfOps.getByteLength(nameChars)];
            keyBytes[0] = REC_CLASS_INFO;
            UtfOps.charsToBytes(nameChars, 0, keyBytes, 1, nameChars.length);
            DatabaseEntry key = new DatabaseEntry(keyBytes);

            /* Read class info.  */
            DatabaseEntry data = new DatabaseEntry();
            OperationStatus status = db.get(null, key, data, LockMode.DEFAULT);
            if (status != OperationStatus.SUCCESS) {
                /*
                 * Not found in the database, write class info and class
                 * format.
                 */
                classInfo = putClassInfo(new ClassInfo(), className, key,
                                         classFormat);
            } else {
                /*
                 * Read class info to get the class format key, then read class
                 * format.
                 */
                classInfo = new ClassInfo(data);
                DatabaseEntry formatData = new DatabaseEntry();
                ObjectStreamClass storedClassFormat =
                    getClassFormat(classInfo.getClassID(), formatData);

                /*
                 * Compare the stored class format to the current class format,
                 * and if they are different then generate a new class ID.
                 */
                if (!areClassFormatsEqual(storedClassFormat,
                                          getBytes(formatData),
                                          classFormat)) {
                    classInfo = putClassInfo(classInfo, className, key,
                                             classFormat);
                }

                /* Update the class info map.  */
                classInfo.setClassFormat(classFormat);
                classMap.put(className, classInfo);
            }
        }
        return classInfo;
    }

    /**
     * Assign a new class ID (increment the current ID record), write the
     * ObjectStreamClass record for this new ID, and update the ClassInfo
     * record with the new ID also.  The ClassInfo passed as an argument is the
     * one to be updated.
     */
    private ClassInfo putClassInfo(ClassInfo classInfo,
				   String className,
				   DatabaseEntry classKey,
				   ObjectStreamClass classFormat)
        throws DatabaseException {

        /* An intent-to-write cursor is needed for CDB. */
        CursorConfig cursorConfig = null;
        if (cdbMode) {
            cursorConfig = new CursorConfig();
            DbCompat.setWriteCursor(cursorConfig, true);
        }
        Cursor cursor = null;
        Transaction txn = null;
        try {
            if (txnMode) {
                txn = db.getEnvironment().beginTransaction(null, null);
            }
            cursor = db.openCursor(txn, cursorConfig);

            /* Get the current class ID. */
            DatabaseEntry key = new DatabaseEntry(LAST_CLASS_ID_KEY);
            DatabaseEntry data = new DatabaseEntry();
            OperationStatus status = cursor.getSearchKey(key, data,
                                                         writeLockMode);
            if (status != OperationStatus.SUCCESS) {
                throw new IllegalStateException("Class ID not initialized");
            }
            byte[] idBytes = getBytes(data);

            /* Increment the ID by one and write the updated record.  */
            idBytes = incrementID(idBytes);
            data.setData(idBytes);
            cursor.put(key, data);

            /*
             * Write the new class format record whose key is the ID just
             * assigned.
             */
            byte[] keyBytes = new byte[1 + idBytes.length];
            keyBytes[0] = REC_CLASS_FORMAT;
            System.arraycopy(idBytes, 0, keyBytes, 1, idBytes.length);
            key.setData(keyBytes);

            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            ObjectOutputStream oos;
            try {
                oos = new ObjectOutputStream(baos);
                oos.writeObject(classFormat);
            } catch (IOException e) {
                throw new RuntimeExceptionWrapper(e);
            }
            data.setData(baos.toByteArray());

            cursor.put(key, data);

            /*
             * Write the new class info record, using the key passed in; this
             * is done last so that a reader who gets the class info record
             * first will always find the corresponding class format record.
             */
            classInfo.setClassID(idBytes);
            classInfo.toDbt(data);

            cursor.put(classKey, data);

            /*
             * Update the maps before closing the cursor, so that the cursor
             * lock prevents other writers from duplicating this entry.
             */
            classInfo.setClassFormat(classFormat);
            classMap.put(className, classInfo);
            formatMap.put(new BigInteger(idBytes), classFormat);
            return classInfo;
        } finally {
            if (cursor != null) {
                cursor.close();
            }
            if (txn != null) {
                txn.commit();
            }
        }
    }

    private static byte[] incrementID(byte[] key) {

        BigInteger id = new BigInteger(key);
        id = id.add(BigInteger.valueOf(1));
        return id.toByteArray();
    }

    /**
     * Holds the class format key for a class, maintains a reference to the
     * ObjectStreamClass.  Other fields can be added when we need to store more
     * information per class.
     */
    private static class ClassInfo implements Serializable {
        static final long serialVersionUID = 3845446969989650562L;

        private byte[] classID;
        private transient ObjectStreamClass classFormat;

        ClassInfo() {
        }

        ClassInfo(DatabaseEntry dbt) {

            byte[] data = dbt.getData();
            int len = data[0];
            classID = new byte[len];
            System.arraycopy(data, 1, classID, 0, len);
        }

        void toDbt(DatabaseEntry dbt) {

            byte[] data = new byte[1 + classID.length];
            data[0] = (byte) classID.length;
            System.arraycopy(classID, 0, data, 1, classID.length);
            dbt.setData(data);
        }

        void setClassID(byte[] classID) {

            this.classID = classID;
        }

        byte[] getClassID() {

            return classID;
        }

        ObjectStreamClass getClassFormat() {

            return classFormat;
        }

        void setClassFormat(ObjectStreamClass classFormat) {

            this.classFormat = classFormat;
        }
    }

    /**
     * Return whether two class formats are equal.  This determines whether a
     * new class format is needed for an object being serialized.  Formats must
     * be identical in all respects, or a new format is needed.
     */
    private static boolean areClassFormatsEqual(ObjectStreamClass format1,
                                                byte[] format1Bytes,
                                                ObjectStreamClass format2) {
        try {
            if (format1Bytes == null) { // using cached format1 object
                format1Bytes = getObjectBytes(format1);
            }
            byte[] format2Bytes = getObjectBytes(format2);
            return java.util.Arrays.equals(format2Bytes, format1Bytes);
        } catch (IOException e) { return false; }
    }

    /*
     * We can return the same byte[] for 0 length arrays.
     */
    private static byte[] ZERO_LENGTH_BYTE_ARRAY = new byte[0];

    private static byte[] getBytes(DatabaseEntry dbt) {
        byte[] b = dbt.getData();
        if (b == null) {
            return null;
        }
        if (dbt.getOffset() == 0 && b.length == dbt.getSize()) {
            return b;
        }
	int len = dbt.getSize();
	if (len == 0) {
	    return ZERO_LENGTH_BYTE_ARRAY;
	} else {
	    byte[] t = new byte[len];
	    System.arraycopy(b, dbt.getOffset(), t, 0, t.length);
	    return t;
	}
    }

    private static byte[] getObjectBytes(Object o)
        throws IOException {

        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        ObjectOutputStream oos = new ObjectOutputStream(baos);
        oos.writeObject(o);
        return baos.toByteArray();
    }
}

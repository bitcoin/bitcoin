/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.bind.tuple.test;

import com.sleepycat.bind.tuple.MarshalledTupleEntry;
import com.sleepycat.bind.tuple.MarshalledTupleKeyEntity;
import com.sleepycat.bind.tuple.TupleInput;
import com.sleepycat.bind.tuple.TupleOutput;

/**
 * @author Mark Hayes
 */
public class MarshalledObject
    implements MarshalledTupleEntry, MarshalledTupleKeyEntity {

    private String data;
    private String primaryKey;
    private String indexKey1;
    private String indexKey2;

    public MarshalledObject() {
    }

    MarshalledObject(String data, String primaryKey,
                     String indexKey1, String indexKey2) {

        this.data = data;
        this.primaryKey = primaryKey;
        this.indexKey1 = indexKey1;
        this.indexKey2 = indexKey2;
    }

    String getData() {

        return data;
    }

    String getPrimaryKey() {

        return primaryKey;
    }

    String getIndexKey1() {

        return indexKey1;
    }

    String getIndexKey2() {

        return indexKey2;
    }

    int expectedDataLength() {

        return data.length() + 1 +
               indexKey1.length() + 1 +
               indexKey2.length() + 1;
    }

    int expectedKeyLength() {

        return primaryKey.length() + 1;
    }

    public void marshalEntry(TupleOutput dataOutput) {

        dataOutput.writeString(data);
        dataOutput.writeString(indexKey1);
        dataOutput.writeString(indexKey2);
    }

    public void unmarshalEntry(TupleInput dataInput) {

        data = dataInput.readString();
        indexKey1 = dataInput.readString();
        indexKey2 = dataInput.readString();
    }

    public void marshalPrimaryKey(TupleOutput keyOutput) {

        keyOutput.writeString(primaryKey);
    }

    public void unmarshalPrimaryKey(TupleInput keyInput) {

        primaryKey = keyInput.readString();
    }

    public boolean marshalSecondaryKey(String keyName, TupleOutput keyOutput) {

        if ("1".equals(keyName)) {
            if (indexKey1.length() > 0) {
                keyOutput.writeString(indexKey1);
                return true;
            } else {
                return false;
            }
        } else if ("2".equals(keyName)) {
            if (indexKey1.length() > 0) {
                keyOutput.writeString(indexKey2);
                return true;
            } else {
                return false;
            }
        } else {
            throw new IllegalArgumentException("Unknown keyName: " + keyName);
        }
    }

    public boolean nullifyForeignKey(String keyName) {

        if ("1".equals(keyName)) {
            if (indexKey1.length() > 0) {
                indexKey1 = "";
                return true;
            } else {
                return false;
            }
        } else if ("2".equals(keyName)) {
            if (indexKey1.length() > 0) {
                indexKey2 = "";
                return true;
            } else {
                return false;
            }
        } else {
            throw new IllegalArgumentException("Unknown keyName: " + keyName);
        }
    }
}

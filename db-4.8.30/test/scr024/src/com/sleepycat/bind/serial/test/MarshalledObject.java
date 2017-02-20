/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.bind.serial.test;

import java.io.Serializable;

import com.sleepycat.bind.tuple.MarshalledTupleKeyEntity;
import com.sleepycat.bind.tuple.TupleInput;
import com.sleepycat.bind.tuple.TupleOutput;

/**
 * @author Mark Hayes
 */
@SuppressWarnings("serial")
public class MarshalledObject
    implements Serializable, MarshalledTupleKeyEntity {

    private String data;
    private transient String primaryKey;
    private String indexKey1;
    private String indexKey2;

    public MarshalledObject(String data, String primaryKey,
                            String indexKey1, String indexKey2) {
        this.data = data;
        this.primaryKey = primaryKey;
        this.indexKey1 = indexKey1;
        this.indexKey2 = indexKey2;
    }

    public boolean equals(Object o) {

        try {
            MarshalledObject other = (MarshalledObject) o;

            return this.data.equals(other.data) &&
                   this.primaryKey.equals(other.primaryKey) &&
                   this.indexKey1.equals(other.indexKey1) &&
                   this.indexKey2.equals(other.indexKey2);
        } catch (Exception e) {
            return false;
        }
    }

    public String getData() {

        return data;
    }

    public String getPrimaryKey() {

        return primaryKey;
    }

    public String getIndexKey1() {

        return indexKey1;
    }

    public String getIndexKey2() {

        return indexKey2;
    }

    public int expectedKeyLength() {

        return primaryKey.length() + 1;
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
            if (indexKey2.length() > 0) {
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
            if (indexKey2.length() > 0) {
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

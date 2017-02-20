/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.test;

import com.sleepycat.persist.impl.EnhancedAccessor;
import com.sleepycat.persist.impl.EntityInput;
import com.sleepycat.persist.impl.EntityOutput;
import com.sleepycat.persist.impl.Format;
import com.sleepycat.persist.model.Persistent;

/**
 * For running ASMifier -- entity sublcass.
 */
@Persistent
class Enhanced2 extends Enhanced1 {

    static {
        EnhancedAccessor.registerClass(null, new Enhanced2());
    }

    public Object bdbNewInstance() {
        return new Enhanced2();
    }

    public Object bdbNewArray(int len) {
        return new Enhanced2[len];
    }

    public boolean bdbIsPriKeyFieldNullOrZero() {
        return super.bdbIsPriKeyFieldNullOrZero();
    }

    public void bdbWritePriKeyField(EntityOutput output, Format format) {
        super.bdbWritePriKeyField(output, format);
    }

    public void bdbReadPriKeyField(EntityInput input, Format format) {
        super.bdbReadPriKeyField(input, format);
    }

    public void bdbWriteSecKeyFields(EntityOutput output) {
        super.bdbWriteSecKeyFields(output);
    }

    public void bdbReadSecKeyFields(EntityInput input,
                                    int startField,
                                    int endField,
                                    int superLevel) {
        if (superLevel != 0) {
            super.bdbReadSecKeyFields
                (input, startField, endField, superLevel - 1);
        }
    }

    public void bdbWriteNonKeyFields(EntityOutput output) {
        super.bdbWriteNonKeyFields(output);
    }

    public void bdbReadNonKeyFields(EntityInput input,
                                    int startField,
                                    int endField,
                                    int superLevel) {
        if (superLevel != 0) {
            super.bdbReadNonKeyFields
                (input, startField, endField, superLevel - 1);
        }
    }

    public boolean bdbNullifyKeyField(Object o,
                                      int field,
                                      int superLevel,
                                      boolean isSecField,
                                      Object keyElement) {
        if (superLevel > 0) {
            return super.bdbNullifyKeyField
                (o, field, superLevel - 1, isSecField, keyElement);
        } else {
            return false;
        }
    }

    public Object bdbGetField(Object o,
                              int field,
                              int superLevel,
                              boolean isSecField) {
        if (superLevel > 0) {
            return super.bdbGetField
                (o, field, superLevel - 1, isSecField);
        } else {
            return null;
        }
    }

    public void bdbSetField(Object o,
                            int field,
                            int superLevel,
                            boolean isSecField,
                            Object value) {
        if (superLevel > 0) {
            super.bdbSetField
                (o, field, superLevel - 1, isSecField, value);
        }
    }
}

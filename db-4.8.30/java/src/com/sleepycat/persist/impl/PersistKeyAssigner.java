/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.impl;

import com.sleepycat.bind.tuple.TupleBase;
import com.sleepycat.db.DatabaseEntry;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.Sequence;

/**
 * Assigns primary keys from a Sequence.
 *
 * This class is used directly by PrimaryIndex, not via an interface.  To avoid
 * making a public interface, the PersistEntityBinding contains a reference to
 * a PersistKeyAssigner, and the PrimaryIndex gets the key assigner from the
 * binding.  See the PrimaryIndex constructor for more information.
 *
 * @author Mark Hayes
 */
public class PersistKeyAssigner {

    private Catalog catalog;
    private Format keyFieldFormat;
    private Format entityFormat;
    private boolean rawAccess;
    private Sequence sequence;

    PersistKeyAssigner(PersistKeyBinding keyBinding,
                       PersistEntityBinding entityBinding,
                       Sequence sequence) {
        catalog = keyBinding.catalog;
        /* getSequenceKeyFormat will validate the field type for a sequence. */
        keyFieldFormat = keyBinding.keyFormat.getSequenceKeyFormat();
        entityFormat = entityBinding.entityFormat;
        rawAccess = entityBinding.rawAccess;
        this.sequence = sequence;
    }

    public boolean assignPrimaryKey(Object entity, DatabaseEntry key)
        throws DatabaseException {
            
        /*
         * The keyFieldFormat is the format of a simple integer field.  For a
         * composite key class it is the contained integer field.  By writing
         * the Long sequence value using that format, the output data can then
         * be read to construct the actual key instance, whether it is a simple
         * or composite key class, and assign it to the primary key field in
         * the entity object.
         */
        if (entityFormat.isPriKeyNullOrZero(entity, rawAccess)) {
            Long value = sequence.get(null, 1);
            RecordOutput output = new RecordOutput(catalog, rawAccess);
            keyFieldFormat.writeObject(value, output, rawAccess);
            TupleBase.outputToEntry(output, key);
            EntityInput input = new RecordInput
                (catalog, rawAccess, null, 0,
                 key.getData(), key.getOffset(), key.getSize());
            entityFormat.getReader().readPriKey(entity, input, rawAccess);
            return true;
        } else {
            return false;
        }
    }
}

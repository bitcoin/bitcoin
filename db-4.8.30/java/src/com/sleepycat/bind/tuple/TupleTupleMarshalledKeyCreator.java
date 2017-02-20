/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.bind.tuple;

/**
 * A concrete key creator that works in conjunction with a {@link
 * TupleTupleMarshalledBinding}.  This key creator works by calling the
 * methods of the {@link MarshalledTupleKeyEntity} interface to create and
 * clear the index key.
 *
 * <p>Note that a marshalled tuple key creator is somewhat less efficient
 * than a non-marshalled key tuple creator because more conversions are
 * needed.  A marshalled key creator must convert the entry to an object in
 * order to create the key, while an unmarshalled key creator does not.</p>
 *
 * @author Mark Hayes
 */
public class TupleTupleMarshalledKeyCreator<E extends
    MarshalledTupleEntry & MarshalledTupleKeyEntity>
    extends TupleTupleKeyCreator<E> {

    private String keyName;
    private TupleTupleMarshalledBinding<E> binding;

    /**
     * Creates a tuple-tuple marshalled key creator.
     *
     * @param binding is the binding used for the tuple-tuple entity.
     *
     * @param keyName is the key name passed to the {@link
     * MarshalledTupleKeyEntity#marshalSecondaryKey} method to identify the
     * index key.
     */
    public TupleTupleMarshalledKeyCreator(TupleTupleMarshalledBinding<E>
                                          binding,
                                          String keyName) {

        this.binding = binding;
        this.keyName = keyName;
    }

    // javadoc is inherited
    public boolean createSecondaryKey(TupleInput primaryKeyInput,
                                      TupleInput dataInput,
                                      TupleOutput indexKeyOutput) {

        /* The primary key is unmarshalled before marshalling the index key, to
         * account for cases where the index key includes fields taken from the
         * primary key.
         */
        E entity = binding.entryToObject(primaryKeyInput, dataInput);
        return entity.marshalSecondaryKey(keyName, indexKeyOutput);
    }

    // javadoc is inherited
    public boolean nullifyForeignKey(TupleInput dataInput,
                                     TupleOutput dataOutput) {

        E entity = binding.entryToObject(null, dataInput);
        if (entity.nullifyForeignKey(keyName)) {
            binding.objectToData(entity, dataOutput);
            return true;
        } else {
            return false;
        }
    }
}

/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.bind.tuple;

import com.sleepycat.bind.EntryBinding;
import com.sleepycat.db.DatabaseEntry;

/**
 * A concrete <code>EntryBinding</code> that uses the <code>TupleInput</code>
 * object as the key or data object.
 *
 * A concrete tuple binding for key or data entries which are {@link
 * TupleInput} objects.  This binding is used when tuples themselves are the
 * objects, rather than using application defined objects. A {@link TupleInput}
 * must always be used.  To convert a {@link TupleOutput} to a {@link
 * TupleInput}, use the {@link TupleInput#TupleInput(TupleOutput)} constructor.
 *
 * @author Mark Hayes
 */
public class TupleInputBinding implements EntryBinding<TupleInput> {

    /**
     * Creates a tuple input binding.
     */
    public TupleInputBinding() {
    }

    // javadoc is inherited
    public TupleInput entryToObject(DatabaseEntry entry) {

        return TupleBinding.entryToInput(entry);
    }

    // javadoc is inherited
    public void objectToEntry(TupleInput object, DatabaseEntry entry) {

        TupleBinding.inputToEntry(object, entry);
    }
}

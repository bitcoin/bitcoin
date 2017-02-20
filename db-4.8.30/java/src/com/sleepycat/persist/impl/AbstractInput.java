/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.impl;

/**
 * Base class for EntityInput implementations.  RecordInput cannot use this
 * base class because it extends TupleInput, so it repeats the code here.
 *
 * @author Mark Hayes
 */
abstract class AbstractInput implements EntityInput {

    Catalog catalog;
    boolean rawAccess;

    AbstractInput(Catalog catalog, boolean rawAccess) {
        this.catalog = catalog;
        this.rawAccess = rawAccess;
    }

    public Catalog getCatalog() {
        return catalog;
    }

    public boolean isRawAccess() {
        return rawAccess;
    }

    public boolean setRawAccess(boolean rawAccessParam) {
        boolean original = rawAccess;
        rawAccess = rawAccessParam;
        return original;
    }
}

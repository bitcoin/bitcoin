/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.impl;

import java.util.IdentityHashMap;
import java.util.Map;

import com.sleepycat.persist.raw.RawObject;

/**
 * Read-only catalog used by a PersistComparator to return simple formats plus
 * reconstituted enum formats.
 *
 * @author Mark Hayes
 */
class ComparatorCatalog implements Catalog {

    private final Map<String, Format> formatMap;
    private final Catalog simpleCatalog = SimpleCatalog.getInstance();

    ComparatorCatalog(final Map<String, Format> formatMap) {
        this.formatMap = formatMap;
    }

    public int getInitVersion(final Format format, final boolean forReader) {
        return simpleCatalog.getInitVersion(format, forReader);
    }

    public Format getFormat(final int formatId) {
        return simpleCatalog.getFormat(formatId);
    }

    public Format getFormat(final Class cls,
                            final boolean checkEntitySubclassIndexes) {
        return simpleCatalog.getFormat(cls, checkEntitySubclassIndexes);
    }

    public Format getFormat(final String className) {
        if (formatMap != null) {
            final Format f = formatMap.get(className);
            if (f != null) {
                return f;
            }
        }
        return simpleCatalog.getFormat(className);
    }

    public Format createFormat(final String clsName,
                               final Map<String, Format> newFormats) {
        return simpleCatalog.createFormat(clsName, newFormats);
    }

    public Format createFormat(final Class type,
                               final Map<String, Format> newFormats) {
        return simpleCatalog.createFormat(type, newFormats);
    }

    public boolean isRawAccess() {
        return simpleCatalog.isRawAccess();
    }

    public Object convertRawObject(final RawObject o,
                                   final IdentityHashMap converted) {
        return simpleCatalog.convertRawObject(o, converted);
    }
}

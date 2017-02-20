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
 * Catalog operation interface used by format classes.
 *
 * @see PersistCatalog
 * @see SimpleCatalog
 * @see ReadOnlyCatalog
 *
 * @author Mark Hayes
 */
interface Catalog {

    /*
     * The catalog version is returned by getInitVersion and is the version of
     * the serialized format classes loaded from the stored catalog.  When a
     * field is added, for example, the version can be checked to determine how
     * to initialize the field in Format.initialize.
     *
     * -1: The version is considered to be -1 when reading the beta version of
     * the catalog data.  At this point no version field was stored, but we can
     * distinguish the beta stored format.  See PersistCatalog.
     *
     * 0: The first released version of the catalog data, after beta.  At this
     * point no version field was stored, but it is initialized to zero when
     * the PersistCatalog.Data object is de-serialized.
     *
     * 1: Add the ComplexFormat.ConvertFieldReader.oldFieldNum field. [#15797]
     */
    static final int BETA_VERSION = -1;
    static final int CURRENT_VERSION = 1;

    /**
     * See above.
     */
    int getInitVersion(Format format, boolean forReader);

    /**
     * Returns a format for a given ID, or throws an exception.  This method is
     * used when reading an object from the byte array format.
     *
     * @throws IllegalStateException if the formatId does not correspond to a
     * persistent class.  This is an internal consistency error.
     */
    Format getFormat(int formatId);

    /**
     * Returns a format for a given class, or throws an exception.  This method
     * is used when writing an object that was passed in by the user.
     *
     * @param checkEntitySubclassIndexes is true if we're expecting this format
     * to be an entity subclass and therefore subclass secondary indexes should
     * be opened.
     *
     * @throws IllegalArgumentException if the class is not persistent.  This
     * is a user error.
     */
    Format getFormat(Class cls, boolean checkEntitySubclassIndexes);

    /**
     * Returns a format by class name.  Unlike {@link #getFormat(Class)}, the
     * format will not be created if it is not already known.
     */
    Format getFormat(String className);

    /**
     * @see PersistCatalog#createFormat
     */
    Format createFormat(String clsName, Map<String,Format> newFormats);

    /**
     * @see PersistCatalog#createFormat
     */
    Format createFormat(Class type, Map<String,Format> newFormats);

    /**
     * @see PersistCatalog#isRawAccess
     */
    boolean isRawAccess();

    /**
     * @see PersistCatalog#convertRawObject
     */
    Object convertRawObject(RawObject o, IdentityHashMap converted);
}

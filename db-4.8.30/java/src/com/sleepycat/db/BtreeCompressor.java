/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

/**
An interface specifying how Btree compression works.
*/
public interface BtreeCompressor {
    /**
    The application-specific Btree compress callback.
    <p>
    The compress function must return true on success and false on failure. If
    the compressed data is larger than dest.getUserBufferLength() the function
    should set the required length in the dest DatabaseEntry via setSize and
    return false.
    <p>
    @param db
    The enclosing database handle.
    @param prevKey
    A database entry representing the previous key from the compressed stream.
    @param prevData
    A database entry representing the data matching prevKey key entry.
    @param key
    A database entry representing representing the application supplied key.
    @param data
    A database entry representing representing the application supplied data.
    @param dest
    A database entry representing the data stored in the tree. This is where
    the callback function should write the compressed data.
    */
    boolean compress(Database db, DatabaseEntry prevKey, DatabaseEntry prevData,
        DatabaseEntry key, DatabaseEntry data, DatabaseEntry dest);

    /**
    The application-specific Btree decompress callback.
    <p>
    The decompress function must return true on success, and false on failure.
    <p>
    @param db
    The enclosing database handle.
    @param prevKey
    A database entry representing the previous key from the compressed stream.
    @param prevData
    A database entry representing the data matching prevKey key entry.
    @param compressed
    A database entry representing the data stored in the tree. That is the
    compressed data.
    @param key
    A database entry representing representing the application supplied key.
    @param data
    A database entry representing representing the application supplied data.
    */
    boolean decompress(Database db, DatabaseEntry prevKey,
        DatabaseEntry prevData, DatabaseEntry compressed, DatabaseEntry key,
        DatabaseEntry data);
}


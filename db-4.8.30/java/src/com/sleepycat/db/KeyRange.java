/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */
package com.sleepycat.db;

/**
An object that returns status from the {@link com.sleepycat.db.Database#getKeyRange Database.getKeyRange} method.
*/
public class KeyRange {

    /**
    Zero if there is no matching key, and non-zero otherwise.
    */
    public double equal;

    /**
    A value between 0 and 1, the proportion of keys greater than the
    specified key.
    <p>
    For example, if the value is 0.05, 5% of the keys in the database
    are greater than the specified key.
    */
    public double greater;

    /**
    A value between 0 and 1, the proportion of keys less than the specified
    key.
    <p>
    For example, if the value is 0.05, 5% of the keys in the database
    are less than the specified key.
    */
    public double less;
}

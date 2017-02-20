/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

import java.util.Set;

/**
The interface implemented for extracting multi-valued secondary keys from
primary records.
<p>
The key creator object is specified by calling
{@link SecondaryConfig#setMultiKeyCreator SecondaryConfig.setMultiKeyCreator}.
The secondary database configuration is specified when calling
{@link Environment#openSecondaryDatabase Environment.openSecondaryDatabase}.
<p>
For example:
<pre>
    class MyMultiKeyCreator implements SecondaryMultiKeyCreator {
    public void createSecondaryKeys(SecondaryDatabase secondary,
                                        DatabaseEntry key,
                                        DatabaseEntry data,
                                        Set results)
            throws DatabaseException {
            //
            // DO HERE: Extract the secondary keys from the primary key and
            // data.  For each key extracted, create a DatabaseEntry and add it
            // to the results set.
            //
        }
    }
    ...
    SecondaryConfig secConfig = new SecondaryConfig();
    secConfig.setMultiKeyCreator(new MyMultiKeyCreator());
    // Now pass secConfig to Environment.openSecondaryDatabase
</pre>
<p>
Use this interface when any number of secondary keys may be present in a single
primary record, in other words, for many-to-many and one-to-many relationships.
When only zero or one secondary key is present (for many-to-one and one-to-one
relationships) you may use the {@link SecondaryKeyCreator} interface instead.
The table below summarizes how to create all four variations of relationships.
<div>
<table border="yes">
    <tr><th>Relationship</th>
        <th>Interface</th>
        <th>Duplicates</th>
        <th>Example</th>
    </tr>
    <tr><td>One-to-one</td>
        <td>{@link SecondaryKeyCreator}</td>
        <td>No</td>
        <td>A person record with a unique social security number key.</td>
    </tr>
    <tr><td>Many-to-one</td>
        <td>{@link SecondaryKeyCreator}</td>
        <td>Yes</td>
        <td>A person record with a non-unique employer key.</td>
    </tr>
    <tr><td>One-to-many</td>
        <td>{@link SecondaryMultiKeyCreator}</td>
        <td>No</td>
        <td>A person record with multiple unique email address keys.</td>
    </tr>
    <tr><td>Many-to-many</td>
        <td>{@link SecondaryMultiKeyCreator}</td>
        <td>Yes</td>
        <td>A person record with multiple non-unique organization keys.</td>
    </tr>
</table>
</div>
<p>To configure a database for duplicates. pass true to {@link
DatabaseConfig#setSortedDuplicates}.</p>
<p>
Note that <code>SecondaryMultiKeyCreator</code> may also be used for single key
secondaries (many-to-one and one-to-one); in this case, at most a single key is
added to the results set.  <code>SecondaryMultiKeyCreator</code> is only
slightly less efficient than {@link SecondaryKeyCreator} in that two or three
temporary sets must be created to hold the results.
@see SecondaryConfig
*/
public interface SecondaryMultiKeyCreator {
    /**
    Creates a secondary key entry, given a primary key and data entry.
    <p>
    A secondary key may be derived from the primary key, primary data, or a
    combination of the primary key and data.  Zero or more secondary keys may
    be derived from the primary record and returned in the results parameter.
    To ensure the integrity of a secondary database the key creator method must
    always return the same results for a given set of input parameters.
    <p>
    @param secondary the database to which the secondary key will be added.
    This parameter is passed for informational purposes but is not commonly
    used.
    <p>
    @param key the primary key entry.  This parameter must not be modified
    by this method.
    <p>
    @param data the primary data entry.  This parameter must not be modified
    by this method.
    <p>
    @param results the set to contain the the secondary key DatabaseEntry
    objects created by this method.
    <p>
    @throws DatabaseException if an error occurs attempting to create the
    secondary key.
    */
    public void createSecondaryKeys(SecondaryDatabase secondary,
                                    DatabaseEntry key,
                                    DatabaseEntry data,
                                    Set results)
        throws DatabaseException;
}

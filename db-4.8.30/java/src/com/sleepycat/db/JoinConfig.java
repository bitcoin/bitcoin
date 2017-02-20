/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbConstants;

/**
The configuration properties of a <code>JoinCursor</code>.
The join cursor configuration is specified when calling {@link
Database#join Database.join}.
<p>
To create a configuration object with default attributes:
<pre>
    JoinConfig config = new JoinConfig();
</pre>
To set custom attributes:
<pre>
    JoinConfig config = new JoinConfig();
    config.setNoSort(true);
</pre>
<p>
@see Database#join Database.join
@see JoinCursor
*/
public class JoinConfig implements Cloneable {
    /**
    Default configuration used if null is passed to {@link com.sleepycat.db.Database#join Database.join}
    */
   public static final JoinConfig DEFAULT = new JoinConfig();

    /* package */
    static JoinConfig checkNull(JoinConfig config) {
        return (config == null) ? DEFAULT : config;
    }

    private boolean noSort;

    /**
    Creates an instance with the system's default settings.
    */
    public JoinConfig() {
    }

    /**
    Specifies whether automatic sorting of the input cursors is disabled.
    <p>
    Joined values are retrieved by doing a sequential iteration over the
    first cursor in the cursor array, and a nested iteration over each
    following cursor in the order they are specified in the array. This
    requires database traversals to search for the current datum in all the
    cursors after the first. For this reason, the best join performance
    normally results from sorting the cursors from the one that refers to
    the least number of data items to the one that refers to the most.
    Unless this method is called with true, <code>Database.join</code> does
    this sort on behalf of its caller.
    <p>
    If the data are structured so that cursors with many data items also
    share many common elements, higher performance will result from listing
    those cursors before cursors with fewer data items; that is, a sort
    order other than the default. Calling this method permits applications
    to perform join optimization prior to calling
    <code>Database.join</code>.
    <p>
    @param noSort whether automatic sorting of the input cursors is disabled.
    <p>
    @see Database#join Database.join
    */
    public void setNoSort(final boolean noSort) {
        this.noSort = noSort;
    }

    /**
    Returns whether automatic sorting of the input cursors is disabled.
    <p>
    @return whether automatic sorting of the input cursors is disabled.
    <p>
    @see #setNoSort
    */
    public boolean getNoSort() {
        return noSort;
    }

    /* package */
    int getFlags() {
        return noSort ? DbConstants.DB_JOIN_NOSORT : 0;
    }
}

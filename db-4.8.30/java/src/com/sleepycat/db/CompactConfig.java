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
Configuration for {@link Database#compact} operations.
*/
public class CompactConfig implements Cloneable {
    /**
    Default configuration used if null is passed to methods that create a
    cursor.
    */
    public static final CompactConfig DEFAULT = new CompactConfig();

    /* package */
    static CompactConfig checkNull(CompactConfig config) {
        return (config == null) ? DEFAULT : config;
    }

    private int fillpercent = 0;
    private boolean freeListOnly = false;
    private boolean freeSpace = false;
    private int maxPages = 0;
    private int timeout = 0;

    /**
    Construct a default configuration object for compact operations.
    */
    public CompactConfig() {
    }

    /**
    Set the desired fill percentage.
    If non-zero, the goal for filling pages, specified as a percentage
    between 1 and 100.  Any page in a Btree or Recno databases not at or
    above this percentage full will be considered for compaction.  The
    default behavior is to consider every page for compaction, regardless
    of its page fill percentage.
    @param fillpercent
    The desired fill percentage.
    **/
    public void setFillPercent(final int fillpercent) {
        this.fillpercent = fillpercent;
    }

    /**
Return the the desired fill percentage.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The the desired fill percentage.
 */
    public int getFillPercent() {
        return fillpercent;
    }

    /**
    Configure whether to skip page compaction, only returning pages
    to the filesystem that are already free and at the end of the file.
    This flag must be set if the database is a Hash access method database.
    @param freeListOnly
    Whether to skip page compaction
   **/
    public void setFreeListOnly(boolean freeListOnly) {
        this.freeListOnly = freeListOnly;
    }

    /**
Return true if the whether to skip page compaction.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the whether to skip page compaction.
 */
    public boolean getFreeListOnly() {
        return freeListOnly;
    }

    /**
    Return pages to the filesystem if possible.  If this flag is not
    specified, pages emptied as a result of compaction will be placed on the
    free list for re-use, but not returned to the filesystem.
    Note that only pages at the end of the file may be returned.  Given the one
    pass nature of the algorithm if a page near the end of the file is
    logically near the begining of the btree it will inhibit returning pages to
    the file system.
    A second call to the method with a low fillfactor can be used to return
    pages in such a situation.
    @param freeSpace
    Whether to return pages to the filesystem
    **/
    public void setFreeSpace(boolean freeSpace) {
        this.freeSpace = freeSpace;
    }

    /**
Return true if the whether to return pages to the filesystem.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the whether to return pages to the filesystem.
 */
    public boolean getFreeSpace() {
        return freeSpace;
    }

    /**
    Set the maximum number of pages to free.
    @param maxPages
    If non-zero, the call will return after that number of pages have been
    freed.
    **/
    public void setMaxPages(final int maxPages) {
        this.maxPages = maxPages;
    }

    /**
Return the the maximum number of pages to free.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The the maximum number of pages to free.
 */
    public int getMaxPages() {
        return maxPages;
    }

    /**
    Set the lock timeout for implicit transactions.
    If non-zero, and no transaction parameter was specified to
    {@link Database#compact}, the lock timeout set for implicit
    transactions, in microseconds.
    @param timeout
    the lock timeout set for implicit transactions, in microseconds.
    **/
    public void setTimeout(final int timeout) {
        this.timeout = timeout;
    }

    /**
Return the the lock timeout set for implicit transactions, in microseconds.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The the lock timeout set for implicit transactions, in microseconds.
 */
    public int getTimeout() {
        return timeout;
    }

    /* package */
    int getFlags() {
        int flags = 0;
        flags |= freeListOnly ? DbConstants.DB_FREELIST_ONLY : 0;
        flags |= freeSpace ? DbConstants.DB_FREE_SPACE : 0;

        return flags;
    }
}

/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbConstants;
import com.sleepycat.db.internal.DbMpoolFile;

/**
This class allows applications to modify settings for
a {@link com.sleepycat.db.Database Database} using the {@link com.sleepycat.db.Database#getCacheFile Database.getCacheFile}.
*/
public class CacheFile {
    private DbMpoolFile mpf;

    /* package */
    CacheFile(final DbMpoolFile mpf) {
        this.mpf = mpf;
    }

    /**
Return the cache priority for pages from the specified file.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The cache priority for pages from the specified file.
<p>
<p>
@throws DatabaseException if a failure occurs.
    */
    public CacheFilePriority getPriority()
        throws DatabaseException {

        return CacheFilePriority.fromFlag(mpf.get_priority());
    }

    /**
Set the
cache priority for pages from the specified file.
<p>The priority of a page biases the replacement algorithm to be more
    or less likely to discard a page when space is needed in the buffer
    pool.  The bias is temporary, and pages will eventually be discarded
    if they are not referenced again.  Setting the priority is only
    advisory, and does not guarantee pages will be treated in a specific
    way.
<p>
This method may be called at any time during the life of the application.
<p>
@param priority
The cache priority for pages from the specified file.
<p>
<p>
@throws DatabaseException if a failure occurs.
    */
    public void setPriority(final CacheFilePriority priority)
        throws DatabaseException {

        mpf.set_priority(priority.getFlag());
    }

    /**
Return the maximum size for the file backing the database, or 0 if
    no maximum file size has been configured.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The maximum size for the file backing the database, or 0 if
    no maximum file size has been configured.
<p>
<p>
@throws DatabaseException if a failure occurs.
    */
    public long getMaximumSize()
        throws DatabaseException {

        return mpf.get_maxsize();
    }

    /**
Set the
maximum size for the file backing the database.
<p>Attempts to allocate new pages in the file after the limit has been
    reached will fail.
<p>
This method may be called at any time during the life of the application.
<p>
@param bytes
The maximum size for the file backing the database.
<p>
<p>
@throws DatabaseException if a failure occurs.
    */
    public void setMaximumSize(final long bytes)
        throws DatabaseException {

        mpf.set_maxsize(bytes);
    }

    /**
Return true if the opening of backing temporary files for in-memory
    databases has been disallowed.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the opening of backing temporary files for in-memory
    databases has been disallowed.
<p>
<p>
@throws DatabaseException if a failure occurs.
    */
    public boolean getNoFile()
        throws DatabaseException {

        return (mpf.get_flags() & DbConstants.DB_MPOOL_NOFILE) != 0;
    }

    /**
Disallow opening backing temporary files for in-memory
    databases, even if they expand to fill the entire cache.
<p>Attempts to create new file pages after the cache has been filled
    will fail.
<p>
This method may be called at any time during the life of the application.
<p>
@param onoff
If true,
disallow opening backing temporary files for in-memory
    databases, even if they expand to fill the entire cache.
<p>
<p>
@throws DatabaseException if a failure occurs.
    */
    public void setNoFile(final boolean onoff)
        throws DatabaseException {

        mpf.set_flags(DbConstants.DB_MPOOL_NOFILE, onoff);
    }

    /**
Return true if the file will be removed when the last reference to it is
    closed.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the file will be removed when the last reference to it is
    closed.
<p>
<p>
@throws DatabaseException if a failure occurs.
    */
    public boolean getUnlink()
        throws DatabaseException {

        return (mpf.get_flags() & DbConstants.DB_MPOOL_UNLINK) != 0;
    }

    /**
Remove the file when the last reference to it is closed.
<p>
This method may be called at any time during the life of the application.
<p>
@param onoff
If true,
remove the file when the last reference to it is closed.
<p>
<p>
@throws DatabaseException if a failure occurs.
    */
    public void setUnlink(boolean onoff)
        throws DatabaseException {

        mpf.set_flags(DbConstants.DB_MPOOL_UNLINK, onoff);
    }
}

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
 * Priorities that can be assigned to files in the cache.
 */
public final class CacheFilePriority {
    /**
    The default priority.
    */
    public static final CacheFilePriority DEFAULT =
        new CacheFilePriority("DEFAULT", DbConstants.DB_PRIORITY_DEFAULT);
    /**
    The second highest priority.
    */
    public static final CacheFilePriority HIGH =
        new CacheFilePriority("HIGH", DbConstants.DB_PRIORITY_HIGH);
    /**
    The second lowest priority.
    */
    public static final CacheFilePriority LOW =
        new CacheFilePriority("LOW", DbConstants.DB_PRIORITY_LOW);
    /**
    The highest priority: pages are the least likely to be discarded.
    */
    public static final CacheFilePriority VERY_HIGH =
        new CacheFilePriority("VERY_HIGH", DbConstants.DB_PRIORITY_VERY_HIGH);
    /**
    The lowest priority: pages are the most likely to be discarded.
    */
    public static final CacheFilePriority VERY_LOW =
        new CacheFilePriority("VERY_LOW", DbConstants.DB_PRIORITY_VERY_LOW);

    /* package */
    static CacheFilePriority fromFlag(int flag) {
        switch (flag) {
        case 0:
        case DbConstants.DB_PRIORITY_DEFAULT:
            return DEFAULT;
        case DbConstants.DB_PRIORITY_HIGH:
            return HIGH;
        case DbConstants.DB_PRIORITY_LOW:
            return LOW;
        case DbConstants.DB_PRIORITY_VERY_HIGH:
            return VERY_HIGH;
        case DbConstants.DB_PRIORITY_VERY_LOW:
            return VERY_LOW;
        default:
            throw new IllegalArgumentException(
                "Unknown cache priority: " + flag);
        }
    }

    private final String priorityName;
    private final int flag;

    private CacheFilePriority(final String priorityName, final int flag) {
        this.priorityName = priorityName;
        this.flag = flag;
    }

    /** {@inheritDoc} */
    public String toString() {
        return "CacheFilePriority." + priorityName;
    }

    /* package */
    int getFlag() {
        return flag;
    }
}

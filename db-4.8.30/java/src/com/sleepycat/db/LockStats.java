/*-
 * Automatically built by dist/s_java_stat.
 * Only the javadoc comments can be edited.
 *
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 */

package com.sleepycat.db;

/**
Lock statistics for a database environment.
*/
public class LockStats {
    // no public constructor
    /* package */ LockStats() {}

    private int st_id;
    /** TODO */
    public int getId() {
        return st_id;
    }

    private int st_cur_maxid;
    /** TODO */
    public int getCurMaxId() {
        return st_cur_maxid;
    }

    private int st_maxlocks;
    /** TODO */
    public int getMaxLocks() {
        return st_maxlocks;
    }

    private int st_maxlockers;
    /** TODO */
    public int getMaxLockers() {
        return st_maxlockers;
    }

    private int st_maxobjects;
    /** TODO */
    public int getMaxObjects() {
        return st_maxobjects;
    }

    private int st_partitions;
    /** TODO */
    public int getPartitions() {
        return st_partitions;
    }

    private int st_nmodes;
    /** TODO */
    public int getNumModes() {
        return st_nmodes;
    }

    private int st_nlockers;
    /** TODO */
    public int getNumLockers() {
        return st_nlockers;
    }

    private int st_nlocks;
    /** TODO */
    public int getNumLocks() {
        return st_nlocks;
    }

    private int st_maxnlocks;
    /** TODO */
    public int getMaxNlocks() {
        return st_maxnlocks;
    }

    private int st_maxhlocks;
    /** TODO */
    public int getMaxHlocks() {
        return st_maxhlocks;
    }

    private long st_locksteals;
    /** TODO */
    public long getLocksteals() {
        return st_locksteals;
    }

    private long st_maxlsteals;
    /** TODO */
    public long getMaxLsteals() {
        return st_maxlsteals;
    }

    private int st_maxnlockers;
    /** TODO */
    public int getMaxNlockers() {
        return st_maxnlockers;
    }

    private int st_nobjects;
    /** TODO */
    public int getNobjects() {
        return st_nobjects;
    }

    private int st_maxnobjects;
    /** TODO */
    public int getMaxNobjects() {
        return st_maxnobjects;
    }

    private int st_maxhobjects;
    /** TODO */
    public int getMaxHobjects() {
        return st_maxhobjects;
    }

    private long st_objectsteals;
    /** TODO */
    public long getObjectsteals() {
        return st_objectsteals;
    }

    private long st_maxosteals;
    /** TODO */
    public long getMaxOsteals() {
        return st_maxosteals;
    }

    private long st_nrequests;
    /** TODO */
    public long getNumRequests() {
        return st_nrequests;
    }

    private long st_nreleases;
    /** TODO */
    public long getNumReleases() {
        return st_nreleases;
    }

    private long st_nupgrade;
    /** TODO */
    public long getNumUpgrade() {
        return st_nupgrade;
    }

    private long st_ndowngrade;
    /** TODO */
    public long getNumDowngrade() {
        return st_ndowngrade;
    }

    private long st_lock_wait;
    /** TODO */
    public long getLockWait() {
        return st_lock_wait;
    }

    private long st_lock_nowait;
    /** TODO */
    public long getLockNowait() {
        return st_lock_nowait;
    }

    private long st_ndeadlocks;
    /** TODO */
    public long getNumDeadlocks() {
        return st_ndeadlocks;
    }

    private int st_locktimeout;
    /** TODO */
    public int getLockTimeout() {
        return st_locktimeout;
    }

    private long st_nlocktimeouts;
    /** TODO */
    public long getNumLockTimeouts() {
        return st_nlocktimeouts;
    }

    private int st_txntimeout;
    /** TODO */
    public int getTxnTimeout() {
        return st_txntimeout;
    }

    private long st_ntxntimeouts;
    /** TODO */
    public long getNumTxnTimeouts() {
        return st_ntxntimeouts;
    }

    private long st_part_wait;
    /** TODO */
    public long getPartWait() {
        return st_part_wait;
    }

    private long st_part_nowait;
    /** TODO */
    public long getPartNowait() {
        return st_part_nowait;
    }

    private long st_part_max_wait;
    /** TODO */
    public long getPartMaxWait() {
        return st_part_max_wait;
    }

    private long st_part_max_nowait;
    /** TODO */
    public long getPartMaxNowait() {
        return st_part_max_nowait;
    }

    private long st_objs_wait;
    /** TODO */
    public long getObjsWait() {
        return st_objs_wait;
    }

    private long st_objs_nowait;
    /** TODO */
    public long getObjsNowait() {
        return st_objs_nowait;
    }

    private long st_lockers_wait;
    /** TODO */
    public long getLockersWait() {
        return st_lockers_wait;
    }

    private long st_lockers_nowait;
    /** TODO */
    public long getLockersNowait() {
        return st_lockers_nowait;
    }

    private long st_region_wait;
    /** TODO */
    public long getRegionWait() {
        return st_region_wait;
    }

    private long st_region_nowait;
    /** TODO */
    public long getRegionNowait() {
        return st_region_nowait;
    }

    private int st_hash_len;
    /** TODO */
    public int getHashLen() {
        return st_hash_len;
    }

    private int st_regsize;
    /** TODO */
    public int getRegSize() {
        return st_regsize;
    }

    /**
    For convenience, the LockStats class has a toString method
    that lists all the data fields.
    */
    public String toString() {
        return "LockStats:"
            + "\n  st_id=" + st_id
            + "\n  st_cur_maxid=" + st_cur_maxid
            + "\n  st_maxlocks=" + st_maxlocks
            + "\n  st_maxlockers=" + st_maxlockers
            + "\n  st_maxobjects=" + st_maxobjects
            + "\n  st_partitions=" + st_partitions
            + "\n  st_nmodes=" + st_nmodes
            + "\n  st_nlockers=" + st_nlockers
            + "\n  st_nlocks=" + st_nlocks
            + "\n  st_maxnlocks=" + st_maxnlocks
            + "\n  st_maxhlocks=" + st_maxhlocks
            + "\n  st_locksteals=" + st_locksteals
            + "\n  st_maxlsteals=" + st_maxlsteals
            + "\n  st_maxnlockers=" + st_maxnlockers
            + "\n  st_nobjects=" + st_nobjects
            + "\n  st_maxnobjects=" + st_maxnobjects
            + "\n  st_maxhobjects=" + st_maxhobjects
            + "\n  st_objectsteals=" + st_objectsteals
            + "\n  st_maxosteals=" + st_maxosteals
            + "\n  st_nrequests=" + st_nrequests
            + "\n  st_nreleases=" + st_nreleases
            + "\n  st_nupgrade=" + st_nupgrade
            + "\n  st_ndowngrade=" + st_ndowngrade
            + "\n  st_lock_wait=" + st_lock_wait
            + "\n  st_lock_nowait=" + st_lock_nowait
            + "\n  st_ndeadlocks=" + st_ndeadlocks
            + "\n  st_locktimeout=" + st_locktimeout
            + "\n  st_nlocktimeouts=" + st_nlocktimeouts
            + "\n  st_txntimeout=" + st_txntimeout
            + "\n  st_ntxntimeouts=" + st_ntxntimeouts
            + "\n  st_part_wait=" + st_part_wait
            + "\n  st_part_nowait=" + st_part_nowait
            + "\n  st_part_max_wait=" + st_part_max_wait
            + "\n  st_part_max_nowait=" + st_part_max_nowait
            + "\n  st_objs_wait=" + st_objs_wait
            + "\n  st_objs_nowait=" + st_objs_nowait
            + "\n  st_lockers_wait=" + st_lockers_wait
            + "\n  st_lockers_nowait=" + st_lockers_nowait
            + "\n  st_region_wait=" + st_region_wait
            + "\n  st_region_nowait=" + st_region_nowait
            + "\n  st_hash_len=" + st_hash_len
            + "\n  st_regsize=" + st_regsize
            ;
    }
}

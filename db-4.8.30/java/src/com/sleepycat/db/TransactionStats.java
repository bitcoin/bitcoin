/*-
 * Automatically built by dist/s_java_stat.
 * Only the javadoc comments can be edited.
 *
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbUtil;

/**
Transaction statistics for a database environment.
*/
public class TransactionStats {
    // no public constructor
    /* package */ TransactionStats() {}

    public static class Active {
        // no public constructor
        /* package */ Active() {}

        private int txnid;
        /** TODO */
    public int getTxnId() {
            return txnid;
        }

        private int parentid;
        /** TODO */
    public int getParentId() {
            return parentid;
        }

        private int pid;
        /** TODO */
    public int getPid() {
            return pid;
        }

        private LogSequenceNumber lsn;
        /** TODO */
    public LogSequenceNumber getLsn() {
            return lsn;
        }

        private LogSequenceNumber read_lsn;
        /** TODO */
    public LogSequenceNumber getReadLsn() {
            return read_lsn;
        }

        private int mvcc_ref;
        /** TODO */
    public int getMultiversionRef() {
            return mvcc_ref;
        }

        private int status;
        /** TODO */
    public int getStatus() {
            return status;
        }

        private byte[] gid;
        public byte[] getGId() {
            return gid;
        }

        private String name;
        /** TODO */
    public String getName() {
            return name;
        }

        /** {@inheritDoc} */
    public String toString() {
            return "Active:"
                + "\n      txnid=" + txnid
                + "\n      parentid=" + parentid
                + "\n      pid=" + pid
                + "\n      lsn=" + lsn
                + "\n      read_lsn=" + read_lsn
                + "\n      mvcc_ref=" + mvcc_ref
                + "\n      status=" + status
                + "\n      gid=" + DbUtil.byteArrayToString(gid)
                + "\n      name=" + name
                ;
        }
    };

    private int st_nrestores;
    /** TODO */
    public int getNumRestores() {
        return st_nrestores;
    }

    private LogSequenceNumber st_last_ckp;
    /** TODO */
    public LogSequenceNumber getLastCkp() {
        return st_last_ckp;
    }

    private long st_time_ckp;
    /** TODO */
    public long getTimeCkp() {
        return st_time_ckp;
    }

    private int st_last_txnid;
    /** TODO */
    public int getLastTxnId() {
        return st_last_txnid;
    }

    private int st_maxtxns;
    /** TODO */
    public int getMaxTxns() {
        return st_maxtxns;
    }

    private long st_naborts;
    /** TODO */
    public long getNaborts() {
        return st_naborts;
    }

    private long st_nbegins;
    /** TODO */
    public long getNumBegins() {
        return st_nbegins;
    }

    private long st_ncommits;
    /** TODO */
    public long getNumCommits() {
        return st_ncommits;
    }

    private int st_nactive;
    /** TODO */
    public int getNactive() {
        return st_nactive;
    }

    private int st_nsnapshot;
    /** TODO */
    public int getNumSnapshot() {
        return st_nsnapshot;
    }

    private int st_maxnactive;
    /** TODO */
    public int getMaxNactive() {
        return st_maxnactive;
    }

    private int st_maxnsnapshot;
    /** TODO */
    public int getMaxNsnapshot() {
        return st_maxnsnapshot;
    }

    private Active[] st_txnarray;
    public Active[] getTxnarray() {
        return st_txnarray;
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

    private int st_regsize;
    /** TODO */
    public int getRegSize() {
        return st_regsize;
    }

    /** {@inheritDoc} */
    public String toString() {
        return "TransactionStats:"
            + "\n  st_nrestores=" + st_nrestores
            + "\n  st_last_ckp=" + st_last_ckp
            + "\n  st_time_ckp=" + st_time_ckp
            + "\n  st_last_txnid=" + st_last_txnid
            + "\n  st_maxtxns=" + st_maxtxns
            + "\n  st_naborts=" + st_naborts
            + "\n  st_nbegins=" + st_nbegins
            + "\n  st_ncommits=" + st_ncommits
            + "\n  st_nactive=" + st_nactive
            + "\n  st_nsnapshot=" + st_nsnapshot
            + "\n  st_maxnactive=" + st_maxnactive
            + "\n  st_maxnsnapshot=" + st_maxnsnapshot
            + "\n  st_txnarray=" + DbUtil.objectArrayToString(st_txnarray, "st_txnarray")
            + "\n  st_region_wait=" + st_region_wait
            + "\n  st_region_nowait=" + st_region_nowait
            + "\n  st_regsize=" + st_regsize
            ;
    }
}

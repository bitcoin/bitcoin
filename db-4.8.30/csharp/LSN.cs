/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.Text;
using BerkeleyDB.Internal;

namespace BerkeleyDB {
    /// <summary>
    /// A log sequence number, which specifies a unique location in a log file.
    /// </summary>
    public class LSN {
        /// <summary>
        /// The log file number.
        /// </summary>
        public uint LogFileNumber;
        /// <summary>
        /// The offset in the log file. 
        /// </summary>
        public uint Offset;

        /// <summary>
        /// Instantiate a new LSN object
        /// </summary>
        /// <param name="file">The log file number.</param>
        /// <param name="off">The offset in the log file.</param>
        public LSN(uint file, uint off) {
            LogFileNumber = file;
            Offset = off;
        }

        internal LSN(DB_LSN dblsn) {
            LogFileNumber = dblsn.file;
            Offset = dblsn.offset;
        }

        internal static DB_LSN getDB_LSN(LSN inp) {
            if (inp == null)
                return null;

            DB_LSN ret = new DB_LSN();
            ret.file = inp.LogFileNumber;
            ret.offset = inp.Offset;
            return ret;
        }

        /// <summary>
        /// Compare two LSNs.
        /// </summary>
        /// <param name="lsn1">The first LSN to compare</param>
        /// <param name="lsn2">The second LSN to compare</param>
        /// <returns>
        /// 0 if they are equal, 1 if lsn1 is greater than lsn2, and -1 if lsn1
        /// is less than lsn2.
        /// </returns>
        public static int Compare(LSN lsn1, LSN lsn2) {
            DB_LSN a = new DB_LSN();
            a.offset = lsn1.Offset;
            a.file = lsn1.LogFileNumber;

            DB_LSN b = new DB_LSN();
            b.offset = lsn2.Offset;
            b.file = lsn2.LogFileNumber;

            return libdb_csharp.log_compare(a, b);
        }

    }
}
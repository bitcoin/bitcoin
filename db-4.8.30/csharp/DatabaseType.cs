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
    /// A class representing the supported Berkeley DB access methods.
    /// </summary>
    public class DatabaseType {
        /// <summary>
        /// BTree access method
        /// </summary>
        public static readonly DatabaseType BTREE
            = new DatabaseType(DBTYPE.DB_BTREE);
        /// <summary>
        /// Hash access method
        /// </summary>
        public static readonly DatabaseType HASH
            = new DatabaseType(DBTYPE.DB_HASH);
        /// <summary>
        /// Recno access method
        /// </summary>
        public static readonly DatabaseType RECNO
            = new DatabaseType(DBTYPE.DB_RECNO);
        /// <summary>
        /// Queue access method
        /// </summary>
        public static readonly DatabaseType QUEUE
            = new DatabaseType(DBTYPE.DB_QUEUE);
        /// <summary>
        /// Unknown access method
        /// </summary>
        public static readonly DatabaseType UNKNOWN
            = new DatabaseType(DBTYPE.DB_UNKNOWN);

        private BerkeleyDB.Internal.DBTYPE dbtype;

        private DatabaseType(BerkeleyDB.Internal.DBTYPE type) {
            dbtype = type;
        }

        internal BerkeleyDB.Internal.DBTYPE getDBTYPE() {
            return dbtype;
        }

        /// <summary>
        /// Convert this instance of DatabaseType to its string representation.
        /// </summary>
        /// <returns>A string representation of this instance.</returns>
        public override string ToString() {
            switch (dbtype) {
                case DBTYPE.DB_BTREE:
                    return "BTree";
                case DBTYPE.DB_HASH:
                    return "Hash";
                case DBTYPE.DB_QUEUE:
                    return "Queue";
                case DBTYPE.DB_RECNO:
                    return "Recno";
                default:
                    return "Unknown";
            }
        }

    }
}
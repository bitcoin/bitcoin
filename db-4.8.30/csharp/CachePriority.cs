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
    /// A class to represent cache priority for database pages
    /// </summary>
    public class CachePriority {
        private uint _priority;
        internal uint priority {
            get { return _priority; }
            private set { _priority = value; }
        }

        private CachePriority(uint value) {
            priority = value;
        }

        internal static CachePriority fromUInt(uint value) {
            switch (value) {
                case DbConstants.DB_PRIORITY_VERY_LOW:
                    return VERY_LOW;
                case DbConstants.DB_PRIORITY_LOW:
                    return LOW;
                case DbConstants.DB_PRIORITY_DEFAULT:
                    return DEFAULT;
                case DbConstants.DB_PRIORITY_HIGH:
                    return HIGH;
                case DbConstants.DB_PRIORITY_VERY_HIGH:
                    return VERY_HIGH;
                default:
                    throw new ArgumentException("Invalid CachePriority value.");
            }
        }

        /// <summary>
        /// The lowest priority: pages are the most likely to be discarded. 
        /// </summary>
        public static CachePriority VERY_LOW =
            new CachePriority(DbConstants.DB_PRIORITY_VERY_LOW);
        /// <summary>
        /// The next lowest priority.
        /// </summary>
        public static CachePriority LOW =
            new CachePriority(DbConstants.DB_PRIORITY_LOW);
        /// <summary>
        /// The default priority.
        /// </summary>
        public static CachePriority DEFAULT =
                    new CachePriority(DbConstants.DB_PRIORITY_DEFAULT);
        /// <summary>
        /// The next highest priority. 
        /// </summary>
        public static CachePriority HIGH =
                    new CachePriority(DbConstants.DB_PRIORITY_HIGH);
        /// <summary>
        /// The highest priority: pages are the least likely to be discarded.
        /// </summary>
        public static CachePriority VERY_HIGH =
                    new CachePriority(DbConstants.DB_PRIORITY_VERY_HIGH);
    }
}

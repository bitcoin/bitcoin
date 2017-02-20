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
    /// A class for traversing the records of a <see cref="RecnoDatabase"/>
    /// </summary>
    public class RecnoCursor : Cursor {
        internal RecnoCursor(DBC dbc, uint pagesize)
            : base(dbc, DatabaseType.RECNO, pagesize) { }
        internal RecnoCursor(DBC dbc, uint pagesize, CachePriority p)
            : base(dbc, DatabaseType.RECNO, pagesize, p) { }

        /// <summary>
        /// Create a new cursor that uses the same transaction and locker ID as
        /// the original cursor.
        /// </summary>
        /// <remarks>
        /// This is useful when an application is using locking and requires two
        /// or more cursors in the same thread of control.
        /// </remarks>
        /// <param name="keepPosition">
        /// If true, the newly created cursor is initialized to refer to the
        /// same position in the database as the original cursor (if any) and
        /// hold the same locks (if any). If false, or the original cursor does
        /// not hold a database position and locks, the created cursor is
        /// uninitialized and will behave like a cursor newly created by
        /// <see cref="RecnoDatabase.Cursor"/>.</param>
        /// <returns>A newly created cursor</returns>
        public new RecnoCursor Duplicate(bool keepPosition) {
            return new RecnoCursor(
                dbc.dup(keepPosition ? DbConstants.DB_POSITION : 0), pgsz);
        }

        /// <summary>
        /// Insert the data element as a duplicate element of the key to which
        /// the cursor refers.
        /// </summary>
        /// <param name="data">The data element to insert</param>
        /// <param name="loc">
        /// Specify whether to insert the data item immediately before or
        /// immediately after the cursor's current position.
        /// </param>
        public new void Insert(DatabaseEntry data, InsertLocation loc) {
            base.Insert(data, loc);
        }
    }
}
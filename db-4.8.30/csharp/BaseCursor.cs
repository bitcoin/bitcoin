/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 */
using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using BerkeleyDB.Internal;

namespace BerkeleyDB {
    /// <summary>
    /// <para>
    /// The abstract base class from which all cursor classes inherit.
    /// </para>
    /// <para>
    /// Cursors may span threads, but only serially, that is, the application
    /// must serialize access to the cursor handle.
    /// </para>
    /// </summary>
    public abstract class BaseCursor :
        IDisposable, IEnumerable<KeyValuePair<DatabaseEntry, DatabaseEntry>> {
        /// <summary>
        /// The underlying DBC handle
        /// </summary>
        internal DBC dbc;
        private bool isOpen;
        static internal DBC getDBC(BaseCursor curs) {
            return curs == null ? null : curs.dbc;
        }

        internal BaseCursor(DBC dbc) {
            this.dbc = dbc;
            isOpen = true;
        }

        /// <summary>
        /// Compare this cursor's position to another's.
        /// </summary>
        /// <param name="compareTo">The cursor with which to compare.</param>
        /// <returns>
        /// True if both cursors point to the same item, false otherwise.
        /// </returns>
        public bool Compare(Cursor compareTo) {
            int ret = 0;
            dbc.cmp(compareTo.dbc, ref ret, 0);
            return (ret == 0);
        }

        /// <summary>
        /// Returns a count of the number of data items for the key to which the
        /// cursor refers. 
        /// </summary>
        /// <returns>
        /// A count of the number of data items for the key to which the cursor
        /// refers.
        /// </returns>
        public uint Count() {
            int ret;
            uint count = 0;
            ret = dbc.count(ref count, 0);
            return count;
        }

        /// <summary>
        /// <para>
        /// Discard the cursor.
        /// </para>
        /// <para>
        /// It is possible for the Close() method to throw a
        /// <see cref="DeadlockException"/>, signaling that any enclosing
        /// transaction should be aborted. If the application is already
        /// intending to abort the transaction, this error should be ignored,
        /// and the application should proceed.
        /// </para>
        /// <para>
        /// After Close has been called, regardless of its result, the object
        /// may not be used again. 
        /// </para>
        /// </summary>
        /// <exception cref="DeadlockException"></exception>
        public void Close() {
            dbc.close();

            isOpen = false;
        }

        /// <summary>
        /// Release the resources held by this object, and close the cursor if
        /// it's still open.
        /// </summary>
        public void Dispose() {
            try {
                if (isOpen)
                    Close();
            } catch {
                /* 
                 * Errors here are likely because our db has been closed out
                 * from under us.  Not much we can do, just move on. 
                 */
            }
            dbc.Dispose();
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// <para>
        /// Delete the key/data pair to which the cursor refers.
        /// </para>
        /// <para>
        /// When called on a SecondaryCursor, delete the key/data pair from the
        /// primary database and all secondary indices.
        /// </para>
        /// <para>
        /// The cursor position is unchanged after a delete, and subsequent
        /// calls to cursor functions expecting the cursor to refer to an
        /// existing key will fail.
        /// </para>
        /// </summary>
        /// <exception cref="KeyEmptyException">
        /// Thrown if the element has already been deleted.
        /// </exception>
        public void Delete() {
            dbc.del(0);
        }
        IEnumerator IEnumerable.GetEnumerator() { return GetEnumerator(); }
        /// <summary>
        /// Returns an enumerator that iterates through the cursor.
        /// </summary>
        /// <returns>An enumerator for the cursor.</returns>
        public virtual IEnumerator<KeyValuePair<DatabaseEntry, DatabaseEntry>>
            GetEnumerator() { return null; }
    }
}

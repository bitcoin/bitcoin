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
    /// A class representing a join cursor, for use in performing equality or
    /// natural joins on secondary indices.  For information on how to organize
    /// your data to use this functionality, see Equality join in the
    /// Programmer's Reference Guide. 
    /// </summary>
    /// <remarks>
    /// JoinCursor does not support many of the operations offered by
    /// <see cref="Cursor"/> and is not a subclass of <see cref="Cursor"/>.
    /// </remarks>
    /// <seealso cref="Database.Join"/>
    public class JoinCursor : IDisposable,
        IEnumerable<KeyValuePair<DatabaseEntry, DatabaseEntry>> {
        internal DBC dbc;
        private bool isOpen;
        internal JoinCursor(DBC dbc) {
            this.dbc = dbc;
            isOpen = true;
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
            isOpen = false;
            dbc.close();
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
                 * Errors here are likely because our dbc has been closed out
                 * from under us.  Not much we can do, just move on. 
                 */
            }
            dbc.Dispose();
            GC.SuppressFinalize(this);
        }

        private KeyValuePair<DatabaseEntry, DatabaseEntry> cur;
        /// <summary>
        /// The key/data pair at which the cursor currently points.
        /// </summary>
        public KeyValuePair<DatabaseEntry, DatabaseEntry> Current {
            get { return cur; }
            private set { cur = value; }
        }

        IEnumerator IEnumerable.GetEnumerator() {
            return GetEnumerator();
        }
        /// <summary>
        /// Returns an enumerator that iterates through the
        /// <see cref="JoinCursor"/>.
        /// </summary>
        /// <remarks>
        /// The enumerator will begin at the cursor's current position (or the
        /// first record if the cursor has not yet been positioned) and iterate 
        /// forwards (i.e. in the direction of <see cref="MoveNext"/>) over the
        /// remaining records.
        /// </remarks>
        /// <returns>An enumerator for the Cursor.</returns>
        public IEnumerator<KeyValuePair<DatabaseEntry, DatabaseEntry>>
            GetEnumerator() {
            while (MoveNext())
                yield return Current;
        }

        /// <summary>
        /// Iterate over the values associated with the keys to which each 
        /// <see cref="SecondaryCursor"/> passed to <see cref="Database.Join"/>
        /// was initialized. Any data value that appears in all
        /// <see cref="SecondaryCursor"/>s is then used as a key into the
        /// primary, and the key/data pair found in the primary is stored in 
        /// <see cref="Current"/>.
        /// </summary>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveNext() {
            return MoveNext(null, false);
        }
        /// <summary>
        /// Iterate over the values associated with the keys to which each 
        /// <see cref="SecondaryCursor"/> passed to <see cref="Database.Join"/>
        /// was initialized. Any data value that appears in all
        /// <see cref="SecondaryCursor"/>s is then used as a key into the
        /// primary, and the key/data pair found in the primary is stored in 
        /// <see cref="Current"/>.
        /// </summary>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveNext(LockingInfo info) {
            return MoveNext(info, false);
        }
        /// <summary>
        /// Iterate over the values associated with the keys to which each 
        /// <see cref="SecondaryCursor"/> passed to <see cref="Database.Join"/>
        /// was initialized. Any data value that appears in all
        /// <see cref="SecondaryCursor"/>s is then stored in 
        /// <see cref="Current">Current.Key</see>.
        /// </summary>
        /// <remarks>
        /// <see cref="Current">Current.Value</see> will contain an empty
        /// <see cref="DatabaseEntry"/>.
        /// </remarks>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveNextItem() {
            return MoveNext(null, true);
        }
        /// <summary>
        /// Iterate over the values associated with the keys to which each 
        /// <see cref="SecondaryCursor"/> passed to <see cref="Database.Join"/>
        /// was initialized. Any data value that appears in all
        /// <see cref="SecondaryCursor"/>s is then stored in 
        /// <see cref="Current">Current.Key</see>.
        /// </summary>
        /// <remarks>
        /// <see cref="Current">Current.Value</see> will contain an empty
        /// <see cref="DatabaseEntry"/>.
        /// </remarks>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveNextItem(LockingInfo info) {
            return MoveNext(info, true);
        }
        private bool MoveNext(LockingInfo info, bool joinItem) {
            int ret;
            uint flags = 0;
            DatabaseEntry key = new DatabaseEntry();
            DatabaseEntry data = new DatabaseEntry();

            flags |= joinItem ? DbConstants.DB_JOIN_ITEM : 0;
            flags |= (info == null) ? 0 : info.flags;

            try {
                ret = dbc.get(key, data, flags);
                Current = 
                    new KeyValuePair<DatabaseEntry, DatabaseEntry>(key, data);
                return true;
            } catch (NotFoundException) {
                Current = new KeyValuePair<DatabaseEntry, DatabaseEntry>();
                return false;
            }
        }
    }
}

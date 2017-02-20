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
    /// A class representing database cursors over secondary indexes, which
    /// allow for traversal of database records.
    /// </summary>
    public class SecondaryCursor
        : BaseCursor, IEnumerable<KeyValuePair<DatabaseEntry,
        KeyValuePair<DatabaseEntry, DatabaseEntry>>> {
        private KeyValuePair<DatabaseEntry,
            KeyValuePair<DatabaseEntry, DatabaseEntry>> cur;
        /// <summary>
        /// The secondary key and primary key/data pair at which the cursor
        /// currently points.
        /// </summary>
        public KeyValuePair<DatabaseEntry,
            KeyValuePair<DatabaseEntry, DatabaseEntry>> Current {
            get { return cur; }
            private set { cur = value; }
        }
        internal SecondaryCursor(DBC dbc) : base(dbc) { }

        /// <summary>
        /// Protected method wrapping DBC->pget()
        /// </summary>
        /// <param name="key">The secondary key</param>
        /// <param name="pkey">The primary key</param>
        /// <param name="data">The primary data</param>
        /// <param name="flags">Flags to pass to DBC->pget</param>
        /// <param name="info">Locking parameters</param>
        /// <returns></returns>
        protected bool PGet(
            DatabaseEntry key, DatabaseEntry pkey,
            DatabaseEntry data, uint flags, LockingInfo info) {
            flags |= (info == null) ? 0 : info.flags;
            try {
                dbc.pget(key, pkey, data, flags);
                Current = new KeyValuePair<DatabaseEntry,
                    KeyValuePair<DatabaseEntry, DatabaseEntry>>(key,
                    new KeyValuePair<DatabaseEntry, DatabaseEntry>(pkey, data));
                return true;
            } catch (NotFoundException) {
                Current = new KeyValuePair<DatabaseEntry,
                    KeyValuePair<DatabaseEntry, DatabaseEntry>>(
                    null, new KeyValuePair<DatabaseEntry, DatabaseEntry>());
                return false;
            }
        }

        /// <summary>
        /// Delete the key/data pair to which the cursor refers from the primary
        /// database and all secondary indices.
        /// </summary>
        /// <remarks>
        /// <para>
        /// The cursor position is unchanged after a delete, and subsequent
        /// calls to cursor functions expecting the cursor to refer to an
        /// existing key will fail.
        /// </para>
        /// </remarks>
        /// <exception cref="KeyEmptyException">
        /// The element has already been deleted.
        /// </exception>
        public new void Delete() {
            base.Delete();
            Current =
                new KeyValuePair<DatabaseEntry,
                KeyValuePair<DatabaseEntry, DatabaseEntry>>(
                null, new KeyValuePair<DatabaseEntry, DatabaseEntry>());
        }

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
        /// <see cref="BaseDatabase.Cursor"/>.</param>
        /// <returns>A newly created cursor</returns>
        public SecondaryCursor Duplicate(bool keepPosition) {
            return new SecondaryCursor(
                dbc.dup(keepPosition ? DbConstants.DB_POSITION : 0));
        }

        IEnumerator IEnumerable.GetEnumerator() {
            return GetEnumerator();
        }

        /// <summary>
        /// Returns an enumerator that iterates through the
        /// <see cref="SecondaryCursor"/>.
        /// </summary>
        /// <remarks>
        /// The enumerator will begin at the cursor's current position (or the
        /// first record if the cursor has not yet been positioned) and iterate 
        /// forwards (i.e. in the direction of <see cref="MoveNext"/>) over the
        /// remaining records.
        /// </remarks>
        /// <returns>An enumerator for the SecondaryCursor.</returns>
        public new IEnumerator<KeyValuePair<DatabaseEntry,
            KeyValuePair<DatabaseEntry, DatabaseEntry>>> GetEnumerator() {
            while (MoveNext())
                yield return Current;
        }

        /// <summary>
        /// Set the cursor to refer to the first key/data pair of the database, 
        /// and store the secondary key along with the corresponding primary
        /// key/data pair in <see cref="Current"/>. If the first key has
        /// duplicate values, the first data item in the set of duplicates is
        /// stored in <see cref="Current"/>.
        /// </summary>
        /// <remarks>
        /// If positioning the cursor fails, <see cref="Current"/> will contain
        /// an empty <see cref="KeyValuePair{T,T}"/>.
        /// </remarks>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveFirst() { return MoveFirst(null); }
        /// <summary>
        /// Set the cursor to refer to the first key/data pair of the database, 
        /// and store the secondary key along with the corresponding primary
        /// key/data pair in <see cref="Current"/>. If the first key has
        /// duplicate values, the first data item in the set of duplicates is
        /// stored in <see cref="Current"/>.
        /// </summary>
        /// <remarks>
        /// If positioning the cursor fails, <see cref="Current"/> will contain
        /// an empty <see cref="KeyValuePair{T,T}"/>.
        /// </remarks>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveFirst(LockingInfo info) {
            DatabaseEntry key = new DatabaseEntry();
            DatabaseEntry pkey = new DatabaseEntry();
            DatabaseEntry data = new DatabaseEntry();

            return PGet(key, pkey, data, DbConstants.DB_FIRST, info);
        }

        /// <summary>
        /// Set the cursor to refer to <paramref name="key"/>, and store the
        /// primary key/data pair associated with the given secondary key in
        /// <see cref="Current"/>. In the presence of duplicate key values, the
        /// first data item in the set of duplicates is stored in
        /// <see cref="Current"/>.
        /// </summary>
        /// <remarks>
        /// If positioning the cursor fails, <see cref="Current"/> will contain
        /// an empty <see cref="KeyValuePair{T,T}"/>.
        /// </remarks>
        /// <param name="key">The key at which to position the cursor</param>
        /// <param name="exact">
        /// If true, require the given key to match the key in the database
        /// exactly.  If false, position the cursor at the smallest key greater
        /// than or equal to the specified key, permitting partial key matches
        /// and range searches.
        /// </param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool Move(DatabaseEntry key, bool exact) {
            return Move(key, exact, null);
        }
        /// <summary>
        /// Set the cursor to refer to <paramref name="key"/>, and store the
        /// primary key/data pair associated with the given secondary key in
        /// <see cref="Current"/>. In the presence of duplicate key values, the
        /// first data item in the set of duplicates is stored in
        /// <see cref="Current"/>.
        /// </summary>
        /// <remarks>
        /// If positioning the cursor fails, <see cref="Current"/> will contain
        /// an empty <see cref="KeyValuePair{T,T}"/>.
        /// </remarks>
        /// <param name="key">The key at which to position the cursor</param>
        /// <param name="exact">
        /// If true, require the given key to match the key in the database
        /// exactly.  If false, position the cursor at the smallest key greater
        /// than or equal to the specified key, permitting partial key matches
        /// and range searches.
        /// </param>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool Move(DatabaseEntry key, bool exact, LockingInfo info) {
            DatabaseEntry pkey = new DatabaseEntry();
            DatabaseEntry data = new DatabaseEntry();

            return PGet(key, pkey, data,
                exact ? DbConstants.DB_SET : DbConstants.DB_SET_RANGE, info);
        }

        /// <summary>
        /// Move the cursor to the specified key/data pair of the database. The
        /// cursor is positioned to a key/data pair if both the key and data
        /// match the values provided on the key and data parameters. 
        /// </summary>
        /// <remarks>
        /// <para>
        /// If positioning the cursor fails, <see cref="Current"/> will contain
        /// an empty <see cref="KeyValuePair{T,T}"/>.
        /// </para>
        /// <para>
        /// If this flag is specified on a database configured without sorted
        /// duplicate support, the value of <paramref name="exact"/> is ignored.
        /// </para>
        /// </remarks>
        /// <param name="pair">
        /// The key/data pair at which to position the cursor.
        /// </param>
        /// <param name="exact">
        /// If true, require the given key and data to match the key and data
        /// in the database exactly.  If false, position the cursor at the
        /// smallest data value which is greater than or equal to the value
        /// provided by <paramref name="pair.Value"/> (as determined by the
        /// comparison function).
        /// </param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool Move(KeyValuePair<DatabaseEntry,
            KeyValuePair<DatabaseEntry, DatabaseEntry>> pair, bool exact) {
            return Move(pair, exact, null);
        }
        /// <summary>
        /// Move the cursor to the specified key/data pair of the database. The
        /// cursor is positioned to a key/data pair if both the key and data
        /// match the values provided on the key and data parameters. 
        /// </summary>
        /// <remarks>
        /// <para>
        /// If positioning the cursor fails, <see cref="Current"/> will contain
        /// an empty <see cref="KeyValuePair{T,T}"/>.
        /// </para>
        /// <para>
        /// If this flag is specified on a database configured without sorted
        /// duplicate support, the value of <paramref name="exact"/> is ignored.
        /// </para>
        /// </remarks>
        /// <param name="pair">
        /// The key/data pair at which to position the cursor.
        /// </param>
        /// <param name="exact">
        /// If true, require the given key and data to match the key and data
        /// in the database exactly.  If false, position the cursor at the
        /// smallest data value which is greater than or equal to the value
        /// provided by <paramref name="pair.Value"/> (as determined by the
        /// comparison function).
        /// </param>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool Move(KeyValuePair<DatabaseEntry,
            KeyValuePair<DatabaseEntry, DatabaseEntry>> pair,
            bool exact, LockingInfo info) {
            return PGet(pair.Key, pair.Value.Key, pair.Value.Value, exact ?
                DbConstants.DB_GET_BOTH : DbConstants.DB_GET_BOTH_RANGE, info);
        }

        /// <summary>
        /// Set the cursor to refer to the last key/data pair of the database, 
        /// and store the secondary key and primary key/data pair in
        /// <see cref="Current"/>. If the last key has duplicate values, the
        /// last data item in the set of duplicates is stored in
        /// <see cref="Current"/>.
        /// </summary>
        /// <remarks>
        /// If positioning the cursor fails, <see cref="Current"/> will contain
        /// an empty <see cref="KeyValuePair{T,T}"/>.
        /// </remarks>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveLast() { return MoveLast(null); }
        /// <summary>
        /// Set the cursor to refer to the last key/data pair of the database, 
        /// and store the secondary key and primary key/data pair in
        /// <see cref="Current"/>. If the last key has duplicate values, the
        /// last data item in the set of duplicates is stored in
        /// <see cref="Current"/>.
        /// </summary>
        /// <remarks>
        /// If positioning the cursor fails, <see cref="Current"/> will contain
        /// an empty <see cref="KeyValuePair{T,T}"/>.
        /// </remarks>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveLast(LockingInfo info) {
            DatabaseEntry key = new DatabaseEntry();
            DatabaseEntry pkey = new DatabaseEntry();
            DatabaseEntry data = new DatabaseEntry();

            return PGet(key, pkey, data, DbConstants.DB_LAST, info);
        }

        /// <summary>
        /// If the cursor is not yet initialized, MoveNext is identical to 
        /// <see cref="MoveFirst()"/>. Otherwise, move the cursor to the next
        /// key/data pair of the database, and store the secondary key and
        /// primary key/data pair in <see cref="Current"/>. In the presence of
        /// duplicate key values, the value of <see cref="Current">Current.Key
        /// </see> may not change. 
        /// </summary>
        /// <remarks>
        /// If positioning the cursor fails, <see cref="Current"/> will contain
        /// an empty <see cref="KeyValuePair{T,T}"/>.
        /// </remarks>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveNext() { return MoveNext(null); }
        /// <summary>
        /// If the cursor is not yet initialized, MoveNext is identical to 
        /// <see cref="MoveFirst()"/>. Otherwise, move the cursor to the next
        /// key/data pair of the database, and store the secondary key and
        /// primary key/data pair in <see cref="Current"/>. In the presence of
        /// duplicate key values, the value of <see cref="Current">Current.Key
        /// </see> may not change. 
        /// </summary>
        /// <remarks>
        /// If positioning the cursor fails, <see cref="Current"/> will contain
        /// an empty <see cref="KeyValuePair{T,T}"/>.
        /// </remarks>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveNext(LockingInfo info) {
            DatabaseEntry key = new DatabaseEntry();
            DatabaseEntry pkey = new DatabaseEntry();
            DatabaseEntry data = new DatabaseEntry();

            return PGet(key, pkey, data, DbConstants.DB_NEXT, info);
        }

        /// <summary>
        /// If the next key/data pair of the database is a duplicate data record
        /// for the current key/data pair, move the cursor to the next key/data
        /// pair in the database, and store the secondary key and primary
        /// key/data pair in <see cref="Current"/>. MoveNextDuplicate will
        /// return false if the next key/data pair of the database is not a
        /// duplicate data record for the current key/data pair.
        /// </summary>
        /// <remarks>
        /// If positioning the cursor fails, <see cref="Current"/> will contain
        /// an empty <see cref="KeyValuePair{T,T}"/>.
        /// </remarks>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveNextDuplicate() { return MoveNextDuplicate(null); }
        /// <summary>
        /// If the next key/data pair of the database is a duplicate data record
        /// for the current key/data pair, move the cursor to the next key/data
        /// pair in the database, and store the secondary key and primary
        /// key/data pair in <see cref="Current"/>. MoveNextDuplicate will
        /// return false if the next key/data pair of the database is not a
        /// duplicate data record for the current key/data pair.
        /// </summary>
        /// <remarks>
        /// If positioning the cursor fails, <see cref="Current"/> will contain
        /// an empty <see cref="KeyValuePair{T,T}"/>.
        /// </remarks>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveNextDuplicate(LockingInfo info) {
            DatabaseEntry key = new DatabaseEntry();
            DatabaseEntry pkey = new DatabaseEntry();
            DatabaseEntry data = new DatabaseEntry();

            return PGet(key, pkey, data, DbConstants.DB_NEXT_DUP, info);
        }

        /// <summary>
        /// If the cursor is not yet initialized, MoveNextUnique is identical to 
        /// <see cref="MoveFirst()"/>. Otherwise, move the cursor to the next
        /// non-duplicate key in the database, and store the secondary key and
        /// primary key/data pair in <see cref="Current"/>. MoveNextUnique will
        /// return false if no non-duplicate key/data pairs exist after the
        /// cursor position in the database. 
        /// </summary>
        /// <remarks>
        /// If positioning the cursor fails, <see cref="Current"/> will contain
        /// an empty <see cref="KeyValuePair{T,T}"/>.
        /// </remarks>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveNextUnique() { return MoveNextUnique(null); }
        /// <summary>
        /// If the cursor is not yet initialized, MoveNextUnique is identical to 
        /// <see cref="MoveFirst()"/>. Otherwise, move the cursor to the next
        /// non-duplicate key in the database, and store the secondary key and
        /// primary key/data pair in <see cref="Current"/>. MoveNextUnique will
        /// return false if no non-duplicate key/data pairs exist after the
        /// cursor position in the database. 
        /// </summary>
        /// <remarks>
        /// <para>
        /// If the database is a Queue or Recno database, MoveNextUnique will
        /// ignore any keys that exist but were never explicitly created by the
        /// application, or those that were created and later deleted.
        /// </para>
        /// <para>
        /// If positioning the cursor fails, <see cref="Current"/> will contain
        /// an empty <see cref="KeyValuePair{T,T}"/>.
        /// </para>
        /// </remarks>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveNextUnique(LockingInfo info) {
            DatabaseEntry key = new DatabaseEntry();
            DatabaseEntry pkey = new DatabaseEntry();
            DatabaseEntry data = new DatabaseEntry();

            return PGet(key, pkey, data, DbConstants.DB_NEXT_NODUP, info);
        }

        /// <summary>
        /// If the cursor is not yet initialized, MovePrev is identical to 
        /// <see cref="MoveLast()"/>. Otherwise, move the cursor to the previous
        /// key/data pair of the database, and store the secondary key and
        /// primary key/data pair in <see cref="Current"/>. In the presence of
        /// duplicate key values, the value of <see cref="Current">Current.Key
        /// </see> may not change. 
        /// </summary>
        /// <remarks>
        /// If positioning the cursor fails, <see cref="Current"/> will contain
        /// an empty <see cref="KeyValuePair{T,T}"/>.
        /// </remarks>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MovePrev() { return MovePrev(null); }
        /// <summary>
        /// If the cursor is not yet initialized, MovePrev is identical to 
        /// <see cref="MoveLast(LockingInfo)"/>. Otherwise, move the cursor to
        /// the previous key/data pair of the database, and store the secondary
        /// key and primary key/data pair in <see cref="Current"/>. In the
        /// presence of duplicate key values, the value of <see cref="Current">
        /// Current.Key</see> may not change. 
        /// </summary>
        /// <remarks>
        /// If positioning the cursor fails, <see cref="Current"/> will contain
        /// an empty <see cref="KeyValuePair{T,T}"/>.
        /// </remarks>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MovePrev(LockingInfo info) {
            DatabaseEntry key = new DatabaseEntry();
            DatabaseEntry pkey = new DatabaseEntry();
            DatabaseEntry data = new DatabaseEntry();

            return PGet(key, pkey, data, DbConstants.DB_PREV, info);
        }

        /// <summary>
        /// If the previous key/data pair of the database is a duplicate data
        /// record for the current key/data pair, the cursor is moved to the
        /// previous key/data pair of the database, and the secondary key and
        /// primary key/data pair in <see cref="Current"/>. MovePrevDuplicate
        /// will return false if the previous key/data pair of the database is
        /// not a duplicate data record for the current key/data pair.
        /// </summary>
        /// <remarks>
        /// If positioning the cursor fails, <see cref="Current"/> will contain
        /// an empty <see cref="KeyValuePair{T,T}"/>.
        /// </remarks>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MovePrevDuplicate() { return MovePrevDuplicate(null); }
        /// <summary>
        /// If the previous key/data pair of the database is a duplicate data
        /// record for the current key/data pair, the cursor is moved to the
        /// previous key/data pair of the database, and the secondary key and
        /// primary key/data pair in <see cref="Current"/>. MovePrevDuplicate
        /// will return false if the previous key/data pair of the database is
        /// not a duplicate data record for the current key/data pair.
        /// </summary>
        /// <remarks>
        /// If positioning the cursor fails, <see cref="Current"/> will contain
        /// an empty <see cref="KeyValuePair{T,T}"/>.
        /// </remarks>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MovePrevDuplicate(LockingInfo info) {
            DatabaseEntry key = new DatabaseEntry();
            DatabaseEntry pkey = new DatabaseEntry();
            DatabaseEntry data = new DatabaseEntry();

            return PGet(key, pkey, data, DbConstants.DB_PREV_DUP, info);
        }

        /// <summary>
        /// If the cursor is not yet initialized, MovePrevUnique is identical to 
        /// <see cref="MoveLast()"/>. Otherwise, move the cursor to the previous
        /// non-duplicate key in the database, and store the secondary key and
        /// primary key/data pair in <see cref="Current"/>. MovePrevUnique will
        /// return false if no non-duplicate key/data pairs exist after the 
        /// cursor position in the database. 
        /// </summary>
        /// <remarks>
        /// If positioning the cursor fails, <see cref="Current"/> will contain
        /// an empty <see cref="KeyValuePair{T,T}"/>.
        /// </remarks>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MovePrevUnique() { return MovePrevUnique(null); }
        /// <summary>
        /// If the cursor is not yet initialized, MovePrevUnique is identical to 
        /// <see cref="MoveLast(LockingInfo)"/>. Otherwise, move the cursor to
        /// the previous non-duplicate key in the database, and store the
        /// secondary key and primary key/data pair in <see cref="Current"/>.
        /// MovePrevUnique will return false if no non-duplicate key/data pairs
        /// exist after the cursor position in the database. 
        /// </summary>
        /// <remarks>
        /// If positioning the cursor fails, <see cref="Current"/> will contain
        /// an empty <see cref="KeyValuePair{T,T}"/>.
        /// </remarks>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MovePrevUnique(LockingInfo info) {
            DatabaseEntry key = new DatabaseEntry();
            DatabaseEntry pkey = new DatabaseEntry();
            DatabaseEntry data = new DatabaseEntry();

            return PGet(key, pkey, data, DbConstants.DB_PREV_NODUP, info);
        }

        /// <summary>
        /// Store the secondary key and primary key/data pair to which the
        /// cursor refers in <see cref="Current"/>.
        /// </summary>
        /// <remarks>
        /// If positioning the cursor fails, <see cref="Current"/> will contain
        /// an empty <see cref="KeyValuePair{T,T}"/>.
        /// </remarks>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool Refresh() { return Refresh(null); }
        /// <summary>
        /// Store the secondary key and primary key/data pair to which the
        /// cursor refers in <see cref="Current"/>.
        /// </summary>
        /// <remarks>
        /// If positioning the cursor fails, <see cref="Current"/> will contain
        /// an empty <see cref="KeyValuePair{T,T}"/>.
        /// </remarks>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool Refresh(LockingInfo info) {
            DatabaseEntry key = new DatabaseEntry();
            DatabaseEntry pkey = new DatabaseEntry();
            DatabaseEntry data = new DatabaseEntry();

            return PGet(key, pkey, data, DbConstants.DB_CURRENT, info);
        }
    }
}


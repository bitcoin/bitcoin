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
    /// A class representing database cursors, which allow for traversal of 
    /// database records.
    /// </summary>
    public class Cursor
        : BaseCursor,
        IDisposable, IEnumerable<KeyValuePair<DatabaseEntry, DatabaseEntry>> {
        private KeyValuePair<DatabaseEntry, DatabaseEntry> cur;
        private KeyValuePair<DatabaseEntry, MultipleDatabaseEntry> curMult;
        private MultipleKeyDatabaseEntry curMultKey;
        private DatabaseType dbtype;

        /// <summary>
        /// Protected member, storing the pagesize of the underlying database.
        /// Used during bulk get (i.e. Move*Multiple).
        /// </summary>
        protected uint pgsz;

        /// <summary>
        /// Specifies where to place duplicate data elements of the key to which
        /// the cursor refers.
        /// </summary>
        public enum InsertLocation {
            /// <summary>
            /// The new element appears immediately after the current cursor
            /// position.
            /// </summary>
            AFTER,
            /// <summary>
            /// The new element appears immediately before the current cursor
            /// position.
            /// </summary>
            BEFORE,
            /// <summary>
            /// The new element appears as the first of the data items for the 
            /// given key
            /// </summary>
            FIRST,
            /// <summary>
            /// The new element appears as the last of the data items for the 
            /// given key
            /// </summary>
            LAST
        };

        /// <summary>
        /// The key/data pair at which the cursor currently points.
        /// </summary>
        /// <remarks>
        /// Only one of <see cref="Current"/>, <see cref="CurrentMultiple"/> and
        /// <see cref="CurrentMultipleKey"/> will ever be non-empty.
        /// </remarks>
        public KeyValuePair<DatabaseEntry, DatabaseEntry> Current {
            private set {
                cur = value;
                curMult =
                    new KeyValuePair<DatabaseEntry, MultipleDatabaseEntry>();
                curMultKey = null;
            }
            get { return cur; }
        }
        /// <summary>
        /// The key and multiple data items at which the cursor currently
        /// points.
        /// </summary>
        /// <remarks>
        /// Only one of <see cref="Current"/>, <see cref="CurrentMultiple"/> and
        /// <see cref="CurrentMultipleKey"/> will ever be non-empty.
        /// </remarks>
        public
            KeyValuePair<DatabaseEntry, MultipleDatabaseEntry> CurrentMultiple {
            private set {
                cur = new KeyValuePair<DatabaseEntry, DatabaseEntry>();
                curMult = value;
                curMultKey = null;
            }
            get { return curMult; }
        }
        /// <summary>
        /// The multiple key and data items at which the cursor currently
        /// points.
        /// </summary>
        /// <remarks>
        /// Only one of <see cref="Current"/>, <see cref="CurrentMultiple"/> and
        /// <see cref="CurrentMultipleKey"/> will ever be non-empty.
        /// </remarks>
        public MultipleKeyDatabaseEntry CurrentMultipleKey {
            private set {
                cur = new KeyValuePair<DatabaseEntry, DatabaseEntry>();
                curMult =
                    new KeyValuePair<DatabaseEntry, MultipleDatabaseEntry>();
                curMultKey = value;
            }
            get { return curMultKey; }
        }
        private CachePriority _priority;
        /// <summary>
        /// The cache priority for pages referenced by the cursor.
        /// </summary>
        /// <remarks>
        /// The priority of a page biases the replacement algorithm to be more
        /// or less likely to discard a page when space is needed in the buffer
        /// pool. The bias is temporary, and pages will eventually be discarded
        /// if they are not referenced again. The setting is only advisory, and
        /// does not guarantee pages will be treated in a specific way.
        /// </remarks>
        public CachePriority Priority {
            get { return _priority; }
            set {
                dbc.set_priority(value.priority);
                _priority = value;
            }
        }

        internal Cursor(DBC dbc, DatabaseType DbType, uint pagesize)
            : base(dbc) {
            pgsz = pagesize;
            dbtype = DbType;
        }
        internal Cursor(
            DBC dbc, DatabaseType DbType, uint pagesize, CachePriority pri)
            : base(dbc) {
            Priority = pri;
            pgsz = pagesize;
            dbtype = DbType;
        }

        #region Internal API
        /* These protected methods do the heavy lifting.  The API methods for
         * Cursor and its subclasses call into them, which allows the API 
         * methods to expose subsets of the arg lists, because some args are
         * optional. */

        /* Only BTree and Hash can call this version of Add(). */
        /// <summary>
        /// Protected method for BTree and Hash to insert with KEYFIRST and
        /// KEYLAST.
        /// </summary>
        /// <param name="pair">The key/data pair to add</param>
        /// <param name="loc">Where to add, if adding duplicate data</param>
        protected void Add(KeyValuePair<DatabaseEntry, DatabaseEntry> pair, InsertLocation loc) {
            if (loc == InsertLocation.AFTER)
                throw new ArgumentException("AFTER may only be specified on Insert().");
            if (loc == InsertLocation.BEFORE)
                throw new ArgumentException("BEFORE may only be specified on Insert().");
            Put(pair.Key, pair.Value, (loc == InsertLocation.FIRST) ? DbConstants.DB_KEYFIRST : DbConstants.DB_KEYLAST);
        }
        /* Only BTree and Hash can call AddUnique(). */
        /// <summary>
        /// Protected method for BTree and Hash to insert with NODUPDATA.
        /// </summary>
        /// <param name="pair">The key/data pair to add</param>
        protected void AddUnique(KeyValuePair<DatabaseEntry, DatabaseEntry> pair) {
            Put(pair.Key, pair.Value, DbConstants.DB_NODUPDATA);
        }
        /* Only BTree, Hash and Recno can call Insert(). */
        /// <summary>
        /// Protected method for BTree, Hash and Recno to insert with AFTER and
        /// BEFORE.
        /// </summary>
        /// <param name="data">The duplicate data item to add</param>
        /// <param name="loc">
        /// Whether to add the dup data before or after the current cursor
        /// position
        /// </param>
        protected void Insert(DatabaseEntry data, InsertLocation loc) {
            if (loc == InsertLocation.FIRST)
                throw new ArgumentException("FIRST may only be specified on Add().");
            if (loc == InsertLocation.LAST)
                throw new ArgumentException("LAST may only be specified on Add().");
            DatabaseEntry key = new DatabaseEntry();
            Put(key, data, (loc == InsertLocation.AFTER) ? DbConstants.DB_AFTER : DbConstants.DB_BEFORE);
        }

        /* 
         * All flavors of get and put boil down to a call to one of these two
         * methods, just with different flags. 
         */
        /// <summary>
        /// Protected method wrapping DBC->get.
        /// </summary>
        /// <param name="key">The key to retrieve</param>
        /// <param name="data">The data to retrieve</param>
        /// <param name="flags">Modify the behavior of get</param>
        /// <param name="info">The locking configuration to use</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        protected bool Get(DatabaseEntry key, DatabaseEntry data, uint flags, LockingInfo info) {
            flags |= (info == null) ? 0 : info.flags;

            try {
                dbc.get(key, data, flags);
                Current = new KeyValuePair<DatabaseEntry, DatabaseEntry>(key, data);
                return true;
            } catch (NotFoundException) {
                Current = new KeyValuePair<DatabaseEntry, DatabaseEntry>();
                return false;
            }
        }
        /// <summary>
        /// Protected method wrapping DBC->get for bulk get.
        /// </summary>
        /// <param name="key">The key to retrieve</param>
        /// <param name="data">The data to retrieve</param>
        /// <param name="BufferSize">Size of the bulk buffer</param>
        /// <param name="flags">Modify the behavior of get</param>
        /// <param name="info">The locking configuration to use</param>
        /// <param name="isMultKey">
        /// If true, use DB_MULTIPLE_KEY instead of DB_MULTIPLE
        /// </param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        protected bool GetMultiple(DatabaseEntry key, DatabaseEntry data,
            int BufferSize, uint flags, LockingInfo info, bool isMultKey) {
            int datasz = 0;
            bool getboth = false;

            if (flags == DbConstants.DB_GET_BOTH ||
                flags == DbConstants.DB_GET_BOTH_RANGE) {
                datasz = (int)data.Data.Length;
                getboth = true;
            }
            flags |= (info == null) ? 0 : info.flags;
            flags |= (isMultKey) ?
                DbConstants.DB_MULTIPLE_KEY : DbConstants.DB_MULTIPLE;

            for (; ; ) {
                if (getboth) {
                    byte[] udata = new byte[BufferSize];
                    Array.Copy(data.Data, udata, datasz);
                    data.UserData = udata;
                    data.size = (uint)datasz;
                } else {
                    data.UserData = new byte[BufferSize];
                }

                try {
                    dbc.get(key, data, flags);
                    if (isMultKey)
                        CurrentMultipleKey =
                            new MultipleKeyDatabaseEntry(dbtype, data);
                    else {
                        MultipleDatabaseEntry mult =
                            new MultipleDatabaseEntry(data);
                        CurrentMultiple = new
                            KeyValuePair<DatabaseEntry, MultipleDatabaseEntry>(
                            key, mult);
                    }
                    return true;
                } catch (NotFoundException) {
                    if (isMultKey)
                        CurrentMultipleKey = null;
                    else
                        CurrentMultiple = new
                           KeyValuePair<DatabaseEntry, MultipleDatabaseEntry>();
                    return false;
                } catch (MemoryException) {
                    int sz = (int)data.size;
                    if (sz > BufferSize)
                        BufferSize = sz;
                    else
                        BufferSize *= 2;
                }
            }
        }

        /// <summary>
        /// Protected method wrapping DBC->put.
        /// </summary>
        /// <param name="key">The key to store</param>
        /// <param name="data">The data to store</param>
        /// <param name="flags">Modify the behavior of put</param>
        protected void Put(DatabaseEntry key, DatabaseEntry data, uint flags) {
            int ret;
            ret = dbc.put(key, data, flags);
        }
        #endregion Internal API

        /* 
         * User facing API below.  These methods just set the flags as needed
         * before calling Get or Put. 
         */

        /// <summary>
        /// Stores the key/data pair in the database.  
        /// </summary>
        /// <remarks>
        /// If the underlying database supports duplicate data items, and if the
        /// key already exists in the database and a duplicate sort function has
        /// been specified, the inserted data item is added in its sorted
        /// location. If the key already exists in the database and no duplicate
        /// sort function has been specified, the inserted data item is added as
        /// the first of the data items for that key. 
        /// </remarks>
        /// <param name="pair">
        /// The key/data pair to be stored in the database.
        /// </param>
        public void Add(KeyValuePair<DatabaseEntry, DatabaseEntry> pair) {
            Put(pair.Key, pair.Value, DbConstants.DB_KEYFIRST);
        }

        /// <summary>
        /// Delete the key/data pair to which the cursor refers.
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
            Current = new KeyValuePair<DatabaseEntry, DatabaseEntry>();
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
        public Cursor Duplicate(bool keepPosition) {
            return new Cursor(dbc.dup(
                keepPosition ? DbConstants.DB_POSITION : 0), dbtype, pgsz);
        }

        IEnumerator IEnumerable.GetEnumerator() {
            return GetEnumerator();
        }

        /// <summary>
        /// Returns an enumerator that iterates through the
        /// <see cref="Cursor"/>.
        /// </summary>
        /// <remarks>
        /// The enumerator will begin at the cursor's current position (or the
        /// first record if the cursor has not yet been positioned) and iterate 
        /// forwards (i.e. in the direction of <see cref="MoveNext"/>) over the
        /// remaining records.
        /// </remarks>
        /// <returns>An enumerator for the Cursor.</returns>
        public new IEnumerator<KeyValuePair<DatabaseEntry, DatabaseEntry>>
            GetEnumerator() {
            while (MoveNext())
                yield return Current;
        }

        /// <summary>
        /// Set the cursor to refer to the first key/data pair of the database, 
        /// and store that pair in <see cref="Current"/>. If the first key has
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
        /// and store that pair in <see cref="Current"/>. If the first key has
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
            DatabaseEntry data = new DatabaseEntry();

            return Get(key, data, DbConstants.DB_FIRST, info);
        }

        /// <summary>
        /// Set the cursor to refer to the first key/data pair of the database, 
        /// and store that key and as many duplicate data items that can fit in
        /// a buffer the size of one database page in
        /// <see cref="CurrentMultiple"/>.
        /// </summary>
        /// <overloads>
        /// If positioning the cursor fails, <see cref="CurrentMultiple"/> will
        /// contain an empty
        /// <see cref="KeyValuePair{T,T}"/>.
        /// </overloads>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveFirstMultiple() {
            return MoveFirstMultiple((int)pgsz, null);
        }
        /// <summary>
        /// Set the cursor to refer to the first key/data pair of the database, 
        /// and store that key and as many duplicate data items that can fit in
        /// a buffer the size of <paramref name="BufferSize"/> in
        /// <see cref="CurrentMultiple"/>.
        /// </summary>
        /// <param name="BufferSize">
        /// The size of a buffer to fill with duplicate data items.  Must be at
        /// least the page size of the underlying database and be a multiple of
        /// 1024.
        /// </param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveFirstMultiple(int BufferSize) {
            return MoveFirstMultiple(BufferSize, null);
        }
        /// <summary>
        /// Set the cursor to refer to the first key/data pair of the database, 
        /// and store that key and as many duplicate data items that can fit in
        /// a buffer the size of one database page in
        /// <see cref="CurrentMultiple"/>.
        /// </summary>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveFirstMultiple(LockingInfo info) {
            return MoveFirstMultiple((int)pgsz, info);
        }
        /// <summary>
        /// Set the cursor to refer to the first key/data pair of the database, 
        /// and store that key and as many duplicate data items that can fit in
        /// a buffer the size of <paramref name="BufferSize"/> in
        /// <see cref="CurrentMultiple"/>.
        /// </summary>
        /// <param name="BufferSize">
        /// The size of a buffer to fill with duplicate data items.  Must be at
        /// least the page size of the underlying database and be a multiple of
        /// 1024.
        /// </param>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveFirstMultiple(int BufferSize, LockingInfo info) {
            DatabaseEntry key = new DatabaseEntry();
            DatabaseEntry data = new DatabaseEntry();

            return GetMultiple(
                key, data, BufferSize, DbConstants.DB_FIRST, info, false);
        }

        /// <summary>
        /// Set the cursor to refer to the first key/data pair of the database, 
        /// and store that pair and as many ensuing key/data pairs that can fit
        /// in a buffer the size of one database page in
        /// <see cref="CurrentMultipleKey"/>.
        /// </summary>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveFirstMultipleKey() {
            return MoveFirstMultipleKey((int)pgsz, null);
        }
        /// <summary>
        /// Set the cursor to refer to the first key/data pair of the database, 
        /// and store that pair and as many ensuing key/data pairs that can fit
        /// in a buffer the size of <paramref name="BufferSize"/> in
        /// <see cref="CurrentMultipleKey"/>.
        /// </summary>
        /// <param name="BufferSize">
        /// The size of a buffer to fill with key/data pairs.  Must be at least
        /// the page size of the underlying database and be a multiple of 1024.
        /// </param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveFirstMultipleKey(int BufferSize) {
            return MoveFirstMultipleKey(BufferSize, null);
        }
        /// <summary>
        /// Set the cursor to refer to the first key/data pair of the database, 
        /// and store that pair and as many ensuing key/data pairs that can fit
        /// in a buffer the size of one database page in
        /// <see cref="CurrentMultipleKey"/>.
        /// </summary>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveFirstMultipleKey(LockingInfo info) {
            return MoveFirstMultipleKey((int)pgsz, info);
        }
        /// <summary>
        /// Set the cursor to refer to the first key/data pair of the database, 
        /// and store that pair and as many ensuing key/data pairs that can fit
        /// in a buffer the size of <paramref name="BufferSize"/> in
        /// <see cref="CurrentMultipleKey"/>.
        /// </summary>
        /// <param name="BufferSize">
        /// The size of a buffer to fill with key/data pairs.  Must be at least
        /// the page size of the underlying database and be a multiple of 1024.
        /// </param>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveFirstMultipleKey(int BufferSize, LockingInfo info) {
            DatabaseEntry key = new DatabaseEntry();
            DatabaseEntry data = new DatabaseEntry();

            return GetMultiple(
                key, data, BufferSize, DbConstants.DB_FIRST, info, true);
        }

        /// <summary>
        /// Set the cursor to refer to <paramref name="key"/>, and store the
        /// datum associated with the given key in <see cref="Current"/>. In the
        /// presence of duplicate key values, the first data item in the set of
        /// duplicates is stored in <see cref="Current"/>.
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
        /// datum associated with the given key in <see cref="Current"/>. In the
        /// presence of duplicate key values, the first data item in the set of
        /// duplicates is stored in <see cref="Current"/>.
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
            DatabaseEntry data = new DatabaseEntry();

            return Get(key, data,
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
        public bool Move(
            KeyValuePair<DatabaseEntry, DatabaseEntry> pair, bool exact) {
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
        public bool Move(KeyValuePair<DatabaseEntry, DatabaseEntry> pair,
            bool exact, LockingInfo info) {
            return Get(pair.Key, pair.Value, exact ?
                DbConstants.DB_GET_BOTH : DbConstants.DB_GET_BOTH_RANGE, info);
        }

        /// <summary>
        /// Set the cursor to refer to the last key/data pair of the database, 
        /// and store that pair in <see cref="Current"/>. If the last key has
        /// duplicate values, the last data item in the set of duplicates is
        /// stored in <see cref="Current"/>.
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
        /// and store that pair in <see cref="Current"/>. If the last key has
        /// duplicate values, the last data item in the set of duplicates is
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
        public bool MoveLast(LockingInfo info) {
            DatabaseEntry key = new DatabaseEntry();
            DatabaseEntry data = new DatabaseEntry();

            return Get(key, data, DbConstants.DB_LAST, info);
        }

        /// <summary>
        /// Set the cursor to refer to <paramref name="key"/>, and store that 
        /// key and as many duplicate data items associated with the given key that
        /// can fit in a buffer the size of one database page in
        /// <see cref="CurrentMultiple"/>.
        /// </summary>
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
        public bool MoveMultiple(DatabaseEntry key, bool exact) {
            return MoveMultiple(key, exact, (int)pgsz, null);
        }
        /// <summary>
        /// Set the cursor to refer to <paramref name="key"/>, and store that 
        /// key and as many duplicate data items associated with the given key that
        /// can fit in a buffer the size of <paramref name="BufferSize"/> in
        /// <see cref="CurrentMultiple"/>.
        /// </summary>
        /// <param name="key">The key at which to position the cursor</param>
        /// <param name="exact">
        /// If true, require the given key to match the key in the database
        /// exactly.  If false, position the cursor at the smallest key greater
        /// than or equal to the specified key, permitting partial key matches
        /// and range searches.
        /// </param>
        /// <param name="BufferSize">
        /// The size of a buffer to fill with duplicate data items.  Must be at
        /// least the page size of the underlying database and be a multiple of
        /// 1024.
        /// </param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveMultiple(
            DatabaseEntry key, bool exact, int BufferSize) {
            return MoveMultiple(key, exact, BufferSize, null);
        }
        /// <summary>
        /// Set the cursor to refer to <paramref name="key"/>, and store that 
        /// key and as many duplicate data items associated with the given key that
        /// can fit in a buffer the size of one database page in
        /// <see cref="CurrentMultiple"/>.
        /// </summary>
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
        public bool MoveMultiple(
            DatabaseEntry key, bool exact, LockingInfo info) {
            return MoveMultiple(key, exact, (int)pgsz, info);
        }
        /// <summary>
        /// Set the cursor to refer to <paramref name="key"/>, and store that 
        /// key and as many duplicate data items associated with the given key that
        /// can fit in a buffer the size of <paramref name="BufferSize"/> in
        /// <see cref="CurrentMultiple"/>.
        /// </summary>
        /// <param name="key">The key at which to position the cursor</param>
        /// <param name="exact">
        /// If true, require the given key to match the key in the database
        /// exactly.  If false, position the cursor at the smallest key greater
        /// than or equal to the specified key, permitting partial key matches
        /// and range searches.
        /// </param>
        /// <param name="BufferSize">
        /// The size of a buffer to fill with duplicate data items.  Must be at
        /// least the page size of the underlying database and be a multiple of
        /// 1024.
        /// </param>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveMultiple(
            DatabaseEntry key, bool exact, int BufferSize, LockingInfo info) {
            DatabaseEntry data = new DatabaseEntry();

            return GetMultiple(key, data, BufferSize, (exact ?
                DbConstants.DB_SET : DbConstants.DB_SET_RANGE), info, false);
        }

        /// <summary>
        /// Move the cursor to the specified key/data pair of the database, and
        /// store that key/data pair and as many duplicate data items associated
        /// with the given key that can fit in a buffer the size of one database
        /// page in <see cref="CurrentMultiple"/>. The cursor is positioned to a
        /// key/data pair if both the key and data match the values provided on
        /// the key and data parameters. 
        /// </summary>
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
        public bool MoveMultiple(
            KeyValuePair<DatabaseEntry, DatabaseEntry> pair, bool exact) {
            return MoveMultiple(pair, exact, (int)pgsz, null);
        }
        /// <summary>
        /// Move the cursor to the specified key/data pair of the database, and
        /// store that key/data pair and as many duplicate data items associated
        /// with the given key that can fit in a buffer the size of
        /// <paramref name="BufferSize"/> in <see cref="CurrentMultiple"/>. The
        /// cursor is positioned to a key/data pair if both the key and data
        /// match the values provided on the key and data parameters. 
        /// </summary>
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
        /// <param name="BufferSize">
        /// The size of a buffer to fill with duplicate data items.  Must be at
        /// least the page size of the underlying database and be a multiple of
        /// 1024.
        /// </param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveMultiple(
            KeyValuePair<DatabaseEntry, DatabaseEntry> pair,
            bool exact, int BufferSize) {
            return MoveMultiple(pair, exact, BufferSize, null);
        }
        /// <summary>
        /// Move the cursor to the specified key/data pair of the database, and
        /// store that key/data pair and as many duplicate data items associated
        /// with the given key that can fit in a buffer the size of one database
        /// page in <see cref="CurrentMultiple"/>. The cursor is positioned to a
        /// key/data pair if both the key and data match the values provided on
        /// the key and data parameters. 
        /// </summary>
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
        public bool MoveMultiple(
            KeyValuePair<DatabaseEntry, DatabaseEntry> pair,
            bool exact, LockingInfo info) {
            return MoveMultiple(pair, exact, (int)pgsz, info);
        }
        /// <summary>
        /// Move the cursor to the specified key/data pair of the database, and
        /// store that key/data pair and as many duplicate data items associated
        /// with the given key that can fit in a buffer the size of
        /// <paramref name="BufferSize"/> in <see cref="CurrentMultiple"/>. The
        /// cursor is positioned to a key/data pair if both the key and data
        /// match the values provided on the key and data parameters. 
        /// </summary>
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
        /// <param name="BufferSize">
        /// The size of a buffer to fill with duplicate data items.  Must be at
        /// least the page size of the underlying database and be a multiple of
        /// 1024.
        /// </param>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveMultiple(
            KeyValuePair<DatabaseEntry, DatabaseEntry> pair,
            bool exact, int BufferSize, LockingInfo info) {
            return GetMultiple(pair.Key, pair.Value, BufferSize, (exact ?
                DbConstants.DB_GET_BOTH : DbConstants.DB_GET_BOTH_RANGE),
                info, false);
        }

        /// <summary>
        /// Set the cursor to refer to <paramref name="key"/>, and store that 
        /// key and as many ensuing key/data pairs that can fit in a buffer the
        /// size of one database page in <see cref="CurrentMultipleKey"/>.
        /// </summary>
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
        public bool MoveMultipleKey(DatabaseEntry key, bool exact) {
            return MoveMultipleKey(key, exact, (int)pgsz, null);
        }
        /// <summary>
        /// Set the cursor to refer to <paramref name="key"/>, and store that 
        /// key and as many ensuing key/data pairs that can fit in a buffer the
        /// size of <paramref name="BufferSize"/> in
        /// <see cref="CurrentMultipleKey"/>.
        /// </summary>
        /// <param name="key">The key at which to position the cursor</param>
        /// <param name="exact">
        /// If true, require the given key to match the key in the database
        /// exactly.  If false, position the cursor at the smallest key greater
        /// than or equal to the specified key, permitting partial key matches
        /// and range searches.
        /// </param>
        /// <param name="BufferSize">
        /// The size of a buffer to fill with key/data pairs.  Must be at least
        /// the page size of the underlying database and be a multiple of 1024.
        /// </param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveMultipleKey(
            DatabaseEntry key, bool exact, int BufferSize) {
            return MoveMultipleKey(key, exact, BufferSize, null);
        }
        /// <summary>
        /// Set the cursor to refer to <paramref name="key"/>, and store that 
        /// key and as many ensuing key/data pairs that can fit in a buffer the
        /// size of one database page in <see cref="CurrentMultipleKey"/>.
        /// </summary>
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
        public bool MoveMultipleKey(
            DatabaseEntry key, bool exact, LockingInfo info) {
            return MoveMultipleKey(key, exact, (int)pgsz, info);
        }
        /// <summary>
        /// Set the cursor to refer to <paramref name="key"/>, and store that 
        /// key and as many ensuing key/data pairs that can fit in a buffer the
        /// size of <paramref name="BufferSize"/> in
        /// <see cref="CurrentMultipleKey"/>.
        /// </summary>
        /// <param name="key">The key at which to position the cursor</param>
        /// <param name="exact">
        /// If true, require the given key to match the key in the database
        /// exactly.  If false, position the cursor at the smallest key greater
        /// than or equal to the specified key, permitting partial key matches
        /// and range searches.
        /// </param>
        /// <param name="BufferSize">
        /// The size of a buffer to fill with key/data pairs.  Must be at least
        /// the page size of the underlying database and be a multiple of 1024.
        /// </param>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveMultipleKey(
            DatabaseEntry key, bool exact, int BufferSize, LockingInfo info) {
            DatabaseEntry data = new DatabaseEntry();

            return GetMultiple(key, data, BufferSize,
                (exact ?
                DbConstants.DB_SET : DbConstants.DB_SET_RANGE), info, true);
        }

        /// <summary>
        /// Move the cursor to the specified key/data pair of the database, and
        /// store that key/data pair and as many ensuing key/data pairs that can
        /// fit in a buffer the size of one database page in
        /// <see cref="CurrentMultipleKey"/>. The cursor is positioned to a
        /// key/data pair if both the key and data match the values provided on
        /// the key and data parameters. 
        /// </summary>
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
        public bool MoveMultipleKey(
            KeyValuePair<DatabaseEntry, DatabaseEntry> pair, bool exact) {
            return MoveMultipleKey(pair, exact, (int)pgsz, null);
        }
        /// <summary>
        /// Move the cursor to the specified key/data pair of the database, and
        /// store that key/data pair and as many ensuing key/data pairs that can
        /// fit in a buffer the size of <paramref name="BufferSize"/> in
        /// <see cref="CurrentMultipleKey"/>. The cursor is positioned to a
        /// key/data pair if both the key and data match the values provided on
        /// the key and data parameters. 
        /// </summary>
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
        /// <param name="BufferSize">
        /// The size of a buffer to fill with key/data pairs.  Must be at least
        /// the page size of the underlying database and be a multiple of 1024.
        /// </param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveMultipleKey(
            KeyValuePair<DatabaseEntry, DatabaseEntry> pair,
            bool exact, int BufferSize) {
            return MoveMultipleKey(pair, exact, BufferSize, null);
        }
        /// <summary>
        /// Move the cursor to the specified key/data pair of the database, and
        /// store that key/data pair and as many ensuing key/data pairs that can
        /// fit in a buffer the size of one database page in
        /// <see cref="CurrentMultipleKey"/>. The cursor is positioned to a
        /// key/data pair if both the key and data match the values provided on
        /// the key and data parameters. 
        /// </summary>
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
        public bool MoveMultipleKey(
            KeyValuePair<DatabaseEntry, DatabaseEntry> pair,
            bool exact, LockingInfo info) {
            return MoveMultipleKey(pair, exact, (int)pgsz, info);
        }
        /// <summary>
        /// Move the cursor to the specified key/data pair of the database, and
        /// store that key/data pair and as many ensuing key/data pairs that can
        /// fit in a buffer the size of <paramref name="BufferSize"/> in
        /// <see cref="CurrentMultipleKey"/>. The cursor is positioned to a
        /// key/data pair if both the key and data match the values provided on
        /// the key and data parameters. 
        /// </summary>
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
        /// <param name="BufferSize">
        /// The size of a buffer to fill with key/data pairs.  Must be at least
        /// the page size of the underlying database and be a multiple of 1024.
        /// </param>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveMultipleKey(
            KeyValuePair<DatabaseEntry, DatabaseEntry> pair,
            bool exact, int BufferSize, LockingInfo info) {
            return GetMultiple(pair.Key, pair.Value, BufferSize, (exact ?
                DbConstants.DB_GET_BOTH : DbConstants.DB_GET_BOTH_RANGE),
                info, true);
        }

        /// <summary>
        /// If the cursor is not yet initialized, MoveNext is identical to 
        /// <see cref="MoveFirst()"/>. Otherwise, move the cursor to the next
        /// key/data pair of the database, and store that pair in
        /// <see cref="Current"/>. In the presence of duplicate key values, the
        /// value of <see cref="Current">Current.Key</see> may not change. 
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
        /// <see cref="MoveFirst(LockingInfo)"/>. Otherwise, move the cursor to
        /// the next key/data pair of the database, and store that pair in
        /// <see cref="Current"/>. In the presence of duplicate key values, the
        /// value of <see cref="Current">Current.Key</see> may not change. 
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
            DatabaseEntry data = new DatabaseEntry();

            return Get(key, data, DbConstants.DB_NEXT, info);
        }

        /// <summary>
        /// If the cursor is not yet initialized, MoveNextMultiple is identical
        /// to <see cref="MoveFirstMultiple()"/>. Otherwise, move the cursor to
        /// the next key/data pair of the database, and store that pair and as
        /// many duplicate data items that can fit in a buffer the size of one
        /// database page in <see cref="CurrentMultiple"/>. In the presence of
        /// duplicate key values, the value of
        /// <see cref="CurrentMultiple">CurrentMultiple.Key</see> may not
        /// change. 
        /// </summary>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveNextMultiple() {
            return MoveNextMultiple((int)pgsz, null);
        }
        /// <summary>
        /// If the cursor is not yet initialized, MoveNextMultiple is identical
        /// to <see cref="MoveFirstMultiple(int)"/>. Otherwise, move the cursor
        /// to the next key/data pair of the database, and store that pair and
        /// as many duplicate data items that can fit in a buffer the size of
        /// <paramref name="BufferSize"/> in <see cref="CurrentMultiple"/>. In
        /// the presence of duplicate key values, the value of
        /// <see cref="CurrentMultiple">CurrentMultiple.Key</see> may not
        /// change. 
        /// </summary>
        /// <param name="BufferSize">
        /// The size of a buffer to fill with duplicate data items.  Must be at
        /// least the page size of the underlying database and be a multiple of
        /// 1024.
        /// </param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveNextMultiple(int BufferSize) {
            return MoveNextMultiple(BufferSize, null);
        }
        /// <summary>
        /// If the cursor is not yet initialized, MoveNextMultiple is identical
        /// to <see cref="MoveFirstMultiple(LockingInfo)"/>. Otherwise, move the
        /// cursor to the next key/data pair of the database, and store that
        /// pair and as many duplicate data items that can fit in a buffer the
        /// size of one database page in <see cref="CurrentMultiple"/>. In the
        /// presence of duplicate key values, the value of
        /// <see cref="CurrentMultiple">CurrentMultiple.Key</see> may not
        /// change. 
        /// </summary>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveNextMultiple(LockingInfo info) {
            return MoveNextMultiple((int)pgsz, null);
        }
        /// <summary>
        /// If the cursor is not yet initialized, MoveNextMultiple is identical
        /// to <see cref="MoveFirstMultiple(int, LockingInfo)"/>. Otherwise,
        /// move the cursor to the next key/data pair of the database, and store
        /// that pair and as many duplicate data items that can fit in a buffer
        /// the size of <paramref name="BufferSize"/> in
        /// <see cref="CurrentMultiple"/>. In the presence of duplicate key
        /// values, the value of
        /// <see cref="CurrentMultiple">CurrentMultiple.Key</see> may not
        /// change. 
        /// </summary>
        /// <param name="BufferSize">
        /// The size of a buffer to fill with duplicate data items.  Must be at
        /// least the page size of the underlying database and be a multiple of
        /// 1024.
        /// </param>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveNextMultiple(int BufferSize, LockingInfo info) {
            DatabaseEntry key = new DatabaseEntry();
            DatabaseEntry data = new DatabaseEntry();

            return GetMultiple(
                key, data, BufferSize, DbConstants.DB_NEXT, info, false);
        }

        /// <summary>
        /// If the cursor is not yet initialized, MoveNextMultipleKey is
        /// identical to <see cref="MoveFirstMultipleKey()"/>. Otherwise, move
        /// the cursor to the next key/data pair of the database, and store that
        /// pair and as many ensuing key/data pairs that can fit in a buffer the
        /// size of one database page in <see cref="CurrentMultipleKey"/>. In
        /// the presence of duplicate key values, the keys of
        /// <see cref="CurrentMultipleKey"/> may not change. 
        /// </summary>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveNextMultipleKey() {
            return MoveNextMultipleKey((int)pgsz, null);
        }
        /// <summary>
        /// If the cursor is not yet initialized, MoveNextMultipleKey is
        /// identical to <see cref="MoveFirstMultipleKey(int)"/>. Otherwise,
        /// move the cursor to the next key/data pair of the database, and store
        /// that pair and as many ensuing key/data pairs that can fit in a
        /// buffer the size of <paramref name="BufferSize"/> in
        /// <see cref="CurrentMultipleKey"/>. In the presence of duplicate key
        /// values, the keys of <see cref="CurrentMultipleKey"/> may not change. 
        /// </summary>
        /// <param name="BufferSize">
        /// The size of a buffer to fill with key/data pairs.  Must be at least
        /// the page size of the underlying database and be a multiple of 1024.
        /// </param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveNextMultipleKey(int BufferSize) {
            return MoveNextMultipleKey(BufferSize, null);
        }
        /// <summary>
        /// If the cursor is not yet initialized, MoveNextMultipleKey is
        /// identical to <see cref="MoveFirstMultipleKey(LockingInfo)"/>.
        /// Otherwise, move the cursor to the next key/data pair of the
        /// database, and store that pair and as many ensuing key/data pairs
        /// that can fit in a buffer the size of one database page in
        /// <see cref="CurrentMultipleKey"/>. In the presence of duplicate key
        /// values, the keys of <see cref="CurrentMultipleKey"/> may not change. 
        /// </summary>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveNextMultipleKey(LockingInfo info) {
            return MoveNextMultipleKey((int)pgsz, null);
        }
        /// <summary>
        /// If the cursor is not yet initialized, MoveNextMultipleKey is
        /// identical to <see cref="MoveFirstMultipleKey(int, LockingInfo)"/>.
        /// Otherwise, move the cursor to the next key/data pair of the
        /// database, and store that pair and as many ensuing key/data pairs
        /// that can fit in a buffer the size of <paramref name="BufferSize"/>
        /// in <see cref="CurrentMultipleKey"/>. In the presence of duplicate
        /// key values, the keys of <see cref="CurrentMultipleKey"/> may not
        /// change. 
        /// </summary>
        /// <param name="BufferSize">
        /// The size of a buffer to fill with key/data pairs.  Must be at least
        /// the page size of the underlying database and be a multiple of 1024.
        /// </param>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveNextMultipleKey(int BufferSize, LockingInfo info) {
            DatabaseEntry key = new DatabaseEntry();
            DatabaseEntry data = new DatabaseEntry();

            return GetMultiple(
                key, data, BufferSize, DbConstants.DB_NEXT, info, true);
        }

        /// <summary>
        /// If the next key/data pair of the database is a duplicate data record
        /// for the current key/data pair, move the cursor to the next key/data
        /// pair in the database, and store that pair in <see cref="Current"/>.
        /// MoveNextDuplicate will return false if the next key/data pair of the
        /// database is not a duplicate data record for the current key/data
        /// pair.
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
        /// pair in the database, and store that pair in <see cref="Current"/>.
        /// MoveNextDuplicate will return false if the next key/data pair of the
        /// database is not a duplicate data record for the current key/data
        /// pair.
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
            DatabaseEntry data = new DatabaseEntry();

            return Get(key, data, DbConstants.DB_NEXT_DUP, info);
        }

        /// <summary>
        /// If the next key/data pair of the database is a duplicate data record
        /// for the current key/data pair, move the cursor to the next key/data
        /// pair in the database, and store that pair and as many duplicate data
        /// items that can fit in a buffer the size of one database page in
        /// <see cref="CurrentMultiple"/>. MoveNextDuplicateMultiple will return
        /// false if the next key/data pair of the database is not a duplicate
        /// data record for the current key/data pair.
        /// </summary>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveNextDuplicateMultiple() {
            return MoveNextDuplicateMultiple((int)pgsz, null);
        }
        /// <summary>
        /// If the next key/data pair of the database is a duplicate data record
        /// for the current key/data pair, then move cursor to the next key/data
        /// pair in the database, and store that pair and as many duplicate data
        /// items that can fit in a buffer the size of
        /// <paramref name="BufferSize"/> in <see cref="CurrentMultiple"/>.
        /// MoveNextDuplicateMultiple will return false if the next key/data
        /// pair of the database is not a duplicate data record for the current
        /// key/data pair.
        /// </summary>
        /// <param name="BufferSize">
        /// The size of a buffer to fill with duplicate data items.  Must be at
        /// least the page size of the underlying database and be a multiple of
        /// 1024.
        /// </param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveNextDuplicateMultiple(int BufferSize) {
            return MoveNextDuplicateMultiple(BufferSize, null);
        }
        /// <summary>
        /// If the next key/data pair of the database is a duplicate data record
        /// for the current key/data pair, move the cursor to the next key/data
        /// pair in the database, and store that pair and as many duplicate data
        /// items that can fit in a buffer the size of one database page in
        /// <see cref="CurrentMultiple"/>. MoveNextDuplicateMultiple will return
        /// false if the next key/data pair of the database is not a duplicate
        /// data record for the current key/data pair.
        /// </summary>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveNextDuplicateMultiple(LockingInfo info) {
            return MoveNextDuplicateMultiple((int)pgsz, info);
        }
        /// <summary>
        /// If the next key/data pair of the database is a duplicate data record
        /// for the current key/data pair, move the cursor to the next key/data
        /// pair in the database, and store that pair and as many duplicate data
        /// items that can fit in a buffer the size of
        /// <paramref name="BufferSize"/> in <see cref="CurrentMultiple"/>.
        /// MoveNextDuplicateMultiple will return false if the next key/data
        /// pair of the database is not a duplicate data record for the current
        /// key/data pair.
        /// </summary>
        /// <param name="BufferSize">
        /// The size of a buffer to fill with duplicate data items.  Must be at
        /// least the page size of the underlying database and be a multiple of
        /// 1024.
        /// </param>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveNextDuplicateMultiple(
            int BufferSize, LockingInfo info) {
            DatabaseEntry key = new DatabaseEntry();
            DatabaseEntry data = new DatabaseEntry();

            return GetMultiple(
                key, data, BufferSize, DbConstants.DB_NEXT_DUP, info, false);
        }

        /// <summary>
        /// If the next key/data pair of the database is a duplicate data record
        /// for the current key/data pair, move the cursor to the next key/data
        /// pair in the database, and store that pair and as many duplicate data
        /// items that can fit in a buffer the size of one database page in
        /// <see cref="CurrentMultipleKey"/>. MoveNextDuplicateMultipleKey will
        /// return false if the next key/data pair of the database is not a
        /// duplicate data record for the current key/data pair.
        /// </summary>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveNextDuplicateMultipleKey() {
            return MoveNextDuplicateMultipleKey((int)pgsz, null);
        }
        /// <summary>
        /// If the next key/data pair of the database is a duplicate data record
        /// for the current key/data pair, move the cursor to the next key/data
        /// pair in the database, and store that pair and as many duplicate data
        /// items that can fit in a buffer the size of
        /// <paramref name="BufferSize"/> in <see cref="CurrentMultipleKey"/>.
        /// MoveNextDuplicateMultipleKey will return false if the next key/data
        /// pair of the database is not a duplicate data record for the current
        /// key/data pair.
        /// </summary>
        /// <param name="BufferSize">
        /// The size of a buffer to fill with key/data pairs.  Must be at least
        /// the page size of the underlying database and be a multiple of 1024.
        /// </param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveNextDuplicateMultipleKey(int BufferSize) {
            return MoveNextDuplicateMultipleKey(BufferSize, null);
        }
        /// <summary>
        /// If the next key/data pair of the database is a duplicate data record
        /// for the current key/data pair, move the cursor to the next key/data
        /// pair in the database, and store that pair and as many duplicate data
        /// items that can fit in a buffer the size of one database page in
        /// <see cref="CurrentMultipleKey"/>. MoveNextDuplicateMultipleKey will
        /// return false if the next key/data pair of the database is not a
        /// duplicate data record for the current key/data pair.
        /// </summary>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveNextDuplicateMultipleKey(LockingInfo info) {
            return MoveNextDuplicateMultipleKey((int)pgsz, info);
        }
        /// <summary>
        /// If the next key/data pair of the database is a duplicate data record
        /// for the current key/data pair, move the cursor to the next key/data
        /// pair in the database, and store that pair and as many duplicate data
        /// items that can fit in a buffer the size of
        /// <paramref name="BufferSize"/> in <see cref="CurrentMultipleKey"/>.
        /// MoveNextDuplicateMultipleKey will return false if the next key/data
        /// pair of the database is not a duplicate data record for the current
        /// key/data pair.
        /// </summary>
        /// <param name="BufferSize">
        /// The size of a buffer to fill with key/data pairs.  Must be at least
        /// the page size of the underlying database and be a multiple of 1024.
        /// </param>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveNextDuplicateMultipleKey(
            int BufferSize, LockingInfo info) {
            DatabaseEntry key = new DatabaseEntry();
            DatabaseEntry data = new DatabaseEntry();

            return GetMultiple(
                key, data, BufferSize, DbConstants.DB_NEXT_DUP, info, true);
        }

        /// <summary>
        /// If the cursor is not yet initialized, MoveNextUnique is identical to 
        /// <see cref="MoveFirst()"/>. Otherwise, move the cursor to the next
        /// non-duplicate key in the database, and store that key and associated
        /// datum in <see cref="Current"/>. MoveNextUnique will return false if
        /// no non-duplicate key/data pairs exist after the cursor position in
        /// the database. 
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
        /// <see cref="MoveFirst(LockingInfo)"/>. Otherwise, move the cursor to
        /// the next non-duplicate key in the database, and store that key and 
        /// associated datum in <see cref="Current"/>. MoveNextUnique will
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
            DatabaseEntry data = new DatabaseEntry();

            return Get(key, data, DbConstants.DB_NEXT_NODUP, info);
        }

        /// <summary>
        /// If the cursor is not yet initialized, MoveNextUniqueMultiple is
        /// identical to <see cref="MoveFirstMultiple()"/>. Otherwise, move the
        /// cursor to the next non-duplicate key in the database, and store that
        /// key and associated datum and as many duplicate data items that can
        /// fit in a buffer the size of one database page in
        /// <see cref="CurrentMultiple"/>. MoveNextUniqueMultiple will return
        /// false if no non-duplicate key/data pairs exist after the cursor
        /// position in the database. 
        /// </summary>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveNextUniqueMultiple() {
            return MoveNextUniqueMultiple((int)pgsz, null);
        }
        /// <summary>
        /// If the cursor is not yet initialized, MoveNextUniqueMultiple is
        /// identical to <see cref="MoveFirstMultiple(int)"/>. Otherwise, move
        /// the cursor to the next non-duplicate key in the database, and store
        /// that key and associated datum and as many duplicate data items that
        /// can fit in a buffer the size of <paramref name="BufferSize"/> in
        /// <see cref="CurrentMultiple"/>. MoveNextUniqueMultiple will return
        /// false if no non-duplicate key/data pairs exist after the cursor
        /// position in the database. 
        /// </summary>
        /// <param name="BufferSize">
        /// The size of a buffer to fill with duplicate data items.  Must be at
        /// least the page size of the underlying database and be a multiple of
        /// 1024.
        /// </param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveNextUniqueMultiple(int BufferSize) {
            return MoveNextUniqueMultiple(BufferSize, null);
        }
        /// <summary>
        /// If the cursor is not yet initialized, MoveNextUniqueMultiple is
        /// identical to <see cref="MoveFirstMultiple(LockingInfo)"/>.
        /// Otherwise, move the cursor to the next non-duplicate key in the
        /// database, and store that key and associated datum and as many
        /// duplicate data items that can fit in a buffer the size of one
        /// database page in <see cref="CurrentMultiple"/>.
        /// MoveNextUniqueMultiple will return false if no non-duplicate
        /// key/data pairs exist after the cursor position in the database. 
        /// </summary>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveNextUniqueMultiple(LockingInfo info) {
            return MoveNextUniqueMultiple((int)pgsz, info);
        }
        /// <summary>
        /// If the cursor is not yet initialized, MoveNextUniqueMultiple is
        /// identical to <see cref="MoveFirstMultiple(int, LockingInfo)"/>.
        /// Otherwise, move the cursor to the next non-duplicate key in the
        /// database, and store that key and associated datum and as many
        /// duplicate data items that can fit in a buffer the size of
        /// <paramref name="BufferSize"/> in <see cref="CurrentMultiple"/>.
        /// MoveNextUniqueMultiple will return false if no non-duplicate
        /// key/data pairs exist after the cursor position in the database. 
        /// </summary>
        /// <param name="BufferSize">
        /// The size of a buffer to fill with duplicate data items.  Must be at
        /// least the page size of the underlying database and be a multiple of
        /// 1024.
        /// </param>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveNextUniqueMultiple(int BufferSize, LockingInfo info) {
            DatabaseEntry key = new DatabaseEntry();
            DatabaseEntry data = new DatabaseEntry();

            return GetMultiple(
                key, data, BufferSize, DbConstants.DB_NEXT_NODUP, info, false);
        }

        /// <summary>
        /// If the cursor is not yet initialized, MoveNextUniqueMultipleKey is
        /// identical to <see cref="MoveFirstMultipleKey()"/>. Otherwise, move
        /// the cursor to the next non-duplicate key in the database, and store
        /// that key and associated datum and as many ensuing key/data pairs
        /// that can fit in a buffer the size of one database page in
        /// <see cref="CurrentMultipleKey"/>. MoveNextUniqueMultipleKey will
        /// return false if no non-duplicate key/data pairs exist after the
        /// cursor position in the database. 
        /// </summary>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveNextUniqueMultipleKey() {
            return MoveNextUniqueMultipleKey((int)pgsz, null);
        }
        /// <summary>
        /// If the cursor is not yet initialized, MoveNextUniqueMultipleKey is
        /// identical to <see cref="MoveFirstMultipleKey(int)"/>. Otherwise,
        /// move the cursor to the next non-duplicate key in the database, and
        /// store that key and associated datum and as many ensuing key/data
        /// pairs that can fit in a buffer the size of
        /// <paramref name="BufferSize"/> in <see cref="CurrentMultipleKey"/>.
        /// MoveNextUniqueMultipleKey will return false if no non-duplicate
        /// key/data pairs exist after the cursor position in the database. 
        /// </summary>
        /// <param name="BufferSize">
        /// The size of a buffer to fill with key/data pairs.  Must be at least
        /// the page size of the underlying database and be a multiple of 1024.
        /// </param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveNextUniqueMultipleKey(int BufferSize) {
            return MoveNextUniqueMultipleKey(BufferSize, null);
        }
        /// <summary>
        /// If the cursor is not yet initialized, MoveNextUniqueMultipleKey is
        /// identical to <see cref="MoveFirstMultipleKey(LockingInfo)"/>.
        /// Otherwise, move the cursor to the next non-duplicate key in the
        /// database, and store that key and associated datum and as many
        /// ensuing key/data pairs that can fit in a buffer the size of one
        /// database page in <see cref="CurrentMultipleKey"/>.
        /// MoveNextUniqueMultipleKey will return false if no non-duplicate
        /// key/data pairs exist after the cursor position in the database. 
        /// </summary>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveNextUniqueMultipleKey(LockingInfo info) {
            return MoveNextUniqueMultipleKey((int)pgsz, info);
        }
        /// <summary>
        /// If the cursor is not yet initialized, MoveNextUniqueMultipleKey is
        /// identical to <see cref="MoveFirstMultipleKey(int, LockingInfo)"/>.
        /// Otherwise, move the cursor to the next non-duplicate key in the
        /// database, and store that key and associated datum and as many
        /// ensuing key/data pairs that can fit in a buffer the size of
        /// <paramref name="BufferSize"/> in <see cref="CurrentMultipleKey"/>.
        /// MoveNextUniqueMultipleKey will return false if no non-duplicate
        /// key/data pairs exist after the cursor position in the database. 
        /// </summary>
        /// <param name="BufferSize">
        /// The size of a buffer to fill with key/data pairs.  Must be at least
        /// the page size of the underlying database and be a multiple of 1024.
        /// </param>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveNextUniqueMultipleKey(
            int BufferSize, LockingInfo info) {
            DatabaseEntry key = new DatabaseEntry();
            DatabaseEntry data = new DatabaseEntry();

            return GetMultiple(
                key, data, BufferSize, DbConstants.DB_NEXT_NODUP, info, true);
        }

        /// <summary>
        /// If the cursor is not yet initialized, MovePrev is identical to 
        /// <see cref="MoveLast()"/>. Otherwise, move the cursor to the previous
        /// key/data pair of the database, and store that pair in
        /// <see cref="Current"/>. In the presence of duplicate key values, the
        /// value of <see cref="Current">Current.Key</see> may not change. 
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
        /// the previous key/data pair of the database, and store that pair in
        /// <see cref="Current"/>. In the presence of duplicate key values, the
        /// value of <see cref="Current">Current.Key</see> may not change. 
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
            DatabaseEntry data = new DatabaseEntry();

            return Get(key, data, DbConstants.DB_PREV, info);
        }

        /// <summary>
        /// If the previous key/data pair of the database is a duplicate data
        /// record for the current key/data pair, the cursor is moved to the
        /// previous key/data pair of the database, and that pair is stored in
        /// <see cref="Current"/>. MovePrevDuplicate will return false if the
        /// previous key/data pair of the database is not a duplicate data
        /// record for the current key/data pair.
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
        /// previous key/data pair of the database, and that pair is stored in
        /// <see cref="Current"/>. MovePrevDuplicate will return false if the
        /// previous key/data pair of the database is not a duplicate data
        /// record for the current key/data pair.
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
            DatabaseEntry data = new DatabaseEntry();

            return Get(key, data, DbConstants.DB_PREV_DUP, info);
        }

        /// <summary>
        /// If the cursor is not yet initialized, MovePrevUnique is identical to 
        /// <see cref="MoveLast()"/>. Otherwise, move the cursor to the previous
        /// non-duplicate key in the database, and store that key and associated
        /// datum in <see cref="Current"/>. MovePrevUnique will return false if
        /// no non-duplicate key/data pairs exist after the cursor position in
        /// the database. 
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
        /// the previous non-duplicate key in the database, and store that key
        /// and associated datum in <see cref="Current"/>. MovePrevUnique will
        /// return false if no non-duplicate key/data pairs exist after the
        /// cursor position in the database. 
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
            DatabaseEntry data = new DatabaseEntry();

            return Get(key, data, DbConstants.DB_PREV_NODUP, info);
        }

        /// <summary>
        /// Overwrite the data of the key/data pair to which the cursor refers
        /// with the specified data item.
        /// </summary>
        /// <param name="data"></param>
        public void Overwrite(DatabaseEntry data) {
            DatabaseEntry key = new DatabaseEntry();

            Put(key, data, DbConstants.DB_CURRENT);
        }

        /// <summary>
        /// Store the key/data pair to which the cursor refers in
        /// <see cref="Current"/>.
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
        /// Store the key/data pair to which the cursor refers in
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
        public bool Refresh(LockingInfo info) {
            DatabaseEntry key = new DatabaseEntry();
            DatabaseEntry data = new DatabaseEntry();

            return Get(key, data, DbConstants.DB_CURRENT, info);
        }

        /// <summary>
        /// Store the key/data pair to which the cursor refers and as many
        /// duplicate data items that can fit in a buffer the size of one
        /// database page in <see cref="CurrentMultiple"/>.
        /// </summary>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool RefreshMultiple() {
            return RefreshMultiple((int)pgsz, null);
        }
        /// <summary>
        /// Store the key/data pair to which the cursor refers and as many
        /// duplicate data items that can fit in a buffer the size of
        /// <paramref name="BufferSize"/> in <see cref="CurrentMultiple"/>.
        /// </summary>
        /// <param name="BufferSize">
        /// The size of a buffer to fill with duplicate data items.  Must be at
        /// least the page size of the underlying database and be a multiple of
        /// 1024.
        /// </param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool RefreshMultiple(int BufferSize) {
            return RefreshMultiple(BufferSize, null);
        }
        /// <summary>
        /// Store the key/data pair to which the cursor refers and as many
        /// duplicate data items that can fit in a buffer the size of one
        /// database page in <see cref="CurrentMultiple"/>.
        /// </summary>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool RefreshMultiple(LockingInfo info) {
            return RefreshMultiple((int)pgsz, info);
        }
        /// <summary>
        /// Store the key/data pair to which the cursor refers and as many
        /// duplicate data items that can fit in a buffer the size of
        /// <paramref name="BufferSize"/> in <see cref="CurrentMultiple"/>.
        /// </summary>
        /// <param name="BufferSize">
        /// The size of a buffer to fill with duplicate data items.  Must be at
        /// least the page size of the underlying database and be a multiple of
        /// 1024.
        /// </param>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool RefreshMultiple(int BufferSize, LockingInfo info) {
            DatabaseEntry key = new DatabaseEntry();
            DatabaseEntry data = new DatabaseEntry();

            return GetMultiple(
                key, data, BufferSize, DbConstants.DB_CURRENT, info, false);
        }

        /// <summary>
        /// Store the key/data pair to which the cursor refers and as many
        /// ensuing key/data pairs that can fit in a buffer the size of one
        /// database page in <see cref="CurrentMultipleKey"/>.
        /// </summary>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool RefreshMultipleKey() {
            return RefreshMultipleKey((int)pgsz, null);
        }
        /// <summary>
        /// Store the key/data pair to which the cursor refers and as many
        /// ensuing key/data pairs that can fit in a buffer the size of
        /// <paramref name="BufferSize"/> in <see cref="CurrentMultipleKey"/>.
        /// </summary>
        /// <param name="BufferSize">
        /// The size of a buffer to fill with key/data pairs.  Must be at least
        /// the page size of the underlying database and be a multiple of 1024.
        /// </param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool RefreshMultipleKey(int BufferSize) {
            return RefreshMultipleKey(BufferSize, null);
        }
        /// <summary>
        /// Store the key/data pair to which the cursor refers and as many
        /// ensuing key/data pairs that can fit in a buffer the size of one
        /// database page in <see cref="CurrentMultipleKey"/>.
        /// </summary>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool RefreshMultipleKey(LockingInfo info) {
            return RefreshMultipleKey((int)pgsz, info);
        }
        /// <summary>
        /// Store the key/data pair to which the cursor refers and as many
        /// ensuing key/data pairs that can fit in a buffer the size of
        /// <paramref name="BufferSize"/> in <see cref="CurrentMultipleKey"/>.
        /// </summary>
        /// <param name="BufferSize">
        /// The size of a buffer to fill with key/data pairs.  Must be at least
        /// the page size of the underlying database and be a multiple of 1024.
        /// </param>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool RefreshMultipleKey(int BufferSize, LockingInfo info) {
            DatabaseEntry key = new DatabaseEntry();
            DatabaseEntry data = new DatabaseEntry();

            return GetMultiple(
                key, data, BufferSize, DbConstants.DB_CURRENT, info, true);
        }
    }
}

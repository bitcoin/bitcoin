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
    /// A class for traversing the records of a <see cref="BTreeDatabase"/>
    /// </summary>
    public class BTreeCursor : Cursor {
        internal BTreeCursor(DBC dbc, uint pagesize)
            : base(dbc, DatabaseType.BTREE, pagesize) { }
        internal BTreeCursor(DBC dbc, uint pagesize, CachePriority p)
            : base(dbc, DatabaseType.BTREE, pagesize, p) { }

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
        /// <see cref="BTreeDatabase.Cursor"/>.</param>
        /// <returns>A newly created cursor</returns>
        public new BTreeCursor Duplicate(bool keepPosition) {
            return new BTreeCursor(
                dbc.dup(keepPosition ? DbConstants.DB_POSITION : 0), pgsz);
        }
        /// <summary>
        /// Position the cursor at a specific key/data pair in the database, and
        /// store the key/data pair in <see cref="Cursor.Current"/>.
        /// </summary>
        /// <param name="recno">
        /// The specific numbered record of the database at which to position
        /// the cursor.
        /// </param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool Move(uint recno) { return Move(recno, null); }
        /// <summary>
        /// Position the cursor at a specific key/data pair in the database, and
        /// store the key/data pair in <see cref="Cursor.Current"/>.
        /// </summary>
        /// <param name="recno">
        /// The specific numbered record of the database at which to position
        /// the cursor.
        /// </param>
        /// <param name="info">The locking behavior to use</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool Move(uint recno, LockingInfo info) {
            DatabaseEntry key = new DatabaseEntry();
            key.Data = BitConverter.GetBytes(recno);
            DatabaseEntry data = new DatabaseEntry();

            return base.Get(key, data, DbConstants.DB_SET_RECNO, info);
        }

        /// <summary>
        /// Position the cursor at a specific key/data pair in the database, and
        /// store the key/data pair and as many duplicate data items that can
        /// fit in a buffer the size of one database page in
        /// <see cref="Cursor.CurrentMultiple"/>.
        /// </summary>
        /// <param name="recno">
        /// The specific numbered record of the database at which to position
        /// the cursor.
        /// </param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveMultiple(uint recno) {
            return MoveMultiple(recno, (int)pgsz, null);
        }
        /// <summary>
        /// Position the cursor at a specific key/data pair in the database, and
        /// store the key/data pair and as many duplicate data items that can
        /// fit in a buffer the size of one database page in
        /// <see cref="Cursor.CurrentMultiple"/>.
        /// </summary>
        /// <param name="recno">
        /// The specific numbered record of the database at which to position
        /// the cursor.
        /// </param>
        /// <param name="BufferSize">
        /// The size of a buffer to fill with duplicate data items.  Must be at
        /// least the page size of the underlying database and be a multiple of
        /// 1024.
        /// </param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveMultiple(uint recno, int BufferSize) {
            return MoveMultiple(recno, BufferSize, null);
        }
        /// <summary>
        /// Position the cursor at a specific key/data pair in the database, and
        /// store the key/data pair and as many duplicate data items that can
        /// fit in a buffer the size of one database page in
        /// <see cref="Cursor.CurrentMultiple"/>.
        /// </summary>
        /// <param name="recno">
        /// The specific numbered record of the database at which to position
        /// the cursor.
        /// </param>
        /// <param name="info">The locking behavior to use</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveMultiple(uint recno, LockingInfo info) {
            return MoveMultiple(recno, (int)pgsz, info);
        }
        /// <summary>
        /// Position the cursor at a specific key/data pair in the database, and
        /// store the key/data pair and as many duplicate data items that can
        /// fit in a buffer the size of one database page in
        /// <see cref="Cursor.CurrentMultiple"/>.
        /// </summary>
        /// <param name="recno">
        /// The specific numbered record of the database at which to position
        /// the cursor.
        /// </param>
        /// <param name="BufferSize">
        /// The size of a buffer to fill with duplicate data items.  Must be at
        /// least the page size of the underlying database and be a multiple of
        /// 1024.
        /// </param>
        /// <param name="info">The locking behavior to use</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveMultiple(uint recno, int BufferSize, LockingInfo info) {
            DatabaseEntry key = new DatabaseEntry();
            key.Data = BitConverter.GetBytes(recno);
            DatabaseEntry data = new DatabaseEntry();

            return base.GetMultiple(
                key, data, BufferSize, DbConstants.DB_SET_RECNO, info, false);
        }

        /// <summary>
        /// Position the cursor at a specific key/data pair in the database, and
        /// store the key/data pair and as many ensuing key/data pairs that can
        /// fit in a buffer the size of one database page in
        /// <see cref="Cursor.CurrentMultipleKey"/>.
        /// </summary>
        /// <param name="recno">
        /// The specific numbered record of the database at which to position
        /// the cursor.
        /// </param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveMultipleKey(uint recno) {
            return MoveMultipleKey(recno, (int)pgsz, null);
        }
        /// <summary>
        /// Position the cursor at a specific key/data pair in the database, and
        /// store the key/data pair and as many ensuing key/data pairs that can
        /// fit in a buffer the size of one database page in
        /// <see cref="Cursor.CurrentMultipleKey"/>.
        /// </summary>
        /// <param name="recno">
        /// The specific numbered record of the database at which to position
        /// the cursor.
        /// </param>
        /// <param name="BufferSize">
        /// The size of a buffer to fill with key/data pairs.  Must be at least
        /// the page size of the underlying database and be a multiple of 1024.
        /// </param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveMultipleKey(uint recno, int BufferSize) {
            return MoveMultipleKey(recno, BufferSize, null);
        }
        /// <summary>
        /// Position the cursor at a specific key/data pair in the database, and
        /// store the key/data pair and as many ensuing key/data pairs that can
        /// fit in a buffer the size of one database page in
        /// <see cref="Cursor.CurrentMultipleKey"/>.
        /// </summary>
        /// <param name="recno">
        /// The specific numbered record of the database at which to position
        /// the cursor.
        /// </param>
        /// <param name="info">The locking behavior to use</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveMultipleKey(uint recno, LockingInfo info) {
            return MoveMultipleKey(recno, (int)pgsz, info);
        }
        /// <summary>
        /// Position the cursor at a specific key/data pair in the database, and
        /// store the key/data pair and as many ensuing key/data pairs that can
        /// fit in a buffer the size of one database page in
        /// <see cref="Cursor.CurrentMultipleKey"/>.
        /// </summary>
        /// <param name="recno">
        /// The specific numbered record of the database at which to position
        /// the cursor.
        /// </param>
        /// <param name="BufferSize">
        /// The size of a buffer to fill with key/data pairs.  Must be at least
        /// the page size of the underlying database and be a multiple of 1024.
        /// </param>
        /// <param name="info">The locking behavior to use</param>
        /// <returns>
        /// True if the cursor was positioned successfully, false otherwise.
        /// </returns>
        public bool MoveMultipleKey(
            uint recno, int BufferSize, LockingInfo info) {
            DatabaseEntry key = new DatabaseEntry();
            key.Data = BitConverter.GetBytes(recno);
            DatabaseEntry data = new DatabaseEntry();

            return base.GetMultiple(
                key, data, BufferSize, DbConstants.DB_SET_RECNO, info, true);
        }

        /// <summary>
        /// Return the record number associated with the cursor's current
        /// position.
        /// </summary>
        /// <returns>The record number associated with the cursor.</returns>
        public uint Recno() { return Recno(null); }
        /// <summary>
        /// Return the record number associated with the cursor's current
        /// position.
        /// </summary>
        /// <param name="info">The locking behavior to use</param>
        /// <returns>The record number associated with the cursor.</returns>
        public uint Recno(LockingInfo info) {
            DatabaseEntry key = new DatabaseEntry();
            DatabaseEntry data = new DatabaseEntry();
            base.Get(key, data, DbConstants.DB_GET_RECNO, info);

            return BitConverter.ToUInt32(base.Current.Value.Data, 0);
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
        /// <summary>
        /// Insert the specified key/data pair into the database, unless a
        /// key/data pair comparing equally to it already exists in the
        /// database.
        /// </summary>
        /// <param name="pair">The key/data pair to be inserted</param>
        /// <exception cref="KeyExistException">
        /// Thrown if a matching key/data pair already exists in the database.
        /// </exception>
        public new void AddUnique(
            KeyValuePair<DatabaseEntry, DatabaseEntry> pair) {
            base.AddUnique(pair);
        }
        /// <summary>
        /// Insert the specified key/data pair into the database.
        /// </summary>
        /// <param name="pair">The key/data pair to be inserted</param>
        /// <param name="loc">
        /// If the key already exists in the database and no duplicate sort
        /// function has been specified, specify whether the inserted data item
        /// is added as the first or the last of the data items for that key. 
        /// </param>
        public new void Add(KeyValuePair<DatabaseEntry, DatabaseEntry> pair,
            InsertLocation loc) {
            base.Add(pair, loc);
        }
    }
}

/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.Text;

namespace BerkeleyDB {
    /// <summary>
    /// A class representing configuration parameters for
    /// <see cref="RecnoDatabase"/>
    /// </summary>
    public class RecnoDatabaseConfig : DatabaseConfig {
        /* Fields for DB->set_flags() */
        /// <summary>
        /// Cause the logical record numbers to be mutable, and change as
        /// records are added to and deleted from the database.
        /// </summary>
        /// <remarks>
        /// <para>
        /// Using <see cref="Database.Put"/> or <see cref="Cursor.Put"/> to
        /// create new records will cause the creation of multiple records if
        /// the record number is more than one greater than the largest record
        /// currently in the database. For example, creating record 28, when
        /// record 25 was previously the last record in the database, will
        /// create records 26 and 27 as well as 28. Attempts to retrieve records
        /// that were created in this manner will throw a
        /// <see cref="KeyEmptyException"/>.
        /// </para>
        /// <para>
        /// If a created record is not at the end of the database, all records
        /// following the new record will be automatically renumbered upward by
        /// one. For example, the creation of a new record numbered 8 causes
        /// records numbered 8 and greater to be renumbered upward by one. If a
        /// cursor was positioned to record number 8 or greater before the
        /// insertion, it will be shifted upward one logical record, continuing
        /// to refer to the same record as it did before.
        /// </para>
        /// <para>
        /// If a deleted record is not at the end of the database, all records
        /// following the removed record will be automatically renumbered
        /// downward by one. For example, deleting the record numbered 8 causes
        /// records numbered 9 and greater to be renumbered downward by one.  If
        /// a cursor was positioned to record number 9 or greater before the
        /// removal, it will be shifted downward one logical record, continuing
        /// to refer to the same record as it did before.
        /// </para>
        /// <para>
        /// If a record is deleted, all cursors that were positioned on that
        /// record prior to the removal will no longer be positioned on a valid
        /// entry. This includes cursors used to delete an item. For example, if
        /// a cursor was positioned to record number 8 before the removal of
        /// that record, subsequent calls to <see cref="Cursor.Refresh"/>
        /// will return false until the cursor is moved to another record. A
        /// call to <see cref="Cursor.MoveNext"/> will return the new record
        /// numbered 8 - which is the record that was numbered 9 prior to the
        /// delete (if such a record existed).
        /// </para>
        /// <para>
        /// For these reasons, concurrent access to a
        /// <see cref="RecnoDatabase"/> with this setting specified may be
        /// largely meaningless, although it is supported.
        /// </para>
        /// <para>
        /// If the database already exists, this setting must be the same as the
        /// existing database or an exception will be thrown.
        /// </para>
        /// </remarks>
        public bool Renumber;
        /// <summary>
        /// If true, any <see cref="BackingFile"/> file will be read in its
        /// entirety when <see cref="RecnoDatabase.Open"/> is called. If false,
        /// <see cref="BackingFile"/> may be read lazily. 
        /// </summary>
        public bool Snapshot;
        internal new uint flags {
            get {
                uint ret = base.flags;
                ret |= Renumber ? Internal.DbConstants.DB_RENUMBER : 0;
                ret |= Snapshot ? Internal.DbConstants.DB_SNAPSHOT : 0;
                return ret;
            }
        }

        /// <summary>
        /// The policy for how to handle database creation.
        /// </summary>
        /// <remarks>
        /// If the database does not already exist and
        /// <see cref="CreatePolicy.NEVER"/> is set,
        /// <see cref="RecnoDatabase.Open"/> will fail.
        /// </remarks>
        public CreatePolicy Creation;
        internal new uint openFlags {
            get {
                uint flags = base.openFlags;
                flags |= (uint)Creation;
                return flags;
            }
        }

        /// <summary>
        /// A function to call after the record number has been selected but
        /// before the data has been stored into the database.
        /// </summary>
        /// <remarks>
        /// <para>
        /// When using <see cref="QueueDatabase.Append"/>, it may be useful to
        /// modify the stored data based on the generated key. If a delegate is
        /// specified, it will be called after the record number has been
        /// selected, but before the data has been stored.
        /// </para>
        /// </remarks>
        public AppendRecordDelegate Append;

        internal bool delimiterIsSet;
        private int delim;
        /// <summary>
        /// The delimiting byte used to mark the end of a record in
        /// <see cref="BackingFile"/>.
        /// </summary>
        /// <remarks>
        /// <para>
        /// This byte is used for variable length records if
        /// <see cref="BackingFile"/> is set. If <see cref="BackingFile"/> is
        /// specified and no delimiting byte was specified, newline characters
        /// (that is, ASCII 0x0a) are interpreted as end-of-record markers.
        /// </para>
        /// <para>
        /// If the database already exists, this setting will be ignored.
        /// </para>
        /// </remarks>
        public int Delimiter {
            get { return delim; }
            set {
                delimiterIsSet = true;
                delim = value;
            }
        }

        internal bool lengthIsSet;
        private uint len;
        /// <summary>
        /// Specify that the records are fixed-length, not byte-delimited, and
        /// are of length Length. 
        /// </summary>
        /// <remarks>
        /// <para>
        /// Any records added to the database that are less than Length bytes
        /// long are automatically padded (see <see cref="PadByte"/> for more
        /// information).
        /// </para>
        /// <para>
        /// Any attempt to insert records into the database that are greater
        /// than Length bytes long will cause the call to fail immediately and
        /// return an error. 
        /// </para>
        /// <para>
        /// If the database already exists, this setting will be ignored.
        /// </para>
        /// </remarks>
        public uint Length {
            get { return len; }
            set {
                lengthIsSet = true;
                len = value;
            }
        }

        internal bool padIsSet;
        private int pad;
        /// <summary>
        /// The padding character for short, fixed-length records.
        /// </summary>
        /// <remarks>
        /// <para>
        /// If no pad character is specified, space characters (that is, ASCII
        /// 0x20) are used for padding.
        /// </para>
        /// <para>
        /// If the database already exists, this setting will be ignored.
        /// </para>
        /// </remarks>
        public int PadByte {
            get { return pad; }
            set {
                padIsSet = true;
                pad = value;
            }
        }

        /// <summary>
        /// The underlying source file for the Recno access method.
        /// </summary>
        /// <remarks>
        /// <para>
        /// The purpose of the source file is to provide fast access and
        /// modification to databases that are normally stored as flat text
        /// files.
        /// </para>
        /// <para>
        /// The source parameter specifies an underlying flat text database file
        /// that is read to initialize a transient record number index. In the
        /// case of variable length records, the records are separated, as
        /// specified by <see cref="Delimiter"/>. For example, standard UNIX
        /// byte stream files can be interpreted as a sequence of variable
        /// length records separated by newline characters.
        /// </para>
        /// <para>
        /// In addition, when cached data would normally be written back to the
        /// underlying database file (for example,
        /// <see cref="BaseDatabase.Close"/> or
        /// <see cref="BaseDatabase.Sync"/>), the in-memory copy of the
        /// database will be written back to the source file.
        /// </para>
        /// <para>
        /// By default, the backing source file is read lazily; that is, records
        /// are not read from the file until they are requested by the
        /// application. If multiple processes (not threads) are accessing a
        /// Recno database concurrently, and are either inserting or deleting
        /// records, the backing source file must be read in its entirety before
        /// more than a single process accesses the database, and only that
        /// process should specify the backing source file as part of the
        /// <see cref="RecnoDatabase.Open"/> call. See <see cref="Snapshot"/>
        /// for more information.
        /// </para>
        /// <para>
        /// Reading and writing the backing source file specified by source
        /// cannot be transaction-protected because it involves filesystem
        /// operations that are not part of the Db transaction methodology. For
        /// this reason, if a temporary database is used to hold the records, it
        /// is possible to lose the contents of the source file, for example, if
        /// the system crashes at the right instant. If a file is used to hold
        /// the database, normal database recovery on that file can be used to
        /// prevent information loss, although it is still possible that the
        /// contents of source will be lost if the system crashes.
        /// </para>
        /// <para>
        /// The source file must already exist (but may be zero-length) when 
        /// <see cref="RecnoDatabase.Open"/> is called.
        /// </para>
        /// <para>
        /// It is not an error to specify a read-only source file when creating
        /// a database, nor is it an error to modify the resulting database.
        /// However, any attempt to write the changes to the backing source file
        /// using either the <see cref="BaseDatabase.Sync"/> or
        /// <see cref="BaseDatabase.Close"/> methods will fail, of course.
        /// Use <see cref="BaseDatabase.Close(bool)"/> to stop it from
        /// attempting to write the changes to the backing file; instead, they
        /// will be silently discarded.
        /// </para>
        /// <para>
        /// For all of the previous reasons, the source file is generally used
        /// to specify databases that are read-only for Berkeley DB
        /// applications; and that are either generated on the fly by software
        /// tools or modified using a different mechanism — for example, a text
        /// editor.
        /// </para>
        /// <para>
        /// If the database already exists, BackingFile must be the same as that
        /// historically used to create the database or corruption can occur.
        /// </para>
        /// </remarks>
        public string BackingFile;

        /// <summary>
        /// Instantiate a new RecnoDatabaseConfig object
        /// </summary>
        public RecnoDatabaseConfig() {
            Renumber = false;
            Snapshot = false;
            Append = null;
            delimiterIsSet = false;
            lengthIsSet = false;
            padIsSet = false;
            BackingFile = null;
            Creation = CreatePolicy.NEVER;
        }

    }
}
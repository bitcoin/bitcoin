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
    /// <see cref="SecondaryBTreeDatabase"/>
    /// </summary>
    public class SecondaryBTreeDatabaseConfig : SecondaryDatabaseConfig {
        /// <summary>
        /// Policy for duplicate data items in the database; that is, insertion
        /// when the key of the key/data pair being inserted already exists in
        /// the database will be successful.
        /// </summary>
        /// <remarks>
        /// <para>The ordering of duplicates in the database for
        /// <see cref="DuplicatesPolicy.UNSORTED"/> is determined by the order
        /// of insertion, unless the ordering is otherwise specified by use of a
        /// cursor operation or a duplicate sort function. The ordering of
        /// duplicates in the database for
        /// <see cref="DuplicatesPolicy.SORTED"/> is determined by the
        /// duplicate comparison function. If the application does not specify a
        /// comparison function using 
        /// <see cref="DuplicateCompare"/>, a default lexical
        /// comparison will be used.
        /// </para>
        /// <para>
        /// <see cref="DuplicatesPolicy.SORTED"/> is preferred to 
        /// <see cref="DuplicatesPolicy.UNSORTED"/> for performance reasons.
        /// <see cref="DuplicatesPolicy.UNSORTED"/> should only be used by
        /// applications wanting to order duplicate data items manually.
        /// </para>
        /// <para>
        /// If the database already exists, the value of Duplicates must be the
        /// same as the existing database or an error will be returned.
        /// </para>
        /// <para>
        /// It is an error to specify <see cref="UseRecordNumbers"/> and
        /// anything other than <see cref="DuplicatesPolicy.NONE"/>.
        /// </para>
        /// </remarks>
        public DuplicatesPolicy Duplicates;
        /// <summary>
        /// Turn reverse splitting in the Btree on or off.
        /// </summary>
        /// <remarks>
        /// As pages are emptied in a database, the Berkeley DB Btree
        /// implementation attempts to coalesce empty pages into higher-level
        /// pages in order to keep the database as small as possible and
        /// minimize search time. This can hurt performance in applications with
        /// cyclical data demands; that is, applications where the database
        /// grows and shrinks repeatedly. For example, because Berkeley DB does
        /// page-level locking, the maximum level of concurrency in a database
        /// of two pages is far smaller than that in a database of 100 pages, so
        /// a database that has shrunk to a minimal size can cause severe
        /// deadlocking when a new cycle of data insertion begins. 
        /// </remarks>
        public bool NoReverseSplitting;
        /// <summary>
        /// If true, support retrieval from the Btree using record numbers.
        /// </summary>
        /// <remarks>
        /// <para>
        /// Logical record numbers in Btree databases are mutable in the face of
        /// record insertion or deletion. See
        /// <see cref="RecnoDatabaseConfig.Renumber"/> for further discussion.
        /// </para>
        /// <para>
        /// Maintaining record counts within a Btree introduces a serious point
        /// of contention, namely the page locations where the record counts are
        /// stored. In addition, the entire database must be locked during both
        /// insertions and deletions, effectively single-threading the database
        /// for those operations. Specifying UseRecordNumbers can result in
        /// serious performance degradation for some applications and data sets.
        /// </para>
        /// <para>
        /// It is an error to specify <see cref="UseRecordNumbers"/> and
        /// anything other than <see cref="DuplicatesPolicy.NONE"/>.
        /// </para>
        /// <para>
        /// If the database already exists, the value of UseRecordNumbers must
        /// be the same as the existing database or an error will be returned. 
        /// </para>
        /// </remarks>
        public bool UseRecordNumbers;
        internal new uint flags {
            get {
                uint ret = base.flags;
                ret |= (uint)Duplicates;
                ret |= NoReverseSplitting ?
                    Internal.DbConstants.DB_REVSPLITOFF : 0;
                ret |= UseRecordNumbers ? Internal.DbConstants.DB_RECNUM : 0;
                return ret;
            }
        }

        /// <summary>
        /// The policy for how to handle database creation.
        /// </summary>
        /// <remarks>
        /// If the database does not already exist and
        /// <see cref="CreatePolicy.NEVER"/> is set,
        /// <see cref="SecondaryBTreeDatabase.Open"/> will fail.
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
        /// The Btree key comparison function.
        /// </summary>
        /// <remarks>
        /// <para>
        /// The comparison function is called whenever it is necessary to
        /// compare a key specified by the application with a key currently
        /// stored in the tree.
        /// </para>
        /// <para>
        /// If no comparison function is specified, the keys are compared
        /// lexically, with shorter keys collating before longer keys.
        /// </para>
        /// <para>
        /// If the database already exists, the comparison function must be the
        /// same as that historically used to create the database or corruption
        /// can occur. 
        /// </para>
        /// </remarks>
        public EntryComparisonDelegate Compare;
        /// <summary>
        /// The Btree prefix function.
        /// </summary>
        /// <remarks>
        /// <para>
        /// The prefix function is used to determine the amount by which keys
        /// stored on the Btree internal pages can be safely truncated without
        /// losing their uniqueness. See the Btree prefix comparison section of
        /// the Berkeley DB Reference Guide for more details about how this
        /// works. The usefulness of this is data-dependent, but can produce
        /// significantly reduced tree sizes and search times in some data sets.
        /// </para>
        /// <para>
        /// If no prefix function or key comparison function is specified by the
        /// application, a default lexical comparison function is used as the
        /// prefix function. If no prefix function is specified and
        /// <see cref="Compare"/> is specified, no prefix function is
        /// used. It is an error to specify a prefix function without also
        /// specifying <see cref="Compare"/>. 
        /// </para>
        /// <para>
        /// If the database already exists, the prefix function must be the
        /// same as that historically used to create the database or corruption
        /// can occur. 
        /// </para>
        /// </remarks>
        public EntryComparisonDelegate PrefixCompare;
        /// <summary>
        /// The duplicate data item comparison function.
        /// </summary>
        /// <remarks>
        /// <para>
        /// The comparison function is called whenever it is necessary to
        /// compare a data item specified by the application with a data item
        /// currently stored in the database. Setting DuplicateCompare implies 
        /// setting <see cref="Duplicates"/> to
        /// <see cref="DuplicatesPolicy.SORTED"/>.
        /// </para>
        /// <para>
        /// If no comparison function is specified, the data items are compared
        /// lexically, with shorter data items collating before longer data
        /// items.
        /// </para>
        /// <para>
        /// If the database already exists when
        /// <see cref="SecondaryBTreeDatabase.Open"/> is called, the delegate
        /// must be the same as that historically used to create the database or
        /// corruption can occur.
        /// </para>
        /// </remarks>
        public EntryComparisonDelegate DuplicateCompare;

        internal bool minkeysIsSet;
        private uint minKeys;
        /// <summary>
        /// The minimum number of key/data pairs intended to be stored on any
        /// single Btree leaf page.
        /// </summary>
        /// <remarks>
        /// <para>
        /// This value is used to determine if key or data items will be stored
        /// on overflow pages instead of Btree leaf pages. For more information
        /// on the specific algorithm used, see the Berkeley DB Reference Guide.
        /// The value specified must be at least 2; if not explicitly set, a
        /// value of 2 is used. 
        /// </para>
        /// <para>
        /// If the database already exists, MinKeysPerPage will be ignored. 
        /// </para>
        /// </remarks>
        public uint MinKeysPerPage {
            get { return minKeys; }
            set {
                minkeysIsSet = true;
                minKeys = value;
            }
        }

        /// <summary>
        /// Create a new SecondaryBTreeDatabaseConfig object
        /// </summary>
        public SecondaryBTreeDatabaseConfig(
            Database PrimaryDB, SecondaryKeyGenDelegate KeyGenFunc)
            : base(PrimaryDB, KeyGenFunc) {
            Duplicates = DuplicatesPolicy.NONE;
            NoReverseSplitting = false;
            UseRecordNumbers = false;

            Compare = null;
            PrefixCompare = null;

            minkeysIsSet = false;

            DbType = DatabaseType.BTREE;
        }
    }
}
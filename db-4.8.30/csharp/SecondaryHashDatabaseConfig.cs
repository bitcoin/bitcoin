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
    /// <see cref="SecondaryHashDatabase"/>
    /// </summary>
    public class SecondaryHashDatabaseConfig : SecondaryDatabaseConfig {
        /* Fields for db->set_flags() */
        /// <summary>
        /// Policy for duplicate data items in the database; that is, insertion
        /// when the key of the key/data pair being inserted already exists in
        /// the database will be successful.
        /// </summary>
        /// <remarks>
        /// <para>
        /// The ordering of duplicates in the database for
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
        /// </remarks>
        public DuplicatesPolicy Duplicates;
        internal new uint flags {
            get {
                uint ret = base.flags;
                ret |= (uint)Duplicates;
                return ret;
            }
        }

        /// <summary>
        /// The policy for how to handle database creation.
        /// </summary>
        /// <remarks>
        /// If the database does not already exist and
        /// <see cref="CreatePolicy.NEVER"/> is set,
        /// <see cref="SecondaryHashDatabase.Open"/> will fail.
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
        /// The Secondary Hash key comparison function.
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

        internal bool fillFactorIsSet;
        private uint ffactor;
        /// <summary>
        /// The desired density within the hash table. If no value is specified,
        /// the fill factor will be selected dynamically as pages are filled. 
        /// </summary>
        /// <remarks>
        /// <para>
        /// The density is an approximation of the number of keys allowed to
        /// accumulate in any one bucket, determining when the hash table grows
        /// or shrinks. If you know the average sizes of the keys and data in
        /// your data set, setting the fill factor can enhance performance. A
        /// reasonable rule computing fill factor is to set it to the following:
        /// </para>
        /// <para>
        /// (pagesize - 32) / (average_key_size + average_data_size + 8)
        /// </para>
        /// <para>
        /// If the database already exists, this setting will be ignored.
        /// </para>
        /// </remarks>
        public uint FillFactor {
            get { return ffactor; }
            set {
                fillFactorIsSet = true;
                ffactor = value;
            }
        }

        /// <summary>
        /// A user-defined hash function; if no hash function is specified, a
        /// default hash function is used. 
        /// </summary>
        /// <remarks>
        /// <para>
        /// Because no hash function performs equally well on all possible data,
        /// the user may find that the built-in hash function performs poorly
        /// with a particular data set.
        /// </para>
        /// <para>
        /// If the database already exists, HashFunction must be the same as
        /// that historically used to create the database or corruption can
        /// occur.
        /// </para>
        /// </remarks>
        public HashFunctionDelegate HashFunction;
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
        /// <see cref="SecondaryHashDatabase.Open"/> is called, the delegate
        /// must be the same as that historically used to create the database or
        /// corruption can occur.
        /// </para>
        /// </remarks>
        public EntryComparisonDelegate DuplicateCompare;

        internal bool nelemIsSet;
        private uint nelems;
        /// <summary>
        /// An estimate of the final size of the hash table.
        /// </summary>
        /// <remarks>
        /// <para>
        /// In order for the estimate to be used when creating the database,
        /// <see cref="FillFactor"/> must also be set. If the estimate or fill
        /// factor are not set or are set too low, hash tables will still expand
        /// gracefully as keys are entered, although a slight performance
        /// degradation may be noticed.
        /// </para>
        /// <para>
        /// If the database already exists, this setting will be ignored.
        /// </para>
        /// </remarks>
        public uint TableSize {
            get { return nelems; }
            set {
                nelemIsSet = true;
                nelems = value;
            }
        }

        /// <summary>
        /// Instantiate a new SecondaryHashDatabaseConfig object
        /// </summary>
        public SecondaryHashDatabaseConfig(
            Database PrimaryDB, SecondaryKeyGenDelegate KeyGenFunc)
            : base(PrimaryDB, KeyGenFunc) {
            Duplicates = DuplicatesPolicy.NONE;
            Compare = null;
            fillFactorIsSet = false;
            nelemIsSet = false;
            DbType = DatabaseType.HASH;
        }
    }
}
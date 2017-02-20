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
    /// <see cref="SecondaryQueueDatabase"/>
    /// </summary>
    public class SecondaryQueueDatabaseConfig : SecondaryDatabaseConfig {
        internal bool lengthIsSet;
        private uint len;
        /// <summary>
        /// Specify the length of records in the database.
        /// </summary>
        /// <remarks>
        /// <para>
        /// The record length must be enough smaller than
        /// <see cref="DatabaseConfig.PageSize"/> that at least one record plus
        /// the database page's metadata information can fit on each database
        /// page.
        /// </para>
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

        /// <summary>
        /// The policy for how to handle database creation.
        /// </summary>
        /// <remarks>
        /// If the database does not already exist and
        /// <see cref="CreatePolicy.NEVER"/> is set,
        /// <see cref="SecondaryQueueDatabase.Open"/> will fail.
        /// </remarks>
        public CreatePolicy Creation;
        internal new uint openFlags {
            get {
                uint flags = base.openFlags;
                flags |= (uint)Creation;
                return flags;
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

        internal bool extentIsSet;
        private uint extentSz;
        /// <summary>
        /// The size of the extents used to hold pages in a
        /// <see cref="SecondaryQueueDatabase"/>, specified as a number of
        /// pages. 
        /// </summary>
        /// <remarks>
        /// <para>
        /// Each extent is created as a separate physical file. If no extent
        /// size is set, the default behavior is to create only a single
        /// underlying database file.
        /// </para>
        /// <para>
        /// For information on tuning the extent size, see Selecting a extent
        /// size in the Programmer's Reference Guide.
        /// </para>
        /// <para>
        /// If the database already exists, this setting will be ignored.
        /// </para>
        /// </remarks>
        public uint ExtentSize {
            get { return extentSz; }
            set {
                extentIsSet = true;
                extentSz = value;
            }
        }

        /// <summary>
        /// Instantiate a new SecondaryQueueDatabaseConfig object
        /// </summary>
        public SecondaryQueueDatabaseConfig(
            Database PrimaryDB, SecondaryKeyGenDelegate KeyGenFunc)
            : base(PrimaryDB, KeyGenFunc) {
            lengthIsSet = false;
            padIsSet = false;
            extentIsSet = false;
            DbType = DatabaseType.QUEUE;
        }
    }
}
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
    /// A class to represent the database byte order.
    /// </summary>
    public class ByteOrder {
        private int _lorder;
        internal int lorder { get { return _lorder; } }

        /// <summary>
        /// The host byte order of the machine where the Berkeley DB library was
        /// compiled.
        /// </summary>
        public static ByteOrder MACHINE = new ByteOrder(0);
        /// <summary>
        /// Little endian byte order
        /// </summary>
        public static ByteOrder LITTLE_ENDIAN = new ByteOrder(1234);
        /// <summary>
        /// Big endian byte order
        /// </summary>
        public static ByteOrder BIG_ENDIAN = new ByteOrder(4321);

        /// <summary>
        /// Convert from the integer constant used to represent byte order in 
        /// the C library to its corresponding ByteOrder object.
        /// </summary>
        /// <param name="order">The C library constant</param>
        /// <returns>
        /// The ByteOrder object corresponding to the given constant
        /// </returns>
        public static ByteOrder FromConst(int order) {
            switch (order) {
                case 0: return MACHINE;
                case 1234: return LITTLE_ENDIAN;
                case 4321: return BIG_ENDIAN;
            }
            throw new ArgumentException("Invalid byte order constant.");
        }

        private ByteOrder(int order) {
            _lorder = order;
        }
    }
}
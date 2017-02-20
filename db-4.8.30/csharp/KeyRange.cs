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
    /// <para>
    /// A class representing an estimate of the proportion of keys that are less
    /// than, equal to, and greater than a given key.
    /// </para>
    /// <para>
    /// Values are in the range of 0 to 1; for example, if the field less is
    /// 0.05, 5% of the keys in the database are less than the key parameter.
    /// The value for equal will be zero if there is no matching key, and will
    /// be non-zero otherwise. 
    /// </para>
    /// </summary>
    public class KeyRange {
        private DB_KEY_RANGE kr;

        internal KeyRange(DB_KEY_RANGE keyRange) {
            kr = keyRange;
        }
        /// <summary>
        /// A value between 0 and 1, the proportion of keys less than the
        /// specified key. 
        /// </summary>
        public double Less { get { return kr.less; } }
        /// <summary>
        /// A value between 0 and 1, the proportion of keys equal to the
        /// specified key. 
        /// </summary>
        public double Equal { get { return kr.equal; } }
        /// <summary>
        /// A value between 0 and 1, the proportion of keys greater than the
        /// specified key.
        /// </summary>
        public double Greater { get { return kr.greater; } }
    }
}

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
    /// Configuration properties for a Sequence
    /// </summary>
    public class SequenceConfig {
        /// <summary>
        /// The policy for how to handle sequence creation.
        /// </summary>
        /// <remarks>
        /// If the sequence does not already exist and
        /// <see cref="CreatePolicy.NEVER"/> is set, the Sequence constructor
        /// will fail.
        /// </remarks>
        public CreatePolicy Creation;
        /// <summary>
        /// If true, the object returned by the Sequence constructor will be
        /// free-threaded; that is, usable by multiple threads within a single
        /// address space. Note that if multiple threads create multiple
        /// sequences using the same <see cref="BackingDatabase"/>, that
        /// database must have also been opened free-threaded.
        /// </summary>
        public bool FreeThreaded;

        internal uint openFlags {
            get {
                uint flags = 0;
                flags |= (uint)Creation;
                flags |= FreeThreaded ? DbConstants.DB_THREAD : 0;
                return flags;
            }
        }

        /// <summary>
        /// An open database which holds the persistent data for the sequence.
        /// </summary>
        /// <remarks>
        /// <para>
        /// The database may be of any type, but must not have been configured
        /// to support duplicate data items.
        /// </para>
        /// <para>
        /// If <paramref name="P:BackingDatabase"/> was opened in a transaction,
        /// calling Get may result in changes to the sequence object; these
        /// changes will be automatically committed in a transaction internal to
        /// the Berkeley DB library. If the thread of control calling Get has an
        /// active transaction, which holds locks on the same database as the
        /// one in which the sequence object is stored, it is possible for a
        /// thread of control calling Get to self-deadlock because the active
        /// transaction's locks conflict with the internal transaction's locks.
        /// For this reason, it is often preferable for sequence objects to be
        /// stored in their own database. 
        /// </para>
        /// </remarks>
        public Database BackingDatabase;

        private Int64 initVal;
        internal bool initialValIsSet;
        /// <summary>
        /// The initial value for a sequence.
        /// </summary>
        public Int64 InitialValue {
            get {
                return initVal;
            }
            set {
                initialValIsSet = true;
                initVal = value;
            }
        }

        /// <summary>
        /// The record in the database that stores the persistent sequence data. 
        /// </summary>
        public DatabaseEntry key;

        /// <summary>
        /// If true, the sequence should wrap around when it is incremented
        /// (decremented) past the specified maximum (minimum) value. 
        /// </summary>
        public bool Wrap;
        private bool inc = true;
        /// <summary>
        /// If true, the sequence will be decremented.
        /// </summary>
        public bool Decrement {
            get { return !inc; }
            set { inc = !value; }
        }
        /// <summary>
        /// If true, the sequence will be incremented. This is the default. 
        /// </summary>
        public bool Increment {
            get { return inc; }
            set { inc = value; }
        }
        internal uint flags {
            get {
                uint ret = 0;
                ret |= Wrap ? DbConstants.DB_SEQ_WRAP : 0;
                ret |= Increment ? DbConstants.DB_SEQ_INC : DbConstants.DB_SEQ_DEC;
                return ret;
            }
        }

        private int cacheSz;
        internal bool cacheSzIsSet;
        /// <summary>
        /// The number of elements cached by a sequence handle.
        /// </summary>
        public int CacheSize {
            get { return cacheSz; }
            set {
                cacheSz = value;
                cacheSzIsSet = true;
            }
        }

        private Int64 _min;
        private Int64 _max;
        internal bool rangeIsSet;
        /// <summary>
        /// The minimum value in the sequence.
        /// </summary>
        public Int64 Min { get { return _min; } }
        /// <summary>
        /// The maximum value in the sequence.
        /// </summary>
        public Int64 Max { get { return _max; } }
        /// <summary>
        /// Set the minimum and maximum values in the sequence.
        /// </summary>
        /// <param name="Max">The maximum value in the sequence.</param>
        /// <param name="Min">The minimum value in the sequence.</param>
        public void SetRange(Int64 Min, Int64 Max) {
            _min = Min;
            _max = Max;
            rangeIsSet = true;
        }
    }
}

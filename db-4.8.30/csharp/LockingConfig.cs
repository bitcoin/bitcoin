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
    /// A class representing configuration parameters for a
    /// <see cref="DatabaseEnvironment"/>'s locking subsystem.
    /// </summary>
    public class LockingConfig {
        private byte[,] _conflicts;
        private int _nmodes;
        /// <summary>
        /// The locking conflicts matrix. 
        /// </summary>
        /// <remarks>
        /// <para>
        /// If Conflicts is never set, a standard conflicts array is used; see
        /// Standard Lock Modes in the Programmer's Reference Guide for more
        /// information.
        /// </para>
        /// <para>
        /// Conflicts parameter is an nmodes by nmodes array. A non-0 value for
        /// the array element indicates that requested_mode and held_mode
        /// conflict:
        /// <code>
        /// conflicts[requested_mode][held_mode] 
        /// </code>
        /// </para>
        /// <para>The not-granted mode must be represented by 0.</para>
        /// <para>
        /// If the database environment already exists when
        /// <see cref="DatabaseEnvironment.Open"/> is called, the value of
        /// Conflicts will be ignored.
        /// </para>
        /// </remarks>
        public byte[,] Conflicts {
            get { return _conflicts; }
            set {
                double sz;
                sz = Math.Sqrt(value.Length);
                if (Math.Truncate(sz) == sz) {
                    _conflicts = value;
                    _nmodes = (int)sz;
                } else
                    throw new ArgumentException("Conflicts matrix must be square.");
            }
        }

        private uint _maxlockers;
        internal bool maxLockersIsSet;
        /// <summary>
        /// The maximum number of simultaneous locking entities supported by the
        /// Berkeley DB environment
        /// </summary>
        /// <remarks>
        /// <para>
        /// This value is used by <see cref="DatabaseEnvironment.Open"/> to
        /// estimate how much space to allocate for various lock-table data
        /// structures. The default value is 1000 lockers. For specific
        /// information on configuring the size of the lock subsystem, see
        /// Configuring locking: sizing the system in the Programmer's Reference
        /// Guide.
        /// </para>
        /// <para>
        /// If the database environment already exists when
        /// <see cref="DatabaseEnvironment.Open"/> is called, the value of
        /// MaxLockers will be ignored.
        /// </para>
        /// </remarks>
        public uint MaxLockers {
            get { return _maxlockers; }
            set {
                maxLockersIsSet = true;
                _maxlockers = value;
            }
        }
        private uint _maxlocks;
        internal bool maxLocksIsSet;
        /// <summary>
        /// The maximum number of locks supported by the Berkeley DB
        /// environment.
        /// </summary>
        /// <remarks>
        /// <para>
        /// This value is used by <see cref="DatabaseEnvironment.Open"/> to
        /// estimate how much space to allocate for various lock-table data
        /// structures. The default value is 1000 lockers. For specific
        /// information on configuring the size of the lock subsystem, see
        /// Configuring locking: sizing the system in the Programmer's Reference
        /// Guide.
        /// </para>
        /// <para>
        /// If the database environment already exists when
        /// <see cref="DatabaseEnvironment.Open"/> is called, the value of
        /// MaxLocks will be ignored.
        /// </para>
        /// </remarks>
        public uint MaxLocks {
            get { return _maxlocks; }
            set {
                maxLocksIsSet = true;
                _maxlocks = value;
            }
        }
        private uint _maxobjects;
        internal bool maxObjectsIsSet;
        /// <summary>
        /// The maximum number of locked objects supported by the Berkeley DB
        /// environment.
        /// </summary>
        /// <remarks>
        /// <para>
        /// This value is used by <see cref="DatabaseEnvironment.Open"/> to
        /// estimate how much space to allocate for various lock-table data
        /// structures. The default value is 1000 lockers. For specific
        /// information on configuring the size of the lock subsystem, see
        /// Configuring locking: sizing the system in the Programmer's Reference
        /// Guide.
        /// </para>
        /// <para>
        /// If the database environment already exists when
        /// <see cref="DatabaseEnvironment.Open"/> is called, the value of
        /// MaxObjects will be ignored.
        /// </para>
        /// </remarks>
        public uint MaxObjects {
            get { return _maxobjects; }
            set {
                maxObjectsIsSet = true;
                _maxobjects = value;
            }
        }
        private uint _partitions;
        internal bool partitionsIsSet;
        /// <summary>
        /// The number of lock table partitions in the Berkeley DB environment.
        /// </summary>
        /// <remarks>
        /// <para>
        /// The default value is 10 times the number of CPUs on the system if
        /// there is more than one CPU. Increasing the number of partitions can
        /// provide for greater throughput on a system with multiple CPUs and
        /// more than one thread contending for the lock manager. On single
        /// processor systems more than one partition may increase the overhead
        /// of the lock manager. Systems often report threading contexts as
        /// CPUs. If your system does this, set the number of partitions to 1 to
        /// get optimal performance.
        /// </para>
        /// <para>
        /// If the database environment already exists when
        /// <see cref="DatabaseEnvironment.Open"/> is called, the value of
        /// Partitions will be ignored.
        /// </para>
        /// </remarks>
        public uint Partitions {
            get { return _partitions; }
            set {
                partitionsIsSet = true;
                _partitions = value;
            }
        }

        /// <summary>
        /// If non-null, the deadlock detector is to be run whenever a lock
        /// conflict occurs, lock request(s) should be rejected according to the
        /// specified policy.
        /// </summary>
        /// <remarks>
        /// <para>
        /// As transactions acquire locks on behalf of a single locker ID,
        /// rejecting a lock request associated with a transaction normally
        /// requires the transaction be aborted.
        /// </para>
        /// </remarks>
        public DeadlockPolicy DeadlockResolution;
    }
}

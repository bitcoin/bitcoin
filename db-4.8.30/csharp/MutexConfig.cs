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
    /// <see cref="DatabaseEnvironment"/>'s mutex subsystem.
    /// </summary>
    public class MutexConfig {
        internal bool alignmentIsSet;
        private uint _alignment;
        /// <summary>
        /// The mutex alignment, in bytes.
        /// </summary>
        /// <remarks>
        /// <para>
        /// It is sometimes advantageous to align mutexes on specific byte
        /// boundaries in order to minimize cache line collisions. Alignment
        /// specifies an alignment for mutexes allocated by Berkeley DB.
        /// </para>
        /// <para>
        /// If the database environment already exists when
        /// <see cref="DatabaseEnvironment.Open"/> is called, the value of
        /// Alignment will be ignored.
        /// </para>
        /// </remarks>
        public uint Alignment {
            get { return _alignment; }
            set {
                alignmentIsSet = true;
                _alignment = value;
            }
        }

        internal bool incrementIsSet;
        private uint _increment;
        /// <summary>
        /// Configure the number of additional mutexes to allocate.
        /// </summary>
        /// <remarks>
        /// <para>
        /// If both Increment and <see cref="MaxMutexes"/> are set, the value of
        /// Increment will be silently ignored.
        /// </para>
        /// <para>
        /// If the database environment already exists when
        /// <see cref="DatabaseEnvironment.Open"/> is called, the value of
        /// Increment will be ignored.
        /// </para>
        /// </remarks>
        public uint Increment {
            get { return _increment; }
            set {
                incrementIsSet = true;
                _increment = value;
            }
        }

        internal bool maxIsSet;
        private uint _max;
        /// <summary>
        /// The total number of mutexes to allocate.
        /// </summary>
        /// <remarks>
        /// <para>
        /// Berkeley DB allocates a default number of mutexes based on the
        /// initial configuration of the database environment. That default
        /// calculation may be too small if the application has an unusual need
        /// for mutexes (for example, if the application opens an unexpectedly
        /// large number of databases) or too large (if the application is
        /// trying to minimize its memory footprint). MaxMutexes is used to
        /// specify an absolute number of mutexes to allocate.
        /// </para>
        /// <para>
        /// If both <see cref="Increment"/> and MaxMutexes are set, the value of
        /// Increment will be silently ignored.
        /// </para>
        /// <para>
        /// If the database environment already exists when
        /// <see cref="DatabaseEnvironment.Open"/> is called, the value of
        /// MaxMutexes will be ignored.
        /// </para>
        /// </remarks>
        public uint MaxMutexes {
            get { return _max; }
            set {
                maxIsSet = true;
                _max = value;
            }
        }

        internal bool numTASIsSet;
        private uint _numTAS;
        /// <summary>
        /// The number of spins test-and-set mutexes should execute before
        /// blocking. 
        /// </summary>
        public uint NumTestAndSetSpins {
            get { return _numTAS; }
            set {
                numTASIsSet = true;
                _numTAS = value;
            }
        }

        
    }
}

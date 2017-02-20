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
    /// A class representing configuration parameters for <see cref="Cursor"/>
    /// </summary>
    public class CursorConfig {
        /// <summary>
        /// The isolation degree the cursor should use.
        /// </summary>
        /// <remarks>
        /// <para>
        /// <see cref="Isolation.DEGREE_TWO"/> ensures the stability of the
        /// current data item read by this cursor but permits data read by this
        /// cursor to be modified or deleted prior to the commit of the
        /// transaction for this cursor. 
        /// </para>
        /// <para>
        /// <see cref="Isolation.DEGREE_ONE"/> allows read operations performed
        /// by the cursor to return modified but not yet committed data.
        /// Silently ignored if the <see cref="DatabaseConfig.ReadUncommitted"/>
        /// was not specified when the underlying database was opened. 
        /// </para>
        /// </remarks>
        public Isolation IsolationDegree;
        /// <summary>
        /// If true, specify that the cursor will be used to update the
        /// database. The underlying database environment must have been opened
        /// with <see cref="DatabaseEnvironmentConfig.UseCDB"/> set. 
        /// </summary>
        public bool WriteCursor;
        /// <summary>
        /// <para>
        /// Configure a transactional cursor to operate with read-only snapshot
        /// isolation. For databases with <see cref="DatabaseConfig.UseMVCC"/>
        /// set, data values will be read as they are when the cursor is opened,
        /// without taking read locks.
        /// </para>
        /// <para>
        /// This setting implicitly begins a transaction that is committed when
        /// the cursor is closed.
        /// </para>
        /// <para>
        /// This setting is silently ignored if
        /// <see cref="DatabaseConfig.UseMVCC"/> is not set on the underlying
        /// database or if a transaction is supplied to
        /// <see cref="BaseDatabase.Cursor"/>
        /// </para>
        /// </summary>
        public bool SnapshotIsolation;
        /// <summary>
        /// The cache priority for pages referenced by the cursor.
        /// </summary>
        /// <remarks>
        /// The priority of a page biases the replacement algorithm to be more
        /// or less likely to discard a page when space is needed in the buffer
        /// pool. The bias is temporary, and pages will eventually be discarded
        /// if they are not referenced again. The setting is only advisory, and
        /// does not guarantee pages will be treated in a specific way.
        /// </remarks>
        public CachePriority Priority;

        /// <summary>
        /// Instantiate a new CursorConfig object
        /// </summary>
        public CursorConfig() {
            IsolationDegree = Isolation.DEGREE_THREE;
        }
        internal uint flags {
            get {
                uint ret = 0;
                ret |= (IsolationDegree == Isolation.DEGREE_ONE)
                    ? DbConstants.DB_READ_UNCOMMITTED : 0;
                ret |= (IsolationDegree == Isolation.DEGREE_TWO)
                    ? DbConstants.DB_READ_COMMITTED : 0;
                ret |= (WriteCursor) ? DbConstants.DB_WRITECURSOR : 0;
                ret |= (SnapshotIsolation) ? DbConstants.DB_TXN_SNAPSHOT : 0;

                return ret;
            }
        }
    }
}
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
    /// A class representing a transaction that must be resolved by the
    /// application following <see cref="DatabaseEnvironment.Recover"/>.
    /// </summary>
    public class PreparedTransaction {
        private Transaction trans;
        private byte[] txnid;

        internal PreparedTransaction(DB_PREPLIST prep) {
            trans = new Transaction(prep.txn);
            txnid = prep.gid;
        }

        /// <summary>
        /// The transaction which must be committed, aborted or discarded.
        /// </summary>
        public Transaction Txn { get { return trans; } }
        /// <summary>
        /// The global transaction ID for the transaction. The global
        /// transaction ID is the one specified when the transaction was
        /// prepared. The application is responsible for ensuring uniqueness
        /// among global transaction IDs.
        /// </summary>
        public byte[] GlobalID { get { return txnid;}}
    }
}

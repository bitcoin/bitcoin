/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;

namespace BerkeleyDB {
    
    /// <summary>
    /// A function to call after the record number has been selected but before
    /// the data has been stored into the database.
    /// </summary>
    /// <param name="data">The data to be stored.</param>
    /// <param name="recno">The generated record number.</param>
    public delegate void AppendRecordDelegate(DatabaseEntry data, uint recno);
    /// <summary>
    /// A function to store a compressed key/data pair into a supplied buffer.
    /// </summary>
    /// <param name="prevKey">The key immediately preceding the application supplied key.</param>
    /// <param name="prevData">The data associated with prevKey.</param>
    /// <param name="key">The application supplied key.</param>
    /// <param name="data">The application supplied data. </param>
    /// <param name="dest">The compressed data to be stored in the
    /// database.</param>
    /// <param name="size">The number of compressed bytes written to
    /// <paramref name="dest"/>, or the required size of
    /// <paramref name="dest"/>, if too small.</param>
    /// <returns>True on success, false if dest is too small to contain the
    /// compressed data.  All other errors should throw an exception.</returns>
    public delegate bool BTreeCompressDelegate(DatabaseEntry prevKey,
        DatabaseEntry prevData, DatabaseEntry key,
        DatabaseEntry data, ref byte[] dest, out int size);
    /// <summary>
    /// A function to decompress a key/data pair from a supplied buffer.
    /// </summary>
    /// <param name="prevKey">The key immediately preceding the key being decompressed.</param>
    /// <param name="prevData">The data associated with prevKey.</param>
    /// <param name="compressed">The data stored in the tree, that is, the compressed data.</param>
    /// <param name="bytesRead">The number of bytes read from <paramref name="compressed"/>.</param>
    /// <returns>Two new DatabaseEntry objects representing the decompressed
    /// key/data pair.</returns>
    public delegate KeyValuePair<DatabaseEntry, DatabaseEntry>
        BTreeDecompressDelegate(DatabaseEntry prevKey,
        DatabaseEntry prevData, byte[] compressed, out uint bytesRead);
    /// <summary>
    /// The application-specified feedback function called to report Berkeley DB
    /// operation progress.
    /// </summary>
    /// <param name="opcode">
    /// An operation code specifying the Berkley DB operation
    /// </param>
    /// <param name="percent">
    /// The percent of the operation that has been completed, specified as an
    /// integer value between 0 and 100.
    /// </param>
    public delegate void DatabaseFeedbackDelegate(
        DatabaseFeedbackEvent opcode, int percent);
    /// <summary>
    /// An application-specified comparison function.
    /// </summary>
    /// <param name="dbt1">The application supplied key.</param>
    /// <param name="dbt2">The current tree's key.</param>
    /// <returns>
    /// An integer value less than, equal to, or greater than zero if the first
    /// key parameter is considered to be respectively less than, equal to, or
    /// greater than the second key parameter.
    /// </returns>
    public delegate int EntryComparisonDelegate(
        DatabaseEntry dbt1, DatabaseEntry dbt2);
    /// <summary>
    /// The application-specified feedback function called to report Berkeley DB
    /// operation progress.
    /// </summary>
    /// <param name="opcode">
    /// An operation code specifying the Berkley DB operation
    /// </param>
    /// <param name="percent">
    /// The percent of the operation that has been completed, specified as an
    /// integer value between 0 and 100.
    /// </param>
    public delegate void EnvironmentFeedbackDelegate(
        EnvironmentFeedbackEvent opcode, int percent);
    /// <summary>
    /// The application-specified error reporting function.
    /// </summary>
    /// <param name="errPrefix">The prefix string</param>
    /// <param name="errMessage">The error message string</param>
    public delegate void ErrorFeedbackDelegate(
        string errPrefix, string errMessage);
    /// <summary>
    /// The application's event notification function.
    /// </summary>
    /// <param name="eventcode">
    /// An even code specifying the Berkeley DB event
    /// </param>
    /// <param name="event_info">
    /// Additional information describing an event. By default, event_info is
    /// null; specific events may pass non-null values, in which case the event
    /// will also describe the information's structure.
    /// </param>
    public delegate void EventNotifyDelegate(
        NotificationEvent eventcode, byte[] event_info);
    /// <summary>
    /// 
    /// </summary>
    /// <param name="key"></param>
    /// <param name="data"></param>
    /// <param name="foreignkey"></param>
    /// <returns></returns>
    public delegate DatabaseEntry ForeignKeyNullifyDelegate(
        DatabaseEntry key, DatabaseEntry data, DatabaseEntry foreignkey);
    /// <summary>
    /// The application-specified hash function.
    /// </summary>
    /// <param name="data">
    /// A byte string representing a key in the database
    /// </param>
    /// <returns>The hashed value of <paramref name="data"/></returns>
    public delegate uint HashFunctionDelegate(byte[] data);
    /// <summary>
    /// The function used to transmit data using the replication application's
    /// communication infrastructure.
    /// </summary>
    /// <param name="control">
    /// The first of the two data elements to be transmitted by the send
    /// function.
    /// </param>
    /// <param name="rec">
    /// The second of the two data elements to be transmitted by the send
    /// function.
    /// </param>
    /// <param name="lsn">
    /// If the type of message to be sent has an LSN associated with it, then
    /// this is the LSN of the record being sent. This LSN can be used to
    /// determine that certain records have been processed successfully by
    /// clients.
    /// </param>
    /// <param name="envid">
    /// <para>
    /// A positive integer identifier that specifies the replication environment
    /// to which the message should be sent.
    /// </para>
    /// <para>
    /// The special identifier DB_EID_BROADCAST indicates that a message should
    /// be broadcast to every environment in the replication group. The
    /// application may use a true broadcast protocol or may send the message
    /// in sequence to each machine with which it is in communication. In both
    /// cases, the sending site should not be asked to process the message.
    /// </para>
    /// <para>
    /// The special identifier DB_EID_INVALID indicates an invalid environment
    /// ID. This may be used to initialize values that are subsequently checked
    /// for validity. 
    /// </para>
    /// </param>
    /// <param name="flags">XXX: TBD</param>
    /// <returns>0 on success and non-zero on failure</returns>
    public delegate int ReplicationTransportDelegate(DatabaseEntry control,
    DatabaseEntry rec, LSN lsn, int envid, uint flags);
    /// <summary>
    /// The function that creates the set of secondary keys corresponding to a
    /// given primary key and data pair. 
    /// </summary>
    /// <param name="key">The primary key</param>
    /// <param name="data">The primary data item</param>
    /// <returns>The secondary key(s)</returns>
    public delegate DatabaseEntry SecondaryKeyGenDelegate(
        DatabaseEntry key, DatabaseEntry data);
    /// <summary>
    /// A function which returns a unique identifier pair for a thread of
    /// control in a Berkeley DB application.
    /// </summary>
    /// <returns>
    /// A DbThreadID object describing the current thread of control
    /// </returns>
    public delegate DbThreadID SetThreadIDDelegate();
    /// <summary>
    /// A function which returns an identifier pair for a thread of control
    /// formatted for display.
    /// </summary>
    /// <param name="info">The thread of control to format</param>
    /// <returns>The formatted identifier pair</returns>
    public delegate string SetThreadNameDelegate(DbThreadID info);
    /// <summary>
    /// A function which returns whether the thread of control, identified by
    /// <paramref name="info"/>, is still running.
    /// </summary>
    /// <param name="info">The thread of control to check</param>
    /// <param name="procOnly">
    /// If true, return only if the process is alive, and the
    /// <see cref="DbThreadID.threadID"/> portion of <paramref name="info"/>
    /// should be ignored.
    /// </param>
    /// <returns>True if the tread is alive, false otherwise.</returns>
    public delegate bool ThreadIsAliveDelegate(DbThreadID info, bool procOnly);
}

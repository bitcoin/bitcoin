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
    /// A class representing configuration parameters for a
    /// <see cref="DatabaseEnvironment"/>'s replication subsystem.
    /// </summary>
    public class ReplicationConfig {
        internal Dictionary<ReplicationHostAddress, bool> remoteAddrs;

        /// <summary>
        /// Instantiate a new ReplicationConfig object with default
        /// configuration values.
        /// </summary>
        public ReplicationConfig() {
            remoteAddrs = new Dictionary<ReplicationHostAddress, bool>();
        }

        #region Config Flags
        /// <summary>
        /// If true, the replication master will send groups of records to the
        /// clients in a single network transfer
        /// </summary>
        public bool BulkTransfer;
        /// <summary>
        /// If true, the client will delay synchronizing to a newly declared
        /// master (defaults to false). Clients configured in this way will
        /// remain unsynchronized until the application calls
        /// <see cref="DatabaseEnvironment.RepSync"/>. 
        /// </summary>
        public bool DelayClientSync;
        /// <summary>
        /// If true, master leases will be used for this site (defaults to
        /// false). 
        /// </summary>
        /// <remarks>
        /// Configuring this option may result in a 
        /// <see cref="LeaseExpiredException"/> when attempting to read entries
        /// from a database after the site's master lease has expired.
        /// </remarks>
        public bool UseMasterLeases;
        /// <summary>
        /// If true, the replication master will not automatically re-initialize
        /// outdated clients (defaults to false). 
        /// </summary>
        public bool NoAutoInit;
        /// <summary>
        /// If true, Berkeley DB method calls that would normally block while
        /// clients are in recovery will return errors immediately (defaults to
        /// false).
        /// </summary>
        public bool NoBlocking;
        /// <summary>
        /// If true, the Replication Manager will observe the strict "majority"
        /// rule in managing elections, even in a group with only 2 sites. This
        /// means the client in a 2-site group will be unable to take over as
        /// master if the original master fails or becomes disconnected. (See
        /// the Elections section in the Berkeley DB Reference Guide for more
        /// information.) Both sites in the replication group should have the
        /// same value for this parameter.
        /// </summary>
        public bool Strict2Site;
        #endregion Config Flags

        #region Timeout Values
        private uint _ackTimeout;
        internal bool ackTimeoutIsSet;
        /// <summary>
        /// The amount of time the replication manager's transport function
        /// waits to collect enough acknowledgments from replication group
        /// clients, before giving up and returning a failure indication. The
        /// default wait time is 1 second.
        /// </summary>
        public uint AckTimeout {
            get { return _ackTimeout; }
            set {
                _ackTimeout = value;
                ackTimeoutIsSet = true;
            }
        }

        private uint _checkpointDelay;
        internal bool checkpointDelayIsSet;
        /// <summary>
        /// The amount of time a master site will delay between completing a
        /// checkpoint and writing a checkpoint record into the log.
        /// </summary>
        /// <remarks>
        /// This delay allows clients to complete their own checkpoints before
        /// the master requires completion of them. The default is 30 seconds.
        /// If all databases in the environment, and the environment's
        /// transaction log, are configured to reside in memory (never preserved
        /// to disk), then, although checkpoints are still necessary, the delay
        /// is not useful and should be set to 0.
        /// </remarks>
        public uint CheckpointDelay {
            get { return _checkpointDelay; }
            set {
                _checkpointDelay = value;
                checkpointDelayIsSet = true;
            }
        }

        private uint _connectionRetry;
        internal bool connectionRetryIsSet;
        /// <summary>
        /// The amount of time the replication manager will wait before trying
        /// to re-establish a connection to another site after a communication
        /// failure. The default wait time is 30 seconds.
        /// </summary>
        public uint ConnectionRetry {
            get { return _connectionRetry; }
            set {
                _connectionRetry = value;
                connectionRetryIsSet = true;
            }
        }

        private uint _electionTimeout;
        internal bool electionTimeoutIsSet;
        /// <summary>
        /// The timeout period for an election. The default timeout is 2
        /// seconds.
        /// </summary>
        public uint ElectionTimeout {
            get { return _electionTimeout; }
            set {
                _electionTimeout = value;
                electionTimeoutIsSet = true;
            }
        }

        private uint _electionRetry;
        internal bool electionRetryIsSet;
        /// <summary>
        /// Configure the amount of time the replication manager will wait
        /// before retrying a failed election. The default wait time is 10
        /// seconds. 
        /// </summary>
        public uint ElectionRetry {
            get { return _electionRetry; }
            set {
                _electionRetry = value;
                electionRetryIsSet = true;
            }
        }

        private uint _fullElectionTimeout;
        internal bool fullElectionTimeoutIsSet;
        /// <summary>
        /// An optional configuration timeout period to wait for full election
        /// participation the first time the replication group finds a master.
        /// By default this option is turned off and normal election timeouts
        /// are used. (See the Elections section in the Berkeley DB Reference
        /// Guide for more information.) 
        /// </summary>
        public uint FullElectionTimeout {
            get { return _fullElectionTimeout; }
            set {
                _fullElectionTimeout = value;
                fullElectionTimeoutIsSet = true;
            }
        }

        private uint _heartbeatMonitor;
        internal bool heartbeatMonitorIsSet;
        /// <summary>
        /// The amount of time the replication manager, running at a client
        /// site, waits for some message activity on the connection from the
        /// master (heartbeats or other messages) before concluding that the
        /// connection has been lost. When 0 (the default), no monitoring is
        /// performed.
        /// </summary>
        public uint HeartbeatMonitor {
            get { return _heartbeatMonitor; }
            set {
                _heartbeatMonitor = value;
                heartbeatMonitorIsSet = true;
            }
        }

        private uint _heartbeatSend;
        internal bool heartbeatSendIsSet;
        /// <summary>
        /// The frequency at which the replication manager, running at a master
        /// site, broadcasts a heartbeat message in an otherwise idle system.
        /// When 0 (the default), no heartbeat messages will be sent. 
        /// </summary>
        public uint HeartbeatSend {
            get { return _heartbeatSend; }
            set {
                _heartbeatSend = value;
                heartbeatSendIsSet = true;
            }
        }

        private uint _leaseTimeout;
        internal bool leaseTimeoutIsSet;
        /// <summary>
        /// The amount of time a client grants its master lease to a master.
        /// When using master leases all sites in a replication group must use
        /// the same lease timeout value. There is no default value. If leases
        /// are desired, this method must be called prior to calling
        /// <see cref="DatabaseEnvironment.RepStartClient"/> or
        /// <see cref="DatabaseEnvironment.RepStartMaster"/>.
        /// </summary>
        public uint LeaseTimeout {
            get { return _leaseTimeout; }
            set {
                _leaseTimeout = value;
                leaseTimeoutIsSet = true;
            }
        }
        #endregion Timeout Values

        private uint _clockskewFast;
        private uint _clockskewSlow;
        internal bool clockskewIsSet;
        /// <summary>
        /// The value, relative to <see cref="ClockskewSlow"/>, of the fastest
        /// clock in the group of sites.
        /// </summary>
        public uint ClockskewFast { get { return _clockskewFast; } }
        /// <summary>
        /// The value of the slowest clock in the group of sites.
        /// </summary>
        public uint ClockskewSlow { get { return _clockskewSlow; } }
        /// <summary>
        /// Set the clock skew ratio among replication group members based on
        /// the fastest and slowest measurements among the group for use with
        /// master leases.
        /// </summary>
        /// <remarks>
        /// <para>
        /// Calling this method is optional, the default values for clock skew
        /// assume no skew. The user must also configure leases via
        /// <see cref="UseMasterLeases"/>. Additionally, the user must also
        /// set the master lease timeout via <see cref="LeaseTimeout"/> and
        /// the number of sites in the replication group via
        /// <see cref="NSites"/>. These settings may be configured in any
        /// order. For a description of the clock skew values, see Clock skew 
        /// in the Berkeley DB Programmer's Reference Guide. For a description
        /// of master leases, see Master leases in the Berkeley DB Programmer's
        /// Reference Guide.
        /// </para>
        /// <para>
        /// These arguments can be used to express either raw measurements of a
        /// clock timing experiment or a percentage across machines. For
        /// instance a group of sites have a 2% variance, then
        /// <paramref name="fast"/> should be set to 102, and
        /// <paramref name="slow"/> should be set to 100. Or, for a 0.03%
        /// difference, you can use 10003 and 10000 respectively.
        /// </para>
        /// </remarks>
        /// <param name="fast">
        /// The value, relative to <paramref name="slow"/>, of the fastest clock
        /// in the group of sites.
        /// </param>
        /// <param name="slow">
        /// The value of the slowest clock in the group of sites.
        /// </param>
        public void Clockskew(uint fast, uint slow) {
            clockskewIsSet = true;
            _clockskewSlow = slow;
            _clockskewFast = fast;
        }

        private uint _nsites;
        internal bool nsitesIsSet;
        /// <summary>
        /// The total number of sites in the replication group.
        /// </summary>
        /// <remarks>
        /// <para>
        /// This setting is typically used by applications which use the
        /// Berkeley DB library "replication manager" support. (However, see
        /// also <see cref="DatabaseEnvironment.RepHoldElection"/>, the
        /// description of the nsites parameter.)
        /// </para>
        /// </remarks>
        public uint NSites {
            get { return _nsites; }
            set {
                _nsites = value;
                nsitesIsSet = true;
            }
        }
        private uint _priority;
        internal bool priorityIsSet;
        /// <summary>
        /// The database environment's priority in replication group elections.
        /// A special value of 0 indicates that this environment cannot be a
        /// replication group master. If not configured, then a default value
        /// of 100 is used.
        /// </summary>
        public uint Priority {
            get { return _priority; }
            set {
                _priority = value;
                priorityIsSet = true;
            }
        }

        private uint _retransmissionRequestMin;
        private uint _retransmissionRequestMax;
        internal bool retransmissionRequestIsSet;
        /// <summary>
        /// The minimum number of microseconds a client waits before requesting
        /// retransmission.
        /// </summary>
        public uint RetransmissionRequestMin {
            get { return _retransmissionRequestMin; }
        }
        /// <summary>
        /// The maximum number of microseconds a client waits before requesting
        /// retransmission.
        /// </summary>
        public uint RetransmissionRequestMax {
            get { return _retransmissionRequestMax; }
        }
        /// <summary>
        /// Set a threshold for the minimum and maximum time that a client waits
        /// before requesting retransmission of a missing message.
        /// </summary>
        /// <remarks>
        /// <para>
        /// If the client detects a gap in the sequence of incoming log records
        /// or database pages, Berkeley DB will wait for at least
        /// <paramref name="min"/> microseconds before requesting retransmission
        /// of the missing record. Berkeley DB will double that amount before
        /// requesting the same missing record again, and so on, up to a
        /// maximum threshold of <paramref name="max"/> microseconds.
        /// </para>
        /// <para>
        /// These values are thresholds only. Since Berkeley DB has no thread
        /// available in the library as a timer, the threshold is only checked
        /// when a thread enters the Berkeley DB library to process an incoming
        /// replication message. Any amount of time may have passed since the
        /// last message arrived and Berkeley DB only checks whether the amount
        /// of time since a request was made is beyond the threshold value or
        /// not.
        /// </para>
        /// <para>
        /// By default the minimum is 40000 and the maximum is 1280000 (1.28
        /// seconds). These defaults are fairly arbitrary and the application
        /// likely needs to adjust these. The values should be based on expected
        /// load and performance characteristics of the master and client host
        /// platforms and transport infrastructure as well as round-trip message
        /// time.
        /// </para></remarks>
        /// <param name="min">
        /// The minimum number of microseconds a client waits before requesting
        /// retransmission.
        /// </param>
        /// <param name="max">
        /// The maximum number of microseconds a client waits before requesting
        /// retransmission.
        /// </param>
        public void RetransmissionRequest(uint min, uint max) {
            retransmissionRequestIsSet = true;
            _retransmissionRequestMin = min;
            _retransmissionRequestMax = max;
        }

        private uint _gbytes;
        private uint _bytes;
        internal bool transmitLimitIsSet;
        /// <summary>
        /// The gigabytes component of the byte-count limit on the amount of
        /// data that will be transmitted from a site in response to a single
        /// message processed by
        /// <see cref="DatabaseEnvironment.RepProcessMessage"/>.
        /// </summary>
        public uint TransmitLimitGBytes { get { return _gbytes; } }
        /// <summary>
        /// The bytes component of the byte-count limit on the amount of data
        /// that will be transmitted from a site in response to a single
        /// message processed by
        /// <see cref="DatabaseEnvironment.RepProcessMessage"/>.
        /// </summary>
        public uint TransmitLimitBytes { get { return _bytes; } }
        /// <summary>
        /// Set a byte-count limit on the amount of data that will be
        /// transmitted from a site in response to a single message processed by
        /// <see cref="DatabaseEnvironment.RepProcessMessage"/>. The limit is
        /// not a hard limit, and the record that exceeds the limit is the last
        /// record to be sent. 
        /// </summary>
        /// <remarks>
        /// <para>
        /// Record transmission throttling is turned on by default with a limit
        /// of 10MB.
        /// </para>
        /// <para>
        /// If both <paramref name="GBytes"/> and <paramref name="Bytes"/> are
        /// zero, then the transmission limit is turned off.
        /// </para>
        /// </remarks>
        /// <param name="GBytes">
        /// The number of gigabytes which, when added to
        /// <paramref name="Bytes"/>, specifies the maximum number of bytes that
        /// will be sent in a single call to 
        /// <see cref="DatabaseEnvironment.RepProcessMessage"/>.
        /// </param>
        /// <param name="Bytes">
        /// The number of bytes which, when added to 
        /// <paramref name="GBytes"/>, specifies the maximum number of bytes
        /// that will be sent in a single call to
        /// <see cref="DatabaseEnvironment.RepProcessMessage"/>.
        /// </param>
        public void TransmitLimit(uint GBytes, uint Bytes) {
            transmitLimitIsSet = true;
            _gbytes = GBytes;
            _bytes = Bytes;
        }

        /// <summary>
        /// The delegate used to transmit data using the replication
        /// application's communication infrastructure.
        /// </summary>
        public ReplicationTransportDelegate Transport;

        /// <summary>
        /// Specify how master and client sites will handle acknowledgment of
        /// replication messages which are necessary for "permanent" records.
        /// The current implementation requires all sites in a replication group
        /// configure the same acknowledgement policy. 
        /// </summary>
        /// <seealso cref="AckTimeout"/>
        public AckPolicy RepMgrAckPolicy;

        /// <summary>
        /// The host information for the local system. 
        /// </summary>
        public ReplicationHostAddress RepMgrLocalSite;

        /// <summary>
        /// Add a new replication site to the replication manager's list of
        /// known sites. It is not necessary for all sites in a replication
        /// group to know about all other sites in the group. 
        /// </summary>
        /// <remarks>
        /// Currently, the replication manager framework only supports a single
        /// client peer, and the last specified peer is used.
        /// </remarks>
        /// <param name="host">The remote site's address</param>
        /// <param name="isPeer">
        /// If true, configure client-to-client synchronization with the
        /// specified remote site.
        /// </param>
        public void AddRemoteSite(ReplicationHostAddress host, bool isPeer) {
            remoteAddrs.Add(host, isPeer);
        }
    }
}
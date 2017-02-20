/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 */
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Xml;
using NUnit.Framework;
using BerkeleyDB;

namespace CsharpAPITest
{
    [TestFixture]
    public class ReplicationConfigTest
    {
        private string testFixtureName;
        private string testName;

        [TestFixtureSetUp]
        public void RunBeforeTests()
        {
            testFixtureName = "ReplicationConfigTest";
        }

        [Test]
        public void TestConfig()
        {
            testName = "TestConfig";

            ReplicationConfig repConfig = new ReplicationConfig();
            XmlElement xmlElem = Configuration.TestSetUp(
                testFixtureName, testName);
            Config(xmlElem, ref repConfig, true);
            Confirm(xmlElem, repConfig, true);

            repConfig.Clockskew(102, 100);
            Assert.AreEqual(102, repConfig.ClockskewFast);
            Assert.AreEqual(100, repConfig.ClockskewSlow);

            repConfig.TransmitLimit(1, 1024);
            Assert.AreEqual(1, repConfig.TransmitLimitGBytes);
            Assert.AreEqual(1024, repConfig.TransmitLimitBytes);

            repConfig.RetransmissionRequest(10, 100);
            Assert.AreEqual(100, repConfig.RetransmissionRequestMax);
            Assert.AreEqual(10, repConfig.RetransmissionRequestMin);
        }

        [Test]
        public void TestRepMgrLocalSite()
        {
            string host = "127.0.0.0";
            uint port = 8888;
            testName = "TestRepMgrLocalSite";

            ReplicationConfig repConfig1 = new ReplicationConfig();
            repConfig1.RepMgrLocalSite = new ReplicationHostAddress();
            repConfig1.RepMgrLocalSite.Host = host;
            repConfig1.RepMgrLocalSite.Port = port;
            Assert.AreEqual(host, repConfig1.RepMgrLocalSite.Host);
            Assert.AreEqual(port, repConfig1.RepMgrLocalSite.Port);

            ReplicationConfig repConfig2 = new ReplicationConfig();
            repConfig2.RepMgrLocalSite =
                new ReplicationHostAddress(host, port);
            Assert.AreEqual(host, repConfig2.RepMgrLocalSite.Host);
            Assert.AreEqual(port, repConfig2.RepMgrLocalSite.Port);

            ReplicationConfig repConfig3 = new ReplicationConfig();
            repConfig3.RepMgrLocalSite =
                new ReplicationHostAddress(host + ":" + port);
            Assert.AreEqual(host, repConfig3.RepMgrLocalSite.Host);
            Assert.AreEqual(port, repConfig3.RepMgrLocalSite.Port);
        }
        
        [Test]
        public void TestRepMgrAckPolicy()
        {
            testName = "TestRepMgrAckPolicy";

            ReplicationConfig repConfig = new ReplicationConfig();
            repConfig.RepMgrAckPolicy = AckPolicy.ALL;
            Assert.AreEqual(AckPolicy.ALL, 
                repConfig.RepMgrAckPolicy);

            repConfig.RepMgrAckPolicy = AckPolicy.ALL_PEERS;
            Assert.AreEqual(AckPolicy.ALL_PEERS,
                repConfig.RepMgrAckPolicy);

            repConfig.RepMgrAckPolicy = AckPolicy.NONE;
            Assert.AreEqual(AckPolicy.NONE,
                repConfig.RepMgrAckPolicy);

            repConfig.RepMgrAckPolicy = AckPolicy.ONE;
            Assert.AreEqual(AckPolicy.ONE,
                repConfig.RepMgrAckPolicy);

            repConfig.RepMgrAckPolicy = AckPolicy.ONE_PEER;
            Assert.AreEqual(AckPolicy.ONE_PEER,
                repConfig.RepMgrAckPolicy);

            repConfig.RepMgrAckPolicy = AckPolicy.QUORUM;
            Assert.AreEqual(AckPolicy.QUORUM,
                repConfig.RepMgrAckPolicy);
        }

        public static void Confirm(XmlElement xmlElement, 
            ReplicationConfig cfg, bool compulsory)
        {
            Configuration.ConfirmUint(xmlElement,
                "AckTimeout", cfg.AckTimeout,
                compulsory);
            Configuration.ConfirmBool(xmlElement, "BulkTransfer",
                cfg.BulkTransfer, compulsory);
            Configuration.ConfirmUint(xmlElement, "CheckpointDelay",
                cfg.CheckpointDelay, compulsory);
            Configuration.ConfirmUint(xmlElement, "ConnectionRetry",
                cfg.ConnectionRetry, compulsory);
            Configuration.ConfirmBool(xmlElement, "DelayClientSync",
                cfg.DelayClientSync, compulsory);
            Configuration.ConfirmUint(xmlElement, "ElectionRetry",
                cfg.ElectionRetry, compulsory);
            Configuration.ConfirmUint(xmlElement, "ElectionTimeout",
                cfg.ElectionTimeout, compulsory);
            Configuration.ConfirmUint(xmlElement, "FullElectionTimeout",
                cfg.FullElectionTimeout, compulsory);
            Configuration.ConfirmUint(xmlElement, "HeartbeatMonitor",
                cfg.HeartbeatMonitor, compulsory);
            Configuration.ConfirmUint(xmlElement, "HeartbeatSend",
                cfg.HeartbeatSend, compulsory);
            Configuration.ConfirmUint(xmlElement, "LeaseTimeout",
                cfg.LeaseTimeout, compulsory);
            Configuration.ConfirmBool(xmlElement, "NoAutoInit",
                cfg.NoAutoInit, compulsory);
            Configuration.ConfirmBool(xmlElement, "NoBlocking",
                cfg.NoBlocking, compulsory);
            Configuration.ConfirmUint(xmlElement, "NSites",
                cfg.NSites, compulsory);
            Configuration.ConfirmUint(xmlElement, "Priority",
                cfg.Priority, compulsory);
            Configuration.ConfirmAckPolicy(xmlElement, 
                "RepMgrAckPolicy", cfg.RepMgrAckPolicy, compulsory);
            Configuration.ConfirmReplicationHostAddress(xmlElement,
                "RepMgrLocalSite", cfg.RepMgrLocalSite, compulsory);
            Configuration.ConfirmBool(xmlElement, "Strict2Site",
                cfg.Strict2Site, compulsory);
            Configuration.ConfirmBool(xmlElement, "UseMasterLeases",
                cfg.UseMasterLeases, compulsory);
        }

        public static void Config(XmlElement xmlElement,
            ref ReplicationConfig cfg, bool compulsory)
        {
            uint uintValue = new uint();

            if (Configuration.ConfigUint(xmlElement, "AckTimeout", 
                ref uintValue, compulsory))
                cfg.AckTimeout = uintValue;
            Configuration.ConfigBool(xmlElement, "BulkTransfer",
                ref cfg.BulkTransfer, compulsory);
            if (Configuration.ConfigUint(xmlElement, "CheckpointDelay",
                ref uintValue, compulsory))
                cfg.CheckpointDelay = uintValue;
            if (Configuration.ConfigUint(xmlElement, "ConnectionRetry",
                ref uintValue, compulsory))
                cfg.ConnectionRetry = uintValue;
            Configuration.ConfigBool(xmlElement, "DelayClientSync",
                ref cfg.DelayClientSync, compulsory);
            if (Configuration.ConfigUint(xmlElement, "ElectionRetry",
                ref uintValue, compulsory))
                cfg.ElectionRetry = uintValue;
            if (Configuration.ConfigUint(xmlElement, "ElectionTimeout",
                ref uintValue, compulsory))
                cfg.ElectionTimeout = uintValue;
            if (Configuration.ConfigUint(xmlElement, "FullElectionTimeout",
                ref uintValue, compulsory))
                cfg.FullElectionTimeout = uintValue;
            if (Configuration.ConfigUint(xmlElement, "HeartbeatMonitor",
                ref uintValue, compulsory))
                cfg.HeartbeatMonitor = uintValue;
            if (Configuration.ConfigUint(xmlElement, "HeartbeatSend",
                ref uintValue, compulsory))
                cfg.HeartbeatSend = uintValue;
            if (Configuration.ConfigUint(xmlElement, "LeaseTimeout",
                ref uintValue, compulsory))
                cfg.LeaseTimeout = uintValue;
            Configuration.ConfigBool(xmlElement, "NoAutoInit",
                ref cfg.NoAutoInit, compulsory);
            Configuration.ConfigBool(xmlElement, "NoBlocking",
                ref cfg.NoBlocking, compulsory);
            if (Configuration.ConfigUint(xmlElement, "NSites",
                ref uintValue, compulsory))
                cfg.NSites = uintValue;
            if (Configuration.ConfigUint(xmlElement, "Priority",
                ref uintValue, compulsory))
                cfg.Priority = uintValue;
            Configuration.ConfigAckPolicy(xmlElement,
                "RepMgrAckPolicy", ref cfg.RepMgrAckPolicy, 
                compulsory);
            cfg.RepMgrLocalSite = new ReplicationHostAddress();
            Configuration.ConfigReplicationHostAddress(xmlElement,
                "RepMgrLocalSite", ref cfg.RepMgrLocalSite, compulsory);
            Configuration.ConfigBool(xmlElement, "Strict2Site",
                ref cfg.Strict2Site, compulsory);
            Configuration.ConfigBool(xmlElement, "UseMasterLeases",
                ref cfg.UseMasterLeases, compulsory);
        }

    }
}
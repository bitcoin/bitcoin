/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Xml;
using System.Xml.XPath;
using BerkeleyDB;
using NUnit.Framework;

namespace CsharpAPITest
{
    [TestFixture]
    public class DatabaseEnvironmentConfigTest
    {
        private string testFixtureHome;
        private string testFixtureName;
        private string testName;
        private string testHome;

        [TestFixtureSetUp]
        public void RunBeforeTests()
        {
            testFixtureName = "DatabaseEnvironmentConfigTest";
            testFixtureHome = "./TestOut/" + testFixtureName;

            Configuration.ClearDir(testFixtureHome);
        }

        [Test]
        public void TestConfig()
        {
            testName = "TestConfig";
            XmlElement xmlElem = Configuration.TestSetUp(
                testFixtureName, testName);
            DatabaseEnvironmentConfig envConfig =
                new DatabaseEnvironmentConfig();
            Config(xmlElem, ref envConfig, true);
            Confirm(xmlElem, envConfig, true);
        }
        
        public static void Confirm(XmlElement xmlElement,
            DatabaseEnvironmentConfig envConfig, bool compulsory)
        {
            Configuration.ConfirmBool(xmlElement, "AutoCommit",
                envConfig.AutoCommit, compulsory);
            Configuration.ConfirmBool(xmlElement, "CDB_ALLDB",
                envConfig.CDB_ALLDB, compulsory);
            Configuration.ConfirmBool(xmlElement, "Create",
                envConfig.Create, compulsory);
            Configuration.ConfirmStringList(xmlElement, "DataDirs",
                envConfig.DataDirs, compulsory);
            Configuration.ConfirmString(xmlElement, "ErrorPrefix",
                envConfig.ErrorPrefix, compulsory);
            Configuration.ConfirmBool(xmlElement, "ForceFlush",
                envConfig.ForceFlush, compulsory);
            Configuration.ConfirmBool(xmlElement, "FreeThreaded",
                envConfig.FreeThreaded, compulsory);
            Configuration.ConfirmBool(xmlElement, "InitRegions",
                envConfig.InitRegions, compulsory);
            Configuration.ConfirmString(xmlElement,
                "IntermediateDirMode",
                envConfig.IntermediateDirMode, compulsory);
            Configuration.ConfirmBool(xmlElement, "Lockdown",
                envConfig.Lockdown, compulsory);
            Configuration.ConfirmUint(xmlElement, "LockTimeout",
                envConfig.LockTimeout, compulsory);
            Configuration.ConfirmUint(xmlElement, "MaxTransactions",
                envConfig.MaxTransactions, compulsory);
            Configuration.ConfirmBool(xmlElement, "NoBuffer",
                envConfig.NoBuffer, compulsory);
            Configuration.ConfirmBool(xmlElement, "NoLocking",
                envConfig.NoLocking, compulsory);
            Configuration.ConfirmBool(xmlElement, "NoMMap",
                envConfig.NoMMap, compulsory);
            Configuration.ConfirmBool(xmlElement, "NoLocking",
                envConfig.NoLocking, compulsory);
            Configuration.ConfirmBool(xmlElement, "NoPanic",
                envConfig.NoPanic, compulsory);
            Configuration.ConfirmBool(xmlElement, "Overwrite",
                envConfig.Overwrite, compulsory);
            Configuration.ConfirmBool(xmlElement, "Private",
                envConfig.Private, compulsory);
            Configuration.ConfirmBool(xmlElement, "Register",
                envConfig.Register, compulsory);
            Configuration.ConfirmBool(xmlElement, "RunFatalRecovery", 
                envConfig.RunFatalRecovery, compulsory);
            Configuration.ConfirmBool(xmlElement, "RunRecovery",
                envConfig.RunRecovery, compulsory);
            Configuration.ConfirmBool(xmlElement, "SystemMemory",
                envConfig.SystemMemory, compulsory);
            Configuration.ConfirmString(xmlElement, "TempDir",
                envConfig.TempDir, compulsory);
            Configuration.ConfirmBool(xmlElement, "TimeNotGranted",
                envConfig.TimeNotGranted, compulsory);
            Configuration.ConfirmBool(xmlElement, "TxnNoSync",
                envConfig.TxnNoSync, compulsory);
            Configuration.ConfirmBool(xmlElement, "TxnNoWait",
                envConfig.TxnNoWait, compulsory);
            Configuration.ConfirmBool(xmlElement, "TxnSnapshot",
                envConfig.TxnSnapshot, compulsory);
            Configuration.ConfirmDateTime(xmlElement,"TxnTimestamp", 
                envConfig.TxnTimestamp, compulsory);
            Configuration.ConfirmBool(xmlElement, "TxnWriteNoSync",
                envConfig.TxnWriteNoSync, compulsory);
            Configuration.ConfirmBool(xmlElement, "UseCDB",
                envConfig.UseCDB, compulsory);
            Configuration.ConfirmBool(xmlElement, "UseLocking",
                envConfig.UseLocking, compulsory);
            Configuration.ConfirmBool(xmlElement, "UseLogging",
                envConfig.UseLogging, compulsory);
            Configuration.ConfirmBool(xmlElement, "UseMPool",
                envConfig.UseMPool, compulsory);
            Configuration.ConfirmBool(xmlElement, "UseMVCC",
                envConfig.UseMVCC, compulsory);
            Configuration.ConfirmBool(xmlElement, "UseReplication",
                envConfig.UseReplication, compulsory);
            Configuration.ConfirmBool(xmlElement, "UseTxns",
                envConfig.UseTxns, compulsory);
            envConfig.Verbosity = new VerboseMessages();
            Configuration.ConfirmVerboseMessages(xmlElement,
                "Verbosity", envConfig.Verbosity, compulsory);
            Configuration.ConfirmBool(xmlElement, "YieldCPU",
                envConfig.YieldCPU, compulsory);
        }

        public static void Config(XmlElement xmlElement,
            ref DatabaseEnvironmentConfig envConfig, bool compulsory)
        {
            uint value = new uint();
            DateTime time = new DateTime();

            Configuration.ConfigBool(xmlElement, "AutoCommit",
                ref envConfig.AutoCommit, compulsory);
            Configuration.ConfigBool(xmlElement, "CDB_ALLDB",
                ref envConfig.CDB_ALLDB, compulsory);
            Configuration.ConfigBool(xmlElement, "Create",
                ref envConfig.Create, compulsory);
            Configuration.ConfigString(xmlElement, "CreationDir",
                ref envConfig.CreationDir, compulsory);
            Configuration.ConfigStringList(xmlElement, "DataDirs",
                ref envConfig.DataDirs, compulsory);
            Configuration.ConfigString(xmlElement, "ErrorPrefix",
                ref envConfig.ErrorPrefix, compulsory);
            Configuration.ConfigBool(xmlElement, "ForceFlush",
                ref envConfig.ForceFlush, compulsory);
            Configuration.ConfigBool(xmlElement, "FreeThreaded",
                ref envConfig.FreeThreaded, compulsory);
            Configuration.ConfigBool(xmlElement, "InitRegions",
                ref envConfig.InitRegions, compulsory);
            Configuration.ConfigString(xmlElement, "IntermediateDirMode",
                ref envConfig.IntermediateDirMode, compulsory);
            Configuration.ConfigBool(xmlElement, "Lockdown",
                ref envConfig.Lockdown, compulsory);
            if (Configuration.ConfigUint(xmlElement, "LockTimeout",
                ref value, compulsory))
                envConfig.LockTimeout = value;
            if (Configuration.ConfigUint(xmlElement, "MaxTransactions",
                ref value, compulsory))
                envConfig.MaxTransactions = value;
            Configuration.ConfigBool(xmlElement, "NoBuffer",
                ref envConfig.NoBuffer, compulsory);
            Configuration.ConfigBool(xmlElement, "NoLocking",
                ref envConfig.NoLocking, compulsory);
            Configuration.ConfigBool(xmlElement, "NoMMap",
                ref envConfig.NoMMap, compulsory);
            Configuration.ConfigBool(xmlElement, "NoLocking",
                ref envConfig.NoLocking, compulsory);
            Configuration.ConfigBool(xmlElement, "NoPanic",
                ref envConfig.NoPanic, compulsory);
            Configuration.ConfigBool(xmlElement, "Overwrite",
                ref envConfig.Overwrite, compulsory);
            Configuration.ConfigBool(xmlElement, "Private",
                ref envConfig.Private, compulsory);
            Configuration.ConfigBool(xmlElement, "Register",
                ref envConfig.Register, compulsory);
            Configuration.ConfigBool(xmlElement, "RunFatalRecovery",
                ref envConfig.RunFatalRecovery, compulsory);
            Configuration.ConfigBool(xmlElement, "RunRecovery",
                ref envConfig.RunRecovery, compulsory);
            Configuration.ConfigBool(xmlElement, "SystemMemory",
                ref envConfig.SystemMemory, compulsory);
            Configuration.ConfigString(xmlElement, "TempDir",
                ref envConfig.TempDir, compulsory);
            Configuration.ConfigBool(xmlElement, "TimeNotGranted",
                ref envConfig.TimeNotGranted, compulsory);
            Configuration.ConfigBool(xmlElement, "TxnNoSync",
                ref envConfig.TxnNoSync, compulsory);
            Configuration.ConfigBool(xmlElement, "TxnNoWait",
                ref envConfig.TxnNoWait, compulsory);
            Configuration.ConfigBool(xmlElement, "TxnSnapshot",
                ref envConfig.TxnSnapshot, compulsory);
            if (Configuration.ConfigDateTime(xmlElement, "TxnTimestamp", 
                ref time, compulsory))
                envConfig.TxnTimestamp = time;
            Configuration.ConfigBool(xmlElement, "TxnWriteNoSync",
                ref envConfig.TxnWriteNoSync, compulsory);
            Configuration.ConfigBool(xmlElement, "UseLocking",
                ref envConfig.UseLocking, compulsory);
            Configuration.ConfigBool(xmlElement, "UseLogging",
                ref envConfig.UseLogging, compulsory);
            Configuration.ConfigBool(xmlElement, "UseMPool",
                ref envConfig.UseMPool, compulsory);
            Configuration.ConfigBool(xmlElement, "UseMVCC",
                ref envConfig.UseMVCC, compulsory);
            Configuration.ConfigBool(xmlElement, "UseReplication",
                ref envConfig.UseReplication, compulsory);
            Configuration.ConfigBool(xmlElement, "UseTxns",
                ref envConfig.UseTxns, compulsory);
            envConfig.Verbosity = new VerboseMessages();
            Configuration.ConfigVerboseMessages(xmlElement, 
                "Verbosity", ref envConfig.Verbosity, compulsory);
            Configuration.ConfigBool(xmlElement, "YieldCPU",
                ref envConfig.YieldCPU, compulsory);
        }

        [Test]
        public void TestConfigLock()
        {
            testName = "TestConfigLock";
            XmlElement xmlElem = Configuration.TestSetUp(
                testFixtureName, testName);
            DatabaseEnvironmentConfig cfg = 
                new DatabaseEnvironmentConfig();
            cfg.LockSystemCfg = new LockingConfig();
            LockingConfigTest.Config(xmlElem, 
                ref cfg.LockSystemCfg, true);
            LockingConfigTest.Confirm(xmlElem, 
                cfg.LockSystemCfg, true);
        }

        [Test]
        public void TestConfigLog()
        {
            testName = "TestConfigLog";
            XmlElement xmlElem = Configuration.TestSetUp(
                testFixtureName, testName);
            DatabaseEnvironmentConfig cfg =
                new DatabaseEnvironmentConfig();
            cfg.LogSystemCfg = new LogConfig();
            LogConfigTest.Config(xmlElem, ref cfg.LogSystemCfg, true);
            LogConfigTest.Confirm(xmlElem, cfg.LogSystemCfg, true);
        }

        [Test]
        public void TestConfigMutex()
        {
            testName = "TestConfigMutex";
            XmlElement xmlElem = Configuration.TestSetUp(
                testFixtureName, testName);
            DatabaseEnvironmentConfig cfg =
                new DatabaseEnvironmentConfig();
            cfg.MutexSystemCfg = new MutexConfig();
            MutexConfigTest.Config(xmlElem, ref cfg.MutexSystemCfg, true);
            MutexConfigTest.Confirm(xmlElem, cfg.MutexSystemCfg, true);
        }

        [Test]
        public void TestConfigReplication()
        {
            testName = "TestConfigReplication";
            XmlElement xmlElem = Configuration.TestSetUp(
                testFixtureName, testName);
            DatabaseEnvironmentConfig cfg =
                new DatabaseEnvironmentConfig();
            cfg.RepSystemCfg = new ReplicationConfig();
            ReplicationConfigTest.Config(xmlElem, 
                ref cfg.RepSystemCfg, true);
            ReplicationConfigTest.Confirm(xmlElem, 
                cfg.RepSystemCfg, true);
        }

        [Test]
        public void TestSetEncryption()
        {
            testName = "TestSetEncryption";
            testHome = testFixtureHome + "/" + testName;

            Configuration.ClearDir(testHome);
            DatabaseEnvironmentConfig envConfig =
                new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.SetEncryption("key", EncryptionAlgorithm.AES);
            Assert.AreEqual("key", envConfig.EncryptionPassword);
            Assert.AreEqual(EncryptionAlgorithm.AES, envConfig.EncryptAlgorithm);
            DatabaseEnvironment env = DatabaseEnvironment.Open(
                testHome, envConfig);
            Assert.AreEqual(EncryptionAlgorithm.AES, env.EncryptAlgorithm);
            env.Close();
        }

    }
}


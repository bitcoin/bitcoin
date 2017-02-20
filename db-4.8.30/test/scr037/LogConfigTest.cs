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
    public class LogConfigTest
    {
        private string testFixtureHome;
        private string testFixtureName;
        private string testName;
        private string testHome;

        [TestFixtureSetUp]
        public void RunBeforeTests()
        {
            testFixtureName = "LogConfigTest";
            testFixtureHome = "./TestOut/" + testFixtureName;
            Configuration.ClearDir(testFixtureHome);
        }

        [Test]
        public void TestConfig()
        {
            testName = "TestConfig";
            /* 
             * Configure the fields/properties and see if 
             * they are updated successfully.
             */
            LogConfig logConfig = new LogConfig();
            XmlElement xmlElem = Configuration.TestSetUp(testFixtureName, testName);
            Config(xmlElem, ref logConfig, true);
            Confirm(xmlElem, logConfig, true);
        }

        [Test, ExpectedException(typeof(ExpectedTestException))]
        public void TestFullLogBufferException()
        {
            testName = "TestFullLogBufferException";
            testHome = testFixtureHome + "/" + testName;

            Configuration.ClearDir(testHome);

            // Open an environment and configured log subsystem.
            DatabaseEnvironmentConfig cfg =
                new DatabaseEnvironmentConfig();
            cfg.Create = true;
            cfg.TxnNoSync = true;
            cfg.UseTxns = true;
            cfg.UseLocking = true;
            cfg.UseMPool = true;
            cfg.UseLogging = true;
            cfg.LogSystemCfg = new LogConfig();
            cfg.LogSystemCfg.AutoRemove = false;
            cfg.LogSystemCfg.BufferSize = 409600;
            cfg.LogSystemCfg.MaxFileSize = 10480;
            cfg.LogSystemCfg.NoBuffer = false;
            cfg.LogSystemCfg.ZeroOnCreate = true;
            cfg.LogSystemCfg.InMemory = true;
            DatabaseEnvironment env = DatabaseEnvironment.Open(testHome, cfg);

            BTreeDatabase db;
            try
            {
                Transaction openTxn = env.BeginTransaction();
                try
                {
                    BTreeDatabaseConfig dbConfig =
                        new BTreeDatabaseConfig();
                    dbConfig.Creation = CreatePolicy.IF_NEEDED;
                    dbConfig.Env = env;
                    db = BTreeDatabase.Open(testName + ".db", dbConfig, openTxn);
                    openTxn.Commit();
                }
                catch (DatabaseException e)
                {
                    openTxn.Abort();
                    throw e;
                }

                Transaction writeTxn = env.BeginTransaction();
                try
                {
                    /*
                     * Writing 10 large records into in-memory logging 
                     * database should throw FullLogBufferException since 
                     * the amount of put data is larger than buffer size.
                     */
                    byte[] byteArr = new byte[204800];
                    for (int i = 0; i < 10; i++)
                        db.Put(new DatabaseEntry(BitConverter.GetBytes(i)),
                            new DatabaseEntry(byteArr), writeTxn);
                    writeTxn.Commit();
                }
                catch (Exception e)
                {
                    writeTxn.Abort();
                    throw e;
                }
                finally
                {
                    db.Close(true);
                }
            }
            catch (FullLogBufferException e)
            {
                Assert.AreEqual(ErrorCodes.DB_LOG_BUFFER_FULL, e.ErrorCode);
                throw new ExpectedTestException();
            }
            finally
            {
                env.Close();
            }
        }

        [Test]
        public void TestLoggingSystemStats()
        {
            testName = "TestLoggingSystemStats";
            testHome = testFixtureHome + "/" + testName;
            string logDir = "./";

            Configuration.ClearDir(testHome);
            Directory.CreateDirectory(testHome + "/" + logDir);

            DatabaseEnvironmentConfig cfg =
                new DatabaseEnvironmentConfig();
            cfg.Create = true;
            cfg.UseTxns = true;
            cfg.AutoCommit = true;
            cfg.UseLocking = true;
            cfg.UseMPool = true;
            cfg.UseLogging = true;
            cfg.MPoolSystemCfg = new MPoolConfig();
            cfg.MPoolSystemCfg.CacheSize = new CacheInfo(0, 1048576, 1);

            cfg.LogSystemCfg = new LogConfig();
            cfg.LogSystemCfg.AutoRemove = false;
            cfg.LogSystemCfg.BufferSize = 10240;
            cfg.LogSystemCfg.Dir = logDir;
            cfg.LogSystemCfg.FileMode = 755;
            cfg.LogSystemCfg.ForceSync = true;
            cfg.LogSystemCfg.InMemory = false;
            cfg.LogSystemCfg.MaxFileSize = 1048576;
            cfg.LogSystemCfg.NoBuffer = false;
            cfg.LogSystemCfg.RegionSize = 204800;
            cfg.LogSystemCfg.ZeroOnCreate = true;

            DatabaseEnvironment env = DatabaseEnvironment.Open(testHome, cfg);

            LogStats stats = env.LoggingSystemStats();
            env.PrintLoggingSystemStats();
            Assert.AreEqual(10240, stats.BufferSize);
            Assert.AreEqual(1, stats.CurrentFile);
            Assert.AreNotEqual(0, stats.CurrentOffset);
            Assert.AreEqual(1048576, stats.FileSize);
            Assert.AreNotEqual(0, stats.MagicNumber);
            Assert.AreNotEqual(0, stats.PermissionsMode);
            Assert.AreEqual(1, stats.Records);
            Assert.AreNotEqual(0, stats.RegionLockNoWait);
            Assert.LessOrEqual(204800, stats.RegionSize);
            Assert.AreNotEqual(0, stats.Version);

            Transaction openTxn = env.BeginTransaction();
            BTreeDatabaseConfig dbConfig = new BTreeDatabaseConfig();
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            dbConfig.Env = env;
            BTreeDatabase db = BTreeDatabase.Open(testName + ".db", dbConfig, openTxn);
            openTxn.Commit();

            Transaction writeTxn = env.BeginTransaction();
            byte[] byteArr = new byte[1024];
            for (int i = 0; i < 1000; i++)
                db.Put(new DatabaseEntry(BitConverter.GetBytes(i)),
                    new DatabaseEntry(byteArr), writeTxn);
            writeTxn.Commit();

            stats = env.LoggingSystemStats();
            Assert.AreNotEqual(0, stats.Bytes);
            Assert.AreNotEqual(0, stats.BytesSinceCheckpoint);
            Assert.AreNotEqual(0, stats.DiskFileNumber);
            Assert.AreNotEqual(0, stats.DiskOffset);
            Assert.AreNotEqual(0, stats.MaxCommitsPerFlush);
            Assert.AreNotEqual(0, stats.MBytes);
            Assert.AreNotEqual(0, stats.MBytesSinceCheckpoint);
            Assert.AreNotEqual(0, stats.MinCommitsPerFlush);
            Assert.AreNotEqual(0, stats.OverflowWrites);
            Assert.AreNotEqual(0, stats.Syncs);
            Assert.AreNotEqual(0, stats.Writes);
            Assert.AreEqual(0, stats.Reads);
            Assert.AreEqual(0, stats.RegionLockWait);

            stats = env.LoggingSystemStats(true);
            stats = env.LoggingSystemStats();
            Assert.AreEqual(0, stats.Bytes);
            Assert.AreEqual(0, stats.BytesSinceCheckpoint);
            Assert.AreEqual(0, stats.MaxCommitsPerFlush);
            Assert.AreEqual(0, stats.MBytes);
            Assert.AreEqual(0, stats.MBytesSinceCheckpoint);
            Assert.AreEqual(0, stats.MinCommitsPerFlush);
            Assert.AreEqual(0, stats.OverflowWrites);
            Assert.AreEqual(0, stats.Syncs);
            Assert.AreEqual(0, stats.Writes);
            Assert.AreEqual(0, stats.Reads);

            env.PrintLoggingSystemStats(true, true);

            db.Close();
            env.Close();
        }

        [Test]
        public void TestLsn()
        {
            testName = "TestLsn";
            testHome = testFixtureHome + "/" + testName;
            Configuration.ClearDir(testHome);

            LSN lsn = new LSN(12, 411);
            Assert.AreEqual(12, lsn.LogFileNumber);
            Assert.AreEqual(411, lsn.Offset);

            LSN newLsn = new LSN(15, 410);
            Assert.AreEqual(0, LSN.Compare(lsn, lsn));
            Assert.Greater(0, LSN.Compare(lsn, newLsn));
        }
        
        public static void Confirm(XmlElement
            xmlElement, LogConfig logConfig, bool compulsory)
        {
            Configuration.ConfirmBool(xmlElement, "AutoRemove",
                logConfig.AutoRemove, compulsory);
            Configuration.ConfirmUint(xmlElement, "BufferSize",
                logConfig.BufferSize, compulsory);
            Configuration.ConfirmString(xmlElement, "Dir",
                logConfig.Dir, compulsory);
            Configuration.ConfirmInt(xmlElement, "FileMode",
                logConfig.FileMode, compulsory);
            Configuration.ConfirmBool(xmlElement, "ForceSync",
                logConfig.ForceSync, compulsory);
            Configuration.ConfirmBool(xmlElement, "InMemory",
                logConfig.InMemory, compulsory);
            Configuration.ConfirmUint(xmlElement, "MaxFileSize",
                logConfig.MaxFileSize, compulsory);
            Configuration.ConfirmBool(xmlElement, "NoBuffer",
                logConfig.NoBuffer, compulsory);
            Configuration.ConfirmUint(xmlElement, "RegionSize",
                logConfig.RegionSize, compulsory);
            Configuration.ConfirmBool(xmlElement, "ZeroOnCreate",
                logConfig.ZeroOnCreate, compulsory);
        }

        public static void Config(XmlElement
            xmlElement, ref LogConfig logConfig, bool compulsory)
        {
            uint uintValue = new uint();
            int intValue = new int();

            Configuration.ConfigBool(xmlElement, "AutoRemove",
                ref logConfig.AutoRemove, compulsory);
            if (Configuration.ConfigUint(xmlElement, "BufferSize",
                ref uintValue, compulsory))
                logConfig.BufferSize = uintValue;
            Configuration.ConfigString(xmlElement, "Dir",
                ref logConfig.Dir, compulsory);
            if (Configuration.ConfigInt(xmlElement, "FileMode",
                ref intValue, compulsory))
                logConfig.FileMode = intValue;
            Configuration.ConfigBool(xmlElement, "ForceSync",
                ref logConfig.ForceSync, compulsory);
            Configuration.ConfigBool(xmlElement, "InMemory",
                ref logConfig.InMemory, compulsory);
            if (Configuration.ConfigUint(xmlElement, "MaxFileSize",
                ref uintValue, compulsory))
                logConfig.MaxFileSize = uintValue;
            Configuration.ConfigBool(xmlElement, "NoBuffer",
                ref logConfig.NoBuffer, compulsory);
            if (Configuration.ConfigUint(xmlElement, "RegionSize",
                ref uintValue, compulsory))
                logConfig.RegionSize = uintValue;
            Configuration.ConfigBool(xmlElement, "ZeroOnCreate",
                ref logConfig.ZeroOnCreate, compulsory);
        }
    }
}

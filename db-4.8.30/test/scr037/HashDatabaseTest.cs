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
using NUnit.Framework;
using BerkeleyDB;

namespace CsharpAPITest
{
    [TestFixture]
    public class HashDatabaseTest
    {
        private string testFixtureHome;
        private string testFixtureName;
        private string testName;
        private string testHome;

        [TestFixtureSetUp]
        public void RunBeforeTests()
        {
            testFixtureName = "HashDatabaseTest";
            testFixtureHome = "./TestOut/" + testFixtureName;

            Configuration.ClearDir(testFixtureHome);
        }

        [Test]
        public void TestHashComparison()
        {
            testName = "TestHashComparison";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";
            Configuration.ClearDir(testHome);

            HashDatabaseConfig dbConfig = new HashDatabaseConfig();
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            dbConfig.HashComparison = new EntryComparisonDelegate(EntryComparison);
            HashDatabase db = HashDatabase.Open(dbFileName,dbConfig);
            int ret;

            /*
             * Comparison gets the value that lowest byte of the 
             * former dbt minus that of the latter one.
             */
            ret = db.Compare(new DatabaseEntry(BitConverter.GetBytes(2)),
                new DatabaseEntry(BitConverter.GetBytes(2)));
            Assert.AreEqual(0, ret);

            ret = db.Compare(new DatabaseEntry(BitConverter.GetBytes(256)),
                new DatabaseEntry(BitConverter.GetBytes(1)));
            Assert.Greater(0, ret);

            db.Close();
        }

        public int EntryComparison(DatabaseEntry dbt1,
             DatabaseEntry dbt2)
        {
            return dbt1.Data[0] - dbt2.Data[0];
        }

        [Test]
        public void TestHashFunction()
        {
            testName = "TestHashFunction";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";
            Configuration.ClearDir(testHome);

            HashDatabaseConfig dbConfig = new HashDatabaseConfig();
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            dbConfig.HashFunction = new HashFunctionDelegate(HashFunction);
            HashDatabase db = HashDatabase.Open(dbFileName, dbConfig);

            // Hash function will change the lowest byte to 0;
            uint data = db.HashFunction(BitConverter.GetBytes(1));
            Assert.AreEqual(0, data);
            db.Close();
        }

        public uint HashFunction(byte[] data)
        {
            data[0] = 0;
            return BitConverter.ToUInt32(data, 0);
        }

        [Test]
        public void TestOpenExistingHashDB()
        {
            testName = "TestOpenExistingHashDB";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";

            Configuration.ClearDir(testHome);

            HashDatabaseConfig hashConfig =
                new HashDatabaseConfig();
            hashConfig.Creation = CreatePolicy.ALWAYS;
            HashDatabase hashDB = HashDatabase.Open(dbFileName, hashConfig);
            hashDB.Close();

            DatabaseConfig dbConfig = new DatabaseConfig();
            Database db = Database.Open(dbFileName, dbConfig);
            Assert.AreEqual(db.Type, DatabaseType.HASH);
            db.Close();
        }

        [Test]
        public void TestOpenNewHashDB()
        {
            testName = "TestOpenNewHashDB";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";

            Configuration.ClearDir(testHome);

            XmlElement xmlElem = Configuration.TestSetUp(testFixtureName, testName);
            HashDatabaseConfig hashConfig = new HashDatabaseConfig();
            HashDatabaseConfigTest.Config(xmlElem, ref hashConfig, true);
            HashDatabase hashDB = HashDatabase.Open(dbFileName, hashConfig);
            Confirm(xmlElem, hashDB, true);
            hashDB.Close();
        }

        [Test, ExpectedException(typeof(ExpectedTestException))]
        public void TestPutNoDuplicateWithUnsortedDuplicate()
        {
            testName = "TestPutNoDuplicateWithUnsortedDuplicate";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";

            Configuration.ClearDir(testHome);

            HashDatabaseConfig hashConfig = new HashDatabaseConfig();
            hashConfig.Creation = CreatePolicy.ALWAYS;
            hashConfig.Duplicates = DuplicatesPolicy.UNSORTED;
            hashConfig.ErrorPrefix = testName;

            HashDatabase hashDB = HashDatabase.Open(dbFileName, hashConfig);
            DatabaseEntry key, data;
            key = new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("1"));
            data = new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("1"));

            try
            {
                hashDB.PutNoDuplicate(key, data);
            }
            catch (DatabaseException)
            {
                throw new ExpectedTestException();
            }
            finally
            {
                hashDB.Close();
            }
        }


        [Test, ExpectedException(typeof(ExpectedTestException))]
        public void TestKeyExistException()
        {
            testName = "TestKeyExistException";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";

            Configuration.ClearDir(testHome);

            HashDatabaseConfig hashConfig = new HashDatabaseConfig();
            hashConfig.Creation = CreatePolicy.ALWAYS;
            hashConfig.Duplicates = DuplicatesPolicy.SORTED;
            HashDatabase hashDB = HashDatabase.Open(dbFileName, hashConfig);

            // Put the same record into db twice.
            DatabaseEntry key, data;
            key = new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("1"));
            data = new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("1"));
            try
            {
                hashDB.PutNoDuplicate(key, data);
                hashDB.PutNoDuplicate(key, data);
            }
            catch (KeyExistException)
            {
                throw new ExpectedTestException();
            }
            finally
            {
                hashDB.Close();
            }
        }

        [Test]
        public void TestPutNoDuplicate()
        {
            testName = "TestPutNoDuplicate";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";

            Configuration.ClearDir(testHome);

            HashDatabaseConfig hashConfig =
                new HashDatabaseConfig();
            hashConfig.Creation = CreatePolicy.ALWAYS;
            hashConfig.Duplicates = DuplicatesPolicy.SORTED;
            hashConfig.TableSize = 20;
            HashDatabase hashDB = HashDatabase.Open(dbFileName, hashConfig);

            DatabaseEntry key, data;
            for (int i = 1; i <= 10; i++)
            {
                key = new DatabaseEntry(BitConverter.GetBytes(i));
                data = new DatabaseEntry(BitConverter.GetBytes(i));
                hashDB.PutNoDuplicate(key, data);
            }

            Assert.IsTrue(hashDB.Exists(
                new DatabaseEntry(BitConverter.GetBytes((int)5))));

            hashDB.Close();
        }

        [Test, ExpectedException(typeof(ExpectedTestException))]
        public void TestPutNoDuplicateWithTxn()
        {
            testName = "TestPutNoDuplicateWithTxn";
            testHome = testFixtureHome + "/" + testName;

            Configuration.ClearDir(testHome);

            // Open an environment.
            DatabaseEnvironmentConfig envConfig = 
                new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.UseLogging = true;
            envConfig.UseMPool = true;
            envConfig.UseTxns = true;
            DatabaseEnvironment env = DatabaseEnvironment.Open(
                testHome, envConfig);

            // Open a hash database within a transaction.
            Transaction txn = env.BeginTransaction();
            HashDatabaseConfig dbConfig = new HashDatabaseConfig();
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            dbConfig.Duplicates = DuplicatesPolicy.SORTED;
            dbConfig.Env = env;
            HashDatabase db = HashDatabase.Open(testName + ".db", dbConfig, txn);

            DatabaseEntry dbt = new DatabaseEntry(BitConverter.GetBytes((int)100));
            db.PutNoDuplicate(dbt, dbt, txn);
            try
            {
                db.PutNoDuplicate(dbt, dbt, txn);
            }
            catch (KeyExistException)
            {
                throw new ExpectedTestException();
            }
            finally
            {
                // Close all.
                db.Close();
                txn.Commit();
                env.Close();
            }
        }

        [Test]
        public void TestStats()
        {
            testName = "TestStats";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";
            Configuration.ClearDir(testHome);

            HashDatabaseConfig dbConfig = new HashDatabaseConfig();
            ConfigCase1(dbConfig);
            HashDatabase db = HashDatabase.Open(dbFileName, dbConfig);

            HashStats stats = db.Stats();
            HashStats fastStats = db.FastStats();
            ConfirmStatsPart1Case1(stats);
            ConfirmStatsPart1Case1(fastStats);

            // Put 100 records into the database.
            PutRecordCase1(db, null);

            stats = db.Stats();
            ConfirmStatsPart2Case1(stats);

            // Delete some data to get some free pages.
            byte[] bigArray = new byte[262144];
            db.Delete(new DatabaseEntry(bigArray));
            stats = db.Stats();
            ConfirmStatsPart3Case1(stats);

            db.Close();
        }

        [Test]
        public void TestStatsInTxn()
        {
            testName = "TestStatsInTxn";
            testHome = testFixtureHome + "/" + testName;
            Configuration.ClearDir(testHome);

            StatsInTxn(testHome, testName, false);
        }

        [Test]
        public void TestStatsWithIsolation()
        {
            testName = "TestStatsWithIsolation";
            testHome = testFixtureHome + "/" + testName;
            Configuration.ClearDir(testHome);

            StatsInTxn(testHome, testName, true);
        }

        public void StatsInTxn(string home, string name, bool ifIsolation)
        {
            DatabaseEnvironmentConfig envConfig =
                new DatabaseEnvironmentConfig();
            EnvConfigCase1(envConfig);
            DatabaseEnvironment env = DatabaseEnvironment.Open(home, envConfig);

            Transaction openTxn = env.BeginTransaction();
            HashDatabaseConfig dbConfig = new HashDatabaseConfig();
            ConfigCase1(dbConfig);
            dbConfig.Env = env;
            HashDatabase db = HashDatabase.Open(name + ".db", dbConfig, openTxn);
            openTxn.Commit();

            Transaction statsTxn = env.BeginTransaction();
            HashStats stats;
            HashStats fastStats;
            if (ifIsolation == false)
            {
                stats = db.Stats(statsTxn);
                fastStats = db.Stats(statsTxn);
            }
            else
            {
                stats = db.Stats(statsTxn, Isolation.DEGREE_ONE);
                fastStats = db.Stats(statsTxn, Isolation.DEGREE_ONE);
            }
            
            ConfirmStatsPart1Case1(stats);

            // Put 100 records into the database.
            PutRecordCase1(db, statsTxn);

            if (ifIsolation == false)
                stats = db.Stats(statsTxn);
            else
                stats = db.Stats(statsTxn, Isolation.DEGREE_TWO);
            ConfirmStatsPart2Case1(stats);

            // Delete some data to get some free pages.
            byte[] bigArray = new byte[262144];
            db.Delete(new DatabaseEntry(bigArray), statsTxn);
            if (ifIsolation == false)
                stats = db.Stats(statsTxn);
            else
                stats = db.Stats(statsTxn, Isolation.DEGREE_THREE);
            ConfirmStatsPart3Case1(stats);

            statsTxn.Commit();
            db.Close();
            env.Close();
        }

        public void EnvConfigCase1(DatabaseEnvironmentConfig cfg)
        {
            cfg.Create = true;
            cfg.UseTxns = true;
            cfg.UseMPool = true;
            cfg.UseLogging = true;
        }

        public void ConfigCase1(HashDatabaseConfig dbConfig)
        {
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            dbConfig.Duplicates = DuplicatesPolicy.UNSORTED;
            dbConfig.FillFactor = 10;
            dbConfig.TableSize = 20;
            dbConfig.PageSize = 4096;
        }

        public void PutRecordCase1(HashDatabase db, Transaction txn)
        {
            byte[] bigArray = new byte[262144];
            for (int i = 0; i < 50; i++)
            {
                if (txn == null)
                    db.Put(new DatabaseEntry(BitConverter.GetBytes(i)),
                        new DatabaseEntry(BitConverter.GetBytes(i)));
                else
                    db.Put(new DatabaseEntry(BitConverter.GetBytes(i)),
                        new DatabaseEntry(BitConverter.GetBytes(i)), txn);
            }
            for (int i = 50; i < 100; i++)
            {
                if (txn == null)
                    db.Put(new DatabaseEntry(bigArray),
                        new DatabaseEntry(bigArray));
                else
                    db.Put(new DatabaseEntry(bigArray),
                        new DatabaseEntry(bigArray), txn);
            }
        }

        public void ConfirmStatsPart1Case1(HashStats stats)
        {
            Assert.AreEqual(10, stats.FillFactor);
            Assert.AreEqual(4096, stats.PageSize);
            Assert.AreNotEqual(0, stats.Version);
        }

        public void ConfirmStatsPart2Case1(HashStats stats)
        {
            Assert.AreNotEqual(0, stats.BigPages);
            Assert.AreNotEqual(0, stats.BigPagesFreeBytes);
            Assert.AreNotEqual(0, stats.BucketPagesFreeBytes);
            Assert.AreNotEqual(0, stats.DuplicatePages);
            Assert.AreNotEqual(0, stats.DuplicatePagesFreeBytes);
            Assert.AreNotEqual(0, stats.MagicNumber);
            Assert.AreNotEqual(0, stats.MetadataFlags);
            Assert.AreEqual(100, stats.nData);
            Assert.AreNotEqual(0, stats.nHashBuckets);
            Assert.AreEqual(51, stats.nKeys);
            Assert.AreNotEqual(0, stats.nPages);
            Assert.AreEqual(0, stats.OverflowPages);
            Assert.AreEqual(0, stats.OverflowPagesFreeBytes);
        }

        public void ConfirmStatsPart3Case1(HashStats stats)
        {
            Assert.AreNotEqual(0, stats.FreePages);
        }

        public static void Confirm(XmlElement xmlElem,
            HashDatabase hashDB, bool compulsory)
        {
            DatabaseTest.Confirm(xmlElem, hashDB, compulsory);
            Configuration.ConfirmCreatePolicy(xmlElem,
                "Creation", hashDB.Creation, compulsory);
            Configuration.ConfirmDuplicatesPolicy(xmlElem,
                "Duplicates", hashDB.Duplicates, compulsory);
            Configuration.ConfirmUint(xmlElem,
                "FillFactor", hashDB.FillFactor, compulsory);
            Configuration.ConfirmUint(xmlElem,
                "NumElements", hashDB.TableSize * hashDB.FillFactor,
                compulsory);
            Assert.AreEqual(DatabaseType.HASH, hashDB.Type);
            string type = hashDB.Type.ToString();
            Assert.IsNotNull(type);
        }
    }
}

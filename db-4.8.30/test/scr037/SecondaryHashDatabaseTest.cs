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
using System.Threading;
using System.Xml;
using NUnit.Framework;
using BerkeleyDB;

namespace CsharpAPITest
{
    [TestFixture]
    public class SecondaryHashDatabaseTest
    {
        private string testFixtureHome;
        private string testFixtureName;
        private string testName;
        private string testHome;

        [TestFixtureSetUp]
        public void RunBeforeTests()
        {
            testFixtureName = "SecondaryHashDatabaseTest";
            testFixtureHome = "./TestOut/" + testFixtureName;

            Configuration.ClearDir(testFixtureHome);
        }

        [Test]
        public void TestHashFunction()
        {
            testName = "TestHashFunction";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";
            string dbSecFileName = testHome + "/" +
                testName + "_sec.db";

            Configuration.ClearDir(testHome);

            // Open a primary hash database.
            HashDatabaseConfig dbConfig =
                new HashDatabaseConfig();
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            HashDatabase hashDB = HashDatabase.Open(
                dbFileName, dbConfig);

            /*
             * Define hash function and open a secondary 
             * hash database.
             */
            SecondaryHashDatabaseConfig secDBConfig =
                new SecondaryHashDatabaseConfig(hashDB, null);
            secDBConfig.HashFunction = 
                new HashFunctionDelegate(HashFunction);
            secDBConfig.Creation = CreatePolicy.IF_NEEDED;
            SecondaryHashDatabase secDB =
               SecondaryHashDatabase.Open(dbSecFileName,
               secDBConfig);

            /*
             * Confirm the hash function defined in the configuration.
             * Call the hash function and the one from secondary 
             * database. If they return the same value, then the hash 
             * function is configured successfully.
             */
            uint data = secDB.HashFunction(BitConverter.GetBytes(1));
            Assert.AreEqual(0, data);

            // Close all.
            secDB.Close();
            hashDB.Close();
        }

        public uint HashFunction(byte[] data)
        {
            data[0] = 0;
            return BitConverter.ToUInt32(data, 0);
        }

        [Test]
        public void TestCompare()
        {
            testName = "TestCompare";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";
            string dbSecFileName = testHome + "/" + testName +
                "_sec.db";

            Configuration.ClearDir(testHome);

            // Open a primary hash database.
            HashDatabaseConfig dbConfig =
                new HashDatabaseConfig();
            dbConfig.Creation = CreatePolicy.ALWAYS;
            HashDatabase db = HashDatabase.Open(
                dbFileName, dbConfig);

            // Open a secondary hash database. 
            SecondaryHashDatabaseConfig secConfig =
                new SecondaryHashDatabaseConfig(null, null);
            secConfig.Creation = CreatePolicy.IF_NEEDED;
            secConfig.Primary = db;
            secConfig.Compare =
                new EntryComparisonDelegate(SecondaryEntryComparison);
            SecondaryHashDatabase secDB =
                SecondaryHashDatabase.Open(dbSecFileName, secConfig);

            /*
             * Get the compare function set in the configuration 
             * and run it in a comparison to see if it is alright.
             */
            DatabaseEntry dbt1, dbt2;
            dbt1 = new DatabaseEntry(
                BitConverter.GetBytes((int)257));
            dbt2 = new DatabaseEntry(
                BitConverter.GetBytes((int)255));
            Assert.Less(0, secDB.Compare(dbt1, dbt2));

            secDB.Close();
            db.Close();
        }

        [Test]
        public void TestDuplicates()
        {
            testName = "TestDuplicates";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";
            string dbSecFileName = testHome + "/" + testName
                + "_sec.db";

            Configuration.ClearDir(testHome);

            // Open a primary hash database.
            HashDatabaseConfig dbConfig =
                new HashDatabaseConfig();
            dbConfig.Creation = CreatePolicy.ALWAYS;
            HashDatabase db = HashDatabase.Open(
                dbFileName, dbConfig);

            // Open a secondary hash database. 
            SecondaryHashDatabaseConfig secConfig =
                new SecondaryHashDatabaseConfig(null, null);
            secConfig.Primary = db;
            secConfig.Duplicates = DuplicatesPolicy.SORTED;
            secConfig.Creation = CreatePolicy.IF_NEEDED;
            SecondaryHashDatabase secDB =
                SecondaryHashDatabase.Open(
                dbSecFileName, secConfig);

            // Confirm the duplicate in opened secondary database.
            Assert.AreEqual(DuplicatesPolicy.SORTED,
                secDB.Duplicates);

            secDB.Close();
            db.Close();
        }


        /*
         * Tests to all Open() share the same configuration in 
         * AllTestData.xml.
         */
        [Test]
        public void TestOpen()
        {
            testName = "TestOpen";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";
            string dbSecFileName = testHome + "/" +
                testName + "_sec.db";

            Configuration.ClearDir(testHome);

            OpenSecHashDB(testFixtureName, "TestOpen",
                dbFileName, dbSecFileName, false);
        }

        [Test]
        public void TestOpenWithDBName()
        {
            testName = "TestOpenWithDBName";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";
            string dbSecFileName = testHome + "/" +
                testName + "_sec.db";

            Configuration.ClearDir(testHome);

            OpenSecHashDB(testFixtureName, "TestOpen",
                dbFileName, dbSecFileName, true);
        }

        public void OpenSecHashDB(string className,
            string funName, string dbFileName, string dbSecFileName,
            bool ifDBName)
        {
            XmlElement xmlElem = Configuration.TestSetUp(
                className, funName);

            // Open a primary recno database.
            HashDatabaseConfig primaryDBConfig =
                new HashDatabaseConfig();
            primaryDBConfig.Creation = CreatePolicy.IF_NEEDED;
            HashDatabase primaryDB;

            /*
             * If secondary database name is given, the primary 
             * database is also opened with database name.
             */
            if (ifDBName == false)
                primaryDB = HashDatabase.Open(dbFileName,
                    primaryDBConfig);
            else
                primaryDB = HashDatabase.Open(dbFileName,
                    "primary", primaryDBConfig);

            try
            {
                // Open a new secondary database.
                SecondaryHashDatabaseConfig secHashDBConfig =
                    new SecondaryHashDatabaseConfig(
                    primaryDB, null);
                SecondaryHashDatabaseConfigTest.Config(
                    xmlElem, ref secHashDBConfig, false);
                secHashDBConfig.Creation =
                    CreatePolicy.IF_NEEDED;

                SecondaryHashDatabase secHashDB;
                if (ifDBName == false)
                    secHashDB = SecondaryHashDatabase.Open(
                        dbSecFileName, secHashDBConfig);
                else
                    secHashDB = SecondaryHashDatabase.Open(
                        dbSecFileName, "secondary",
                        secHashDBConfig);

                // Close the secondary database.
                secHashDB.Close();

                // Open the existing secondary database.
                SecondaryDatabaseConfig secDBConfig =
                    new SecondaryDatabaseConfig(
                    primaryDB, null);

                SecondaryDatabase secDB;
                if (ifDBName == false)
                    secDB = SecondaryHashDatabase.Open(
                        dbSecFileName, secDBConfig);
                else
                    secDB = SecondaryHashDatabase.Open(
                        dbSecFileName, "secondary", secDBConfig);

                // Close secondary database.
                secDB.Close();
            }
            catch (DatabaseException)
            {
                throw new TestException();
            }
            finally
            {
                // Close primary database.
                primaryDB.Close();
            }
        }

        [Test]
        public void TestOpenWithinTxn()
        {
            testName = "TestOpenWithinTxn";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testName + ".db";
            string dbSecFileName = testName + "_sec.db";

            Configuration.ClearDir(testHome);

            OpenSecHashDBWithinTxn(testFixtureName,
                "TestOpen", testHome, dbFileName,
                dbSecFileName, false);
        }

        [Test]
        public void TestOpenDBNameWithinTxn()
        {
            testName = "TestOpenDBNameWithinTxn";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testName + ".db";
            string dbSecFileName = testName + "_sec.db";

            Configuration.ClearDir(testHome);

            OpenSecHashDBWithinTxn(testFixtureName,
                "TestOpen", testHome, dbFileName,
                dbSecFileName, true);
        }

        public void OpenSecHashDBWithinTxn(string className,
            string funName, string home, string dbFileName,
            string dbSecFileName, bool ifDbName)
        {
            XmlElement xmlElem = Configuration.TestSetUp(
                className, funName);

            // Open an environment.
            DatabaseEnvironmentConfig envConfig =
                new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.UseTxns = true;
            envConfig.UseMPool = true;
            envConfig.UseLogging = true;
            DatabaseEnvironment env = DatabaseEnvironment.Open(
                home, envConfig);

            // Open a primary hash database.
            Transaction openDBTxn = env.BeginTransaction();
            HashDatabaseConfig dbConfig =
                new HashDatabaseConfig();
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            dbConfig.Env = env;
            HashDatabase db = HashDatabase.Open(
                dbFileName, dbConfig, openDBTxn);
            openDBTxn.Commit();

            // Open a secondary hash database.
            Transaction openSecTxn = env.BeginTransaction();
            SecondaryHashDatabaseConfig secDBConfig =
                new SecondaryHashDatabaseConfig(db,
                new SecondaryKeyGenDelegate(SecondaryKeyGen));
            SecondaryHashDatabaseConfigTest.Config(xmlElem,
                ref secDBConfig, false);
            secDBConfig.HashFunction = null;
            secDBConfig.Env = env;
            SecondaryHashDatabase secDB;
            if (ifDbName == false)
                secDB = SecondaryHashDatabase.Open(
                    dbSecFileName, secDBConfig, openSecTxn);
            else
                secDB = SecondaryHashDatabase.Open(
                    dbSecFileName, "secondary", secDBConfig,
                    openSecTxn);
            openSecTxn.Commit();

            // Confirm its flags configured in secDBConfig.
            Confirm(xmlElem, secDB, true);
            secDB.Close();

            // Open the existing secondary database.
            Transaction secTxn = env.BeginTransaction();
            SecondaryDatabaseConfig secConfig =
                new SecondaryDatabaseConfig(db,
                new SecondaryKeyGenDelegate(SecondaryKeyGen));
            secConfig.Env = env;

            SecondaryDatabase secExDB;
            if (ifDbName == false)
                secExDB = SecondaryHashDatabase.Open(
                    dbSecFileName, secConfig, secTxn);
            else
                secExDB = SecondaryHashDatabase.Open(
                    dbSecFileName, "secondary", secConfig,
                    secTxn);
            secExDB.Close();
            secTxn.Commit();

            db.Close();
            env.Close();
        }

        public DatabaseEntry SecondaryKeyGen(
            DatabaseEntry key, DatabaseEntry data)
        {
            DatabaseEntry dbtGen;
            dbtGen = new DatabaseEntry(data.Data);
            return dbtGen;
        }

        public int SecondaryEntryComparison(
            DatabaseEntry dbt1, DatabaseEntry dbt2)
        {
            int a, b;
            a = BitConverter.ToInt32(dbt1.Data, 0);
            b = BitConverter.ToInt32(dbt2.Data, 0);
            return a - b;
        }

        public static void Confirm(XmlElement xmlElem,
            SecondaryHashDatabase secDB, bool compulsory)
        {
            Configuration.ConfirmUint(xmlElem, "FillFactor",
                secDB.FillFactor, compulsory);
            Configuration.ConfirmUint(xmlElem, "NumElements",
                secDB.TableSize * secDB.FillFactor, compulsory);
        }
    }
}

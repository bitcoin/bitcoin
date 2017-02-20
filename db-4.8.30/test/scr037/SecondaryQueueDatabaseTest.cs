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
    public class SecondaryQueueDatabaseTest
    {
        private string testFixtureHome;
        private string testFixtureName;
        private string testName;
        private string testHome;

        [TestFixtureSetUp]
        public void RunBeforeTests()
        {
            testFixtureName = "SecondaryQueueDatabaseTest";
            testFixtureHome = "./TestOut/" + testFixtureName;

            Configuration.ClearDir(testFixtureHome);
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

            OpenSecQueueDB(testFixtureName, "TestOpen", 
                dbFileName, dbSecFileName);
        }
        
        public void OpenSecQueueDB(string className, 
            string funName, string dbFileName, string dbSecFileName)
        {
            XmlElement xmlElem = Configuration.TestSetUp(
                className, funName);

            // Open a primary queue database.
            QueueDatabaseConfig primaryDBConfig =
                new QueueDatabaseConfig();
            primaryDBConfig.Creation = CreatePolicy.IF_NEEDED;
            QueueDatabase primaryDB;

            /*
             * If secondary database name is given, the primary 
             * database is also opened with database name.
             */ 
            primaryDB = QueueDatabase.Open(dbFileName,
                primaryDBConfig);

            try
            {
                // Open a new secondary database.
                SecondaryQueueDatabaseConfig secQueueDBConfig =
                    new SecondaryQueueDatabaseConfig(
                    primaryDB, null);
                SecondaryQueueDatabaseConfigTest.Config(
                    xmlElem, ref secQueueDBConfig, false);
                secQueueDBConfig.Creation =
                    CreatePolicy.IF_NEEDED;
    
                SecondaryQueueDatabase secQueueDB;
                secQueueDB = SecondaryQueueDatabase.Open(
                    dbSecFileName, secQueueDBConfig);

                // Close the secondary database.
                secQueueDB.Close();

                // Open the existing secondary database.
                SecondaryDatabaseConfig secDBConfig =
                    new SecondaryQueueDatabaseConfig(
                    primaryDB, null);

                SecondaryDatabase secDB;
                secDB = SecondaryQueueDatabase.Open(
                    dbSecFileName, secDBConfig);

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

            OpenSecQueueDBWithinTxn(testFixtureName,
                "TestOpen", testHome, dbFileName, 
                dbSecFileName);
        }

        [Test]
        public void TestOpenDBNameWithinTxn()
        {
            testName = "TestOpenDBNameWithinTxn";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testName + ".db";
            string dbSecFileName = testName + "_sec.db";

            Configuration.ClearDir(testHome);

            OpenSecQueueDBWithinTxn(testFixtureName, 
                "TestOpen", testHome, dbFileName, 
                dbSecFileName);
        }

        public void OpenSecQueueDBWithinTxn(string className, 
            string funName, string home, string dbFileName, 
            string dbSecFileName)
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

            // Open a primary queue database.
            Transaction openDBTxn = env.BeginTransaction();
            QueueDatabaseConfig dbConfig =
                new QueueDatabaseConfig();
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            dbConfig.Env = env;
            QueueDatabase db = QueueDatabase.Open(
                dbFileName, dbConfig, openDBTxn);
            openDBTxn.Commit();

            // Open a secondary queue database.
            Transaction openSecTxn = env.BeginTransaction();
            SecondaryQueueDatabaseConfig secDBConfig =
                new SecondaryQueueDatabaseConfig(db,
                new SecondaryKeyGenDelegate(SecondaryKeyGen));
            SecondaryQueueDatabaseConfigTest.Config(xmlElem,
                ref secDBConfig, true);
            secDBConfig.Env = env;
            SecondaryQueueDatabase secDB;
            secDB = SecondaryQueueDatabase.Open(
                dbSecFileName, secDBConfig, openSecTxn);
    
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
            secExDB = SecondaryQueueDatabase.Open(
                dbSecFileName, secConfig, secTxn);

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

        public static void Confirm(XmlElement xmlElem,
            SecondaryQueueDatabase secDB, bool compulsory)
        {
            Configuration.ConfirmUint(xmlElem,
                "ExtentSize", secDB.ExtentSize, compulsory);
            Configuration.ConfirmUint(xmlElem, "Length",
                secDB.Length, compulsory);
            Configuration.ConfirmInt(xmlElem, "PadByte",
                secDB.PadByte, compulsory);
        }
    }
}

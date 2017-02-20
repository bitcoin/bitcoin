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
using BerkeleyDB;
using NUnit.Framework;

namespace CsharpAPITest
{
    public class Configuration
    {
        /*
         * Configure the value with data in xml and return true or
         * false to indicate if the value is configured. If there
         * is no testing data and it is optional, return false. If
         * there is no testing data in xml and it is compulsory,
         * ConfigNotFoundException will be thrown. If any testing
         * data is provided, the value will be set by the testing
         * data and true will be returned.
         */
        #region Config
        public static void ConfigAckPolicy(XmlElement xmlElem,
            string name, ref AckPolicy ackPolicy, bool compulsory)
        {
            XmlNode xmlNode = XMLReader.GetNode(xmlElem,
                name);
            if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);
            else if (xmlNode != null)
            {
                string policy = xmlNode.InnerText;
                if (policy == "ALL")
                    ackPolicy = AckPolicy.ALL;
                else if (policy == "ALL_PEERS")
                    ackPolicy = AckPolicy.ALL_PEERS;
                else if (policy == "NONE")
                    ackPolicy = AckPolicy.NONE;
                else if (policy == "ONE")
                    ackPolicy = AckPolicy.ONE;
                else if (policy == "ONE_PEER")
                    ackPolicy = AckPolicy.ONE_PEER;
                else if (policy == "QUORUM")
                    ackPolicy = AckPolicy.QUORUM;
                else
                    throw new InvalidConfigException(name);
            }
        }

        public static bool ConfigBool(XmlElement xmlElem,
            string name, ref bool value, bool compulsory)
        {
            XmlNode xmlNode;

            xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == false)
                return false;
            else if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);

            value = bool.Parse(xmlNode.InnerText);
            return true;
        }

        public static bool ConfigByteOrder(XmlElement xmlElem,
            string name, ref ByteOrder byteOrder, bool compulsory)
        {
            XmlNode xmlNode;

            xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == false)
                return false;
            else if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);

            byteOrder = ByteOrder.FromConst(
            int.Parse(xmlNode.InnerText));
            return true;
        }

        public static bool ConfigByteMatrix(XmlElement xmlElem,
            string name, ref byte[,] byteMatrix, bool compulsory)
        {
            int i, j, matrixLen;

            XmlNode xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == false)
                return false;
            else if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);

            matrixLen = xmlNode.ChildNodes.Count;
            byte[,] matrix = new byte[matrixLen, matrixLen];
            for (i = 0; i < matrixLen; i++)
            {
                if (xmlNode.ChildNodes[i].ChildNodes.Count != matrixLen)
                    throw new ConfigNotFoundException(name);
                for (j = 0; j < matrixLen; j++)
                {
                    matrix[i, j] = byte.Parse(
                        xmlNode.ChildNodes[i].ChildNodes[j].InnerText);
                }
            }

            byteMatrix = matrix;
            return true;
        }

        public static bool ConfigCacheInfo(XmlElement xmlElem,
            string name, ref CacheInfo cacheSize, bool compulsory)
        {
            XmlNode xmlNode;
            XmlNode xmlChildNode;
            uint bytes;
            uint gigabytes;
            int nCaches;

            xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);
            else if (xmlNode != null)
            {
                if ((xmlChildNode = XMLReader.GetNode(
                    (XmlElement)xmlNode, "Bytes")) != null)
                {
                    bytes = uint.Parse(xmlChildNode.InnerText);
                    if ((xmlChildNode = XMLReader.GetNode(
                         (XmlElement)xmlNode, "Gigabytes")) != null)
                    {
                        gigabytes = uint.Parse(xmlChildNode.InnerText);
                        if ((xmlChildNode = XMLReader.GetNode(
                            (XmlElement)xmlNode, "NCaches")) != null)
                        {
                            nCaches = int.Parse(xmlChildNode.InnerText);
                            cacheSize = new CacheInfo(gigabytes,bytes,nCaches);
                            return true;
                        }
                    }
                }
            }

            return false;
        }

        public static bool ConfigCachePriority(XmlElement xmlElem,
            string name, ref CachePriority cachePriority, bool compulsory)
        {
            XmlNode xmlNode;
            string priority;

            xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == false)
                return false;
            else if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);

            priority = xmlNode.InnerText;
            if (priority == "DEFAULT")
                cachePriority = CachePriority.DEFAULT;
            else if (priority == "HIGH")
                cachePriority = CachePriority.HIGH;
            else if (priority == "LOW")
                cachePriority = CachePriority.LOW;
            else if (priority == "VERY_HIGH")
                cachePriority = CachePriority.VERY_HIGH;
            else if (priority == "VERY_LOW")
                cachePriority = CachePriority.VERY_LOW;
            else
                throw new InvalidConfigException(name);

            return true;
        }

        public static bool ConfigCreatePolicy(XmlElement xmlElem,
            string name, ref CreatePolicy createPolicy, bool compulsory)
        {
            XmlNode xmlNode;

            xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == false)
                return false;
            else if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);

            if (xmlNode.InnerText == "ALWAYS")
                createPolicy = CreatePolicy.ALWAYS;
            else if (xmlNode.InnerText == "IF_NEEDED")
                createPolicy = CreatePolicy.IF_NEEDED;
            else if (xmlNode.InnerText == "NEVER")
                createPolicy = CreatePolicy.NEVER;
            else
                throw new InvalidConfigException(name);

            return true;
        }

        public static bool ConfigDuplicatesPolicy(XmlElement xmlElem,
            string name, ref DuplicatesPolicy duplicatePolicy,bool compulsory)
        {
            XmlNode xmlNode;

            xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == false)
                return false;
            else if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);

            if (xmlNode.InnerText == "NONE")
                duplicatePolicy = DuplicatesPolicy.NONE;
            else if (xmlNode.InnerText == "SORTED")
                duplicatePolicy = DuplicatesPolicy.SORTED;
            else if (xmlNode.InnerText == "UNSORTED")
                duplicatePolicy = DuplicatesPolicy.UNSORTED;
            else
                throw new InvalidConfigException(name);

            return true;
        }

        public static bool ConfigDateTime(XmlElement xmlElem,
            string name, ref DateTime time, bool compulsory)
        {
            XmlNode xmlNode;

            xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == false)
                return false;
            else if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);

            time = DateTime.Parse(xmlNode.InnerText);
            return true;
        }

        public static bool ConfigDeadlockPolicy(XmlElement xmlElem,
            string name, ref DeadlockPolicy deadlockPolicy, bool compulsory)
        {
            XmlNode xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == false)
                return false;
            else if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);

            string policy = xmlNode.InnerText;
            if (policy == "DEFAULT")
                deadlockPolicy = DeadlockPolicy.DEFAULT;
            else if (policy == "EXPIRE")
                deadlockPolicy = DeadlockPolicy.EXPIRE;
            else if (policy == "MAX_LOCKS")
                deadlockPolicy = DeadlockPolicy.MAX_LOCKS;
            else if (policy == "MAX_WRITE")
                deadlockPolicy = DeadlockPolicy.MAX_WRITE;
            else if (policy == "MIN_LOCKS")
                deadlockPolicy = DeadlockPolicy.MIN_LOCKS;
            else if (policy == "MIN_WRITE")
                deadlockPolicy = DeadlockPolicy.MIN_WRITE;
            else if (policy == "OLDEST")
                deadlockPolicy = DeadlockPolicy.OLDEST;
            else if (policy == "RANDOM")
                deadlockPolicy = DeadlockPolicy.RANDOM;
            else if (policy == "YOUNGEST")
                deadlockPolicy = DeadlockPolicy.YOUNGEST;
            else
                throw new InvalidConfigException(name);
            return true;
        }

        public static bool ConfigEncryption(XmlElement xmlElem,
            string name, DatabaseConfig dbConfig, bool compulsory)
        {
            EncryptionAlgorithm alg;
            XmlNode xmlNode;
            string tmp, password;
            
            xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == false)
                return false;
            else if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);

            password = XMLReader.GetNode((XmlElement)xmlNode,
                "password").InnerText;
            tmp = XMLReader.GetNode((XmlElement)xmlNode, "algorithm").InnerText;
            if (tmp == "AES")
                alg = EncryptionAlgorithm.AES;
            else
                alg = EncryptionAlgorithm.DEFAULT;
            dbConfig.SetEncryption(password, alg);
            return true;
        }

        public static bool ConfigInt(XmlElement xmlElem,
            string name, ref int value, bool compulsory)
        {
            XmlNode xmlNode;

            xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == false)
                return false;
            else if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);

            value = int.Parse(xmlNode.InnerText);
            return true;
        }

        public static bool ConfigIsolation(XmlElement xmlElem,
            string name, ref Isolation value, bool compulsory)
        {
            XmlNode xmlNode;
            int isolationDegree;

            xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == false)
                return false;
            else if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);

            isolationDegree = int.Parse(xmlNode.InnerText);
            if (isolationDegree == 1)
                value = Isolation.DEGREE_ONE;
            else if (isolationDegree == 2)
                value = Isolation.DEGREE_TWO;
            else if (isolationDegree == 3)
                value = Isolation.DEGREE_THREE;
            else
                throw new InvalidConfigException(name);

            return true;
        }

        public static bool ConfigLogFlush(XmlElement xmlElem,
            string name, ref TransactionConfig.LogFlush value,
            bool compulsory)
        {
            XmlNode xmlNode;
            string logFlush;

            xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == false)
                return false;
            else if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);

            logFlush = xmlNode.InnerText;
            if (logFlush == "DEFAULT")
                value = TransactionConfig.LogFlush.DEFAULT;
            else if (logFlush == "NOSYNC")
                value = TransactionConfig.LogFlush.NOSYNC;
            else if (logFlush == "WRITE_NOSYNC")
                value = TransactionConfig.LogFlush.WRITE_NOSYNC;
            else if (logFlush == "SYNC")
                value = TransactionConfig.LogFlush.SYNC;
            else
                throw new InvalidConfigException(name);

            return true;
        }

        public static bool ConfigLong(XmlElement xmlElem,
            string name, ref long value, bool compulsory)
        {
            XmlNode xmlNode;

            xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == false)
                return false;
            else if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);

            value = long.Parse(xmlNode.InnerText);
            return true;
        }

        public static bool ConfigMaxSequentialWrites(
            XmlElement xmlElem, string name, 
            MPoolConfig mpoolConfig, bool compulsory)
        {
            XmlNode xmlNode;
            uint pause;
            int writes;

            xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == false)
                return false;
            else if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);

            pause = uint.Parse(XMLReader.GetNode(
                (XmlElement)xmlNode, "pause").InnerText);
            writes = int.Parse(XMLReader.GetNode(
                (XmlElement)xmlNode,"maxWrites").InnerText);
            mpoolConfig.SetMaxSequentialWrites(writes, pause);
            return true;
        }

        public static bool ConfigReplicationHostAddress(
            XmlElement xmlElem, string name,
            ref ReplicationHostAddress address, bool compulsory)
        {
            XmlNode xmlNode = XMLReader.GetNode(
                xmlElem, name);
            if (xmlNode == null && compulsory == false)
                return false;
            else if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);

            address.Host = XMLReader.GetNode(
                (XmlElement)xmlNode, "Host").InnerText;
            address.Port = uint.Parse(XMLReader.GetNode(
                (XmlElement)xmlNode, "Port").InnerText);
            return true;
        }
        
        public static bool ConfigString(XmlElement xmlElem,
            string name, ref string valChar, bool compulsory)
        {
            XmlNode xmlNode;

            xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == false)
                return false;
            else if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);

            valChar = xmlNode.InnerText;
            return true;
        }

        public static bool ConfigStringList(XmlElement xmlElem,
            string name, ref List<string> strings, bool compulsory)
        {
            XmlNode xmlNode;

            xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == false)
                return false;
            else if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);

            XmlNodeList list = xmlNode.ChildNodes;
            for (int i = 0; i < list.Count; i++)
                strings.Add(list[i].InnerText);

            return true;
        }

        public static bool ConfigUint(XmlElement xmlElem,
            string name, ref uint value, bool compulsory)
        {
            XmlNode xmlNode;

            xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == false)
                return false;
            else if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);

            value = uint.Parse(xmlNode.InnerText);
            return true;
        }

        public static bool ConfigVerboseMessages(
            XmlElement xmlElem, string name, 
            ref VerboseMessages verbose, bool compulsory)
        {
            XmlNode xmlNode = XMLReader.GetNode(xmlElem, 
                name);
            if (xmlNode == null && compulsory == false)
                return false;
            else if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);

            ConfigBool((XmlElement)xmlNode, "AllFileOps",
                ref verbose.AllFileOps, compulsory);
            ConfigBool((XmlElement)xmlNode, "Deadlock",
                ref verbose.Deadlock, compulsory);
            ConfigBool((XmlElement)xmlNode, "FileOps",
                ref verbose.FileOps, compulsory);
            ConfigBool((XmlElement)xmlNode, "Recovery",
                ref verbose.Recovery, compulsory);
            ConfigBool((XmlElement)xmlNode, "Register",
                ref verbose.Register, compulsory);
            ConfigBool((XmlElement)xmlNode, "Replication",
                ref verbose.Replication, compulsory);
            ConfigBool((XmlElement)xmlNode, "ReplicationElection",
                ref verbose.ReplicationElection, compulsory);
            ConfigBool((XmlElement)xmlNode, "ReplicationLease",
                ref verbose.ReplicationLease, compulsory);
            ConfigBool((XmlElement)xmlNode, "ReplicationMessages",
                ref verbose.ReplicationMessages, compulsory);
            ConfigBool((XmlElement)xmlNode, "ReplicationMisc",
                ref verbose.ReplicationMisc, compulsory);
            ConfigBool((XmlElement)xmlNode, "ReplicationSync",
                ref verbose.ReplicationSync, compulsory);
            ConfigBool((XmlElement)xmlNode, "RepMgrConnectionFailure",
                ref verbose.RepMgrConnectionFailure, compulsory);
            ConfigBool((XmlElement)xmlNode, "RepMgrMisc",
                ref verbose.RepMgrMisc, compulsory);
            ConfigBool((XmlElement)xmlNode, "WaitsForTable",
                ref verbose.WaitsForTable, compulsory);
            return true;
        }

        #endregion Config

        /*
         * Confirm that the given value is the same with that in 
         * xml. If there is no testing data in xml and it is 
         * compulsory, the ConfigNotFoundException will be thrown. 
         * If there is no testing data and it is optional, nothing 
         * will be done. If any testing data is provided, the value 
         * will be checked.
         */
        #region Confirm
        public static void ConfirmAckPolicy(XmlElement xmlElem,
            string name, AckPolicy ackPolicy, bool compulsory)
        {
            XmlNode xmlNode = XMLReader.GetNode(xmlElem,
                name);
            if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);
            else if (xmlNode != null)
            {
                string policy = xmlNode.InnerText;
                if (policy == "ALL")
                    Assert.AreEqual(AckPolicy.ALL, 
                                            ackPolicy);
                else if (policy == "ALL_PEERS")
                    Assert.AreEqual(AckPolicy.ALL_PEERS, 
                                            ackPolicy);
                else if (policy == "NONE")
                    Assert.AreEqual(AckPolicy.NONE, 
                                            ackPolicy);
                else if (policy == "ONE")
                    Assert.AreEqual(AckPolicy.ONE, 
                                            ackPolicy);
                else if (policy == "ONE_PEER")
                    Assert.AreEqual(AckPolicy.ONE_PEER, 
                                            ackPolicy);
                else if (policy == "QUORUM")
                    Assert.AreEqual(AckPolicy.QUORUM, 
                                            ackPolicy);
                else
                    throw new InvalidConfigException(name);
            }
        }

        public static void ConfirmBool(XmlElement xmlElem,
            string name, bool value, bool compulsory)
        {
            XmlNode xmlNode;
            bool expected;

            xmlNode = XMLReader.GetNode(xmlElem,
                name);
            if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);
            else if (xmlNode != null)
            {
                if (xmlNode.ChildNodes.Count > 1)
                {
                    expected = bool.Parse(
                        xmlNode.FirstChild.NextSibling.InnerText);
                    Assert.AreEqual(expected, value);
                }
            }
        }

        /*
         * If configure MACHINE, the ByteOrder in database will 
         * switch to LITTLE_ENDIAN or BIG_ENDIAN according to the 
         * current machine.
         */
        public static void ConfirmByteOrder(XmlElement xmlElem,
            string name, ByteOrder byteOrder, bool compulsory)
        {
            XmlNode xmlNode;
            ByteOrder specOrder;

            xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);
            else if (xmlNode != null)
            {
                specOrder = ByteOrder.FromConst(int.Parse(
                    xmlNode.InnerText));
                if (specOrder == ByteOrder.MACHINE)
                    Assert.AreNotEqual(specOrder, byteOrder);
                else
                    Assert.AreEqual(specOrder, byteOrder);
            }
        }

        public static void ConfirmByteMatrix(XmlElement xmlElem,
            string name, byte[,] byteMatrix, bool compulsory)
        {
            int i, j, matrixLen;

            XmlNode xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);
            else if (xmlNode != null)
            {
                /*
                 * If the length of the 2 matrixes are not 
                 * the same, the matrixes are definately 
                 * not equal.
                 */
                matrixLen = xmlNode.ChildNodes.Count;
                Assert.AreEqual(matrixLen * matrixLen,byteMatrix.Length);

                /*
                 * Go over every element in the matrix to 
                 * see if the same with the given xml data.
                 */
                for (i = 0; i < matrixLen; i++)
                {
                    if (xmlNode.ChildNodes[i].ChildNodes.Count != matrixLen)
                        throw new ConfigNotFoundException(name);
                    for (j = 0; j < matrixLen; j++)
                        Assert.AreEqual(
                            byte.Parse(xmlNode.ChildNodes[i].ChildNodes[j].InnerText), 
                                                    byteMatrix[i, j]);
                }
            }
        }

        public static void ConfirmCachePriority(XmlElement xmlElem,
            string name, CachePriority priority, bool compulsory)
        {
            XmlNode xmlNode;

            xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);
            else if (xmlNode != null)
            {
                if (xmlNode.InnerText == "DEFAULT")
                    Assert.AreEqual(CachePriority.DEFAULT, priority);
                else if (xmlNode.InnerText == "HIGH")
                    Assert.AreEqual(CachePriority.HIGH, priority);
                else if (xmlNode.InnerText == "LOW")
                    Assert.AreEqual(CachePriority.LOW, priority);
                else if (xmlNode.InnerText == "VERY_HIGH")
                    Assert.AreEqual(CachePriority.VERY_HIGH, priority);
                else if (xmlNode.InnerText == "VERY_LOW")
                    Assert.AreEqual(CachePriority.VERY_LOW, priority);
            }
        }

        public static void ConfirmCacheSize(XmlElement xmlElem,
            string name, CacheInfo cache, bool compulsory)
        {
            uint bytes;
            uint gigabytes;
            int nCaches;
            XmlNode xmlNode;
            XmlNode xmlChildNode;

            xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);
            else if (xmlNode != null)
            {
                if ((xmlChildNode = XMLReader.GetNode(
                    (XmlElement)xmlNode, "Bytes")) != null)
                {
                    bytes = uint.Parse(xmlChildNode.InnerText);
                    if ((xmlChildNode = XMLReader.GetNode(
                        (XmlElement)xmlNode, "Gigabytes")) != null)
                    {
                        gigabytes = uint.Parse(xmlChildNode.InnerText);
                        if ((xmlChildNode = XMLReader.GetNode(
                            (XmlElement)xmlNode,
                            "NCaches")) != null)
                        {
                            nCaches = int.Parse(xmlChildNode.InnerText);
                            Assert.LessOrEqual(bytes, cache.Bytes);
                            Assert.AreEqual(gigabytes, cache.Gigabytes);
                            Assert.AreEqual(nCaches, cache.NCaches);
                        }
                    }
                }
            }
        }

        /*
         * If bytes in CacheSize is assigned, the bytes in cachesize
         * couldn't be the default one.
         */
        public static void ConfirmCacheSize(XmlElement xmlElem,
            string name, CacheInfo cache, uint defaultCache,
            bool compulsory)
        {
            uint bytes;
            uint gigabytes;
            int nCaches;
            XmlNode xmlNode;
            XmlNode xmlChildNode;

            xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);
            else if (xmlNode != null)
            {
                if ((xmlChildNode = XMLReader.GetNode(
                    (XmlElement)xmlNode, "Bytes")) != null)
                {
                    bytes = defaultCache;
                    if ((xmlChildNode = XMLReader.GetNode(
                        (XmlElement)xmlNode, "Gigabytes")) != null)
                    {
                        gigabytes = uint.Parse(xmlChildNode.InnerText);
                        if ((xmlChildNode = XMLReader.GetNode(
                            (XmlElement)xmlNode, "NCaches")) != null)
                        {
                            nCaches = int.Parse(xmlChildNode.InnerText);
                            Assert.AreNotEqual(bytes, cache.Bytes);
                            Assert.AreEqual(gigabytes, cache.Gigabytes);
                            Assert.AreEqual(nCaches, cache.NCaches);
                        }
                    }
                }
            }
        }

        public static void ConfirmCreatePolicy(XmlElement xmlElem,
            string name, CreatePolicy createPolicy, bool compulsory)
        {
            XmlNode xmlNode;

            xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);
            else if (xmlNode != null)
            {
                if (xmlNode.InnerText == "ALWAYS")
                    Assert.IsTrue(createPolicy.Equals(CreatePolicy.ALWAYS));
                else if (xmlNode.InnerText == "IF_NEEDED")
                    Assert.IsTrue(createPolicy.Equals(CreatePolicy.IF_NEEDED));
                else if (xmlNode.InnerText == "NEVER")
                    Assert.IsTrue(createPolicy.Equals(CreatePolicy.NEVER));
            }
        }

        public static void ConfirmDataBaseType(XmlElement xmlElem,
            string name, DatabaseType dbType, bool compulsory)
        {
            XmlNode xmlNode;
            xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);
            else if (xmlNode != null)
            {
                if (xmlNode.InnerText == "BTREE")
                    Assert.AreEqual(dbType, DatabaseType.BTREE);
                else if (xmlNode.InnerText == "HASH")
                    Assert.AreEqual(dbType, DatabaseType.HASH);
                else if (xmlNode.InnerText == "QUEUE")
                    Assert.AreEqual(dbType, DatabaseType.QUEUE);
                else if (xmlNode.InnerText == "RECNO")
                    Assert.AreEqual(dbType, DatabaseType.RECNO);
                else if (xmlNode.InnerText == "UNKNOWN")
                    Assert.AreEqual(dbType, DatabaseType.UNKNOWN);
            }
        }

        public static void ConfirmDateTime(XmlElement xmlElem,
            string name, DateTime time, bool compulsory)
        {
            XmlNode xmlNode;

            xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);
            else if (xmlNode != null)
                Assert.AreEqual(DateTime.Parse(
                    xmlNode.InnerText), time);
        }

        public static void ConfirmDeadlockPolicy(XmlElement xmlElem,
            string name, DeadlockPolicy deadlockPolicy, bool compulsory)
        {
            XmlNode xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);
            else if (xmlNode != null)
            {
                string policy = xmlNode.InnerText;
                if (policy == "DEFAULT")
                    Assert.AreEqual(DeadlockPolicy.DEFAULT, deadlockPolicy);
                else if (policy == "EXPIRE")
                    Assert.AreEqual(DeadlockPolicy.EXPIRE, deadlockPolicy);
                else if (policy == "MAX_LOCKS")
                    Assert.AreEqual(DeadlockPolicy.MAX_LOCKS, deadlockPolicy);
                else if (policy == "MAX_WRITE")
                    Assert.AreEqual(DeadlockPolicy.MAX_WRITE, deadlockPolicy);
                else if (policy == "MIN_LOCKS")
                    Assert.AreEqual(DeadlockPolicy.MIN_LOCKS, deadlockPolicy);
                else if (policy == "MIN_WRITE")
                    Assert.AreEqual(DeadlockPolicy.MIN_WRITE, deadlockPolicy);
                else if (policy == "OLDEST")
                    Assert.AreEqual(DeadlockPolicy.OLDEST, deadlockPolicy);
                else if (policy == "RANDOM")
                    Assert.AreEqual(DeadlockPolicy.RANDOM, deadlockPolicy);
                else if (policy == "YOUNGEST")
                    Assert.AreEqual(DeadlockPolicy.YOUNGEST, deadlockPolicy);
                else
                    throw new InvalidConfigException(name);
            }
        }

        public static void ConfirmDuplicatesPolicy(
            XmlElement xmlElem, string name,
            DuplicatesPolicy duplicatedPolicy, bool compulsory)
        {
            XmlNode xmlNode;
            xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);
            else if (xmlNode != null)
            {
                if (xmlNode.InnerText == "NONE")
                    Assert.AreEqual(duplicatedPolicy, DuplicatesPolicy.NONE);
                else if (xmlNode.InnerText == "SORTED")
                    Assert.AreEqual(duplicatedPolicy, DuplicatesPolicy.SORTED);
                else if (xmlNode.InnerText == "UNSORTED")
                    Assert.AreEqual(duplicatedPolicy, DuplicatesPolicy.UNSORTED);
            }
        }

        public static void ConfirmEncryption(XmlElement xmlElem,
            string name, string dPwd, EncryptionAlgorithm dAlg, bool compulsory)
        {
            EncryptionAlgorithm alg;
            XmlNode xmlNode = XMLReader.GetNode(xmlElem, 
                name);
            if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);
            else if (xmlNode != null)
            {
                string password = XMLReader.GetNode(
                    (XmlElement)xmlNode, "password").InnerText;
                string tmp = XMLReader.GetNode(
                    (XmlElement)xmlNode, "algorithm").InnerText;
                if (tmp == "AES")
                    alg = EncryptionAlgorithm.AES;
                else
                    alg = EncryptionAlgorithm.DEFAULT;
                Assert.AreEqual(dAlg, alg);
                Assert.AreEqual(dPwd, dPwd);
            }
        }

        public static void ConfirmInt(XmlElement xmlElem,
            string name, int value, bool compulsory)
        {
            XmlNode xmlNode;
            xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);
            else if (xmlNode != null)
                Assert.AreEqual(int.Parse(xmlNode.InnerText), value);
        }

        public static void ConfirmIsolation(XmlElement xmlElem,
            string name, Isolation value, bool compulsory)
        {
            XmlNode xmlNode;
            int isolationDegree;

            xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);
            else if (xmlNode != null)
            {
                isolationDegree = int.Parse(xmlNode.InnerText);
                if (isolationDegree == 1)
                    Assert.AreEqual(Isolation.DEGREE_ONE, value);
                else if (isolationDegree == 2)
                    Assert.AreEqual(Isolation.DEGREE_TWO, value);
                else if (isolationDegree == 3)
                    Assert.AreEqual(Isolation.DEGREE_THREE, value);
                else
                    throw new InvalidConfigException(name);
            }
        }

        public static void ConfirmLogFlush(XmlElement xmlElem,
            string name, TransactionConfig.LogFlush value,
            bool compulsory)
        {
            XmlNode xmlNode;
            string logFlush;

            xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);
            else if (xmlNode != null)
            {
                logFlush = xmlNode.InnerText;
                if (logFlush == "DEFAULT")
                    Assert.AreEqual(TransactionConfig.LogFlush.DEFAULT, value);
                else if (logFlush == "NOSYNC")
                    Assert.AreEqual(TransactionConfig.LogFlush.NOSYNC, value);
                else if (logFlush == "WRITE_NOSYNC")
                    Assert.AreEqual(TransactionConfig.LogFlush.WRITE_NOSYNC, value);
                else if (logFlush == "SYNC")
                    Assert.AreEqual(TransactionConfig.LogFlush.SYNC, value);
                else
                    throw new InvalidConfigException(name);
            }
        }

        public static void ConfirmLong(XmlElement xmlElem,
            string name, long value, bool compulsory)
        {
            XmlNode xmlNode;
            xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);
            else if (xmlNode != null)
                Assert.AreEqual(long.Parse(xmlNode.InnerText), value);
        }

        public static void ConfirmMaxSequentialWrites(
            XmlElement xmlElem, string name,
            uint mPause, int mWrites, bool compulsory)
        {
            XmlNode xmlNode;
            uint pause;
            int writes;

            xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);
            else if (xmlNode != null)
            {
                writes = int.Parse(XMLReader.GetNode(
                    (XmlElement)xmlNode, "maxWrites").InnerText);
                pause = uint.Parse(XMLReader.GetNode(
                    (XmlElement)xmlNode, "pause").InnerText);

                Assert.AreEqual(mPause, pause);
                Assert.AreEqual(mWrites, writes);
            }
        }

        public static void ConfirmReplicationHostAddress(
            XmlElement xmlElem, string name,
            ReplicationHostAddress address, bool compulsory)
        {
            XmlNode xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);
            else if (xmlNode != null)
            {
                string host = XMLReader.GetNode(
                    (XmlElement)xmlNode, "Host").InnerText;
                uint port = uint.Parse(XMLReader.GetNode(
                    (XmlElement)xmlNode, "Port").InnerText);

                Assert.AreEqual(host, address.Host);
                Assert.AreEqual(port, address.Port);
            }
        }

        public static void ConfirmString(XmlElement xmlElem,
            string name, string str, bool compulsory)
        {
            XmlNode xmlNode;

            if (str != null)
            {
                xmlNode = XMLReader.GetNode(xmlElem, name);
                if (xmlNode == null && compulsory == true)
                    throw new ConfigNotFoundException(name);
                else if (xmlNode != null)
                {
                    if (xmlNode.HasChildNodes)
                        Assert.AreEqual(
                            xmlNode.FirstChild.InnerText, str);
                }
            }
        }

        public static void ConfirmStringList(XmlElement xmlElem,
            string name, List<string> strings, bool compulsory)
        {
            XmlNode xmlNode;

            if (strings != null)
            {
                xmlNode = XMLReader.GetNode(xmlElem, name);
                if (xmlNode == null && compulsory == true)
                    throw new ConfigNotFoundException(name);
                else if (xmlNode != null)
                {
                    if (xmlNode.HasChildNodes)
                    {
                        XmlNodeList list = xmlNode.ChildNodes;
                        for (int i = 0; i < xmlNode.ChildNodes.Count;i++)
                            Assert.IsTrue(
                                strings.Contains(list[i].InnerText));
                    }
                }
            }
        }

        public static void ConfirmUint(XmlElement xmlElem,
            string name, uint value, bool compulsory)
        {
            XmlNode xmlNode;

            xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);
            else if (xmlNode != null)
                Assert.AreEqual(uint.Parse(xmlNode.InnerText), value);
        }

        public static void ConfirmVerboseMessages(
            XmlElement xmlElem, string name, 
            VerboseMessages verbose, bool compulsory)
        {
            XmlNode xmlNode = XMLReader.GetNode(xmlElem, name);
            if (xmlNode == null && compulsory == true)
                throw new ConfigNotFoundException(name);
            else if (xmlNode != null)
            {
                ConfirmBool((XmlElement)xmlNode, "AllFileOps",
                    verbose.AllFileOps, compulsory);
                ConfirmBool((XmlElement)xmlNode, "Deadlock",
                    verbose.Deadlock, compulsory);
                ConfirmBool((XmlElement)xmlNode, "FileOps",
                    verbose.FileOps, compulsory);
                ConfirmBool((XmlElement)xmlNode, "Recovery",
                    verbose.Recovery, compulsory);
                ConfirmBool((XmlElement)xmlNode, "Register",
                    verbose.Register, compulsory);
                ConfirmBool((XmlElement)xmlNode, "Replication",
                    verbose.Replication, compulsory);
                ConfirmBool((XmlElement)xmlNode, "ReplicationElection",
                    verbose.ReplicationElection, compulsory);
                ConfirmBool((XmlElement)xmlNode, "ReplicationLease",
                    verbose.ReplicationLease, compulsory);
                ConfirmBool((XmlElement)xmlNode, "ReplicationMessages",
                    verbose.ReplicationMessages, compulsory);
                ConfirmBool((XmlElement)xmlNode, "ReplicationMisc",
                    verbose.ReplicationMisc, compulsory);
                ConfirmBool((XmlElement)xmlNode, "ReplicationSync",
                    verbose.ReplicationSync, compulsory);
                ConfirmBool((XmlElement)xmlNode, "RepMgrConnectionFailure",
                    verbose.RepMgrConnectionFailure, compulsory);
                ConfirmBool((XmlElement)xmlNode, "RepMgrMisc", 
                    verbose.RepMgrMisc, compulsory);
                ConfirmBool((XmlElement)xmlNode,"WaitsForTable", 
                    verbose.WaitsForTable, compulsory);
            }
        }
        #endregion Confirm

        public static void dbtFromString(DatabaseEntry dbt, string s)
        {
            dbt.Data = System.Text.Encoding.ASCII.GetBytes(s);
        }

        public static string strFromDBT(DatabaseEntry dbt)
        {

            System.Text.ASCIIEncoding decode = new ASCIIEncoding();
            return decode.GetString(dbt.Data);
        }

        /*
         * Reading params successfully returns true. Unless returns 
         * false. The retrieved Xml fragment is returning in xmlElem.
         */
        public static XmlElement TestSetUp(string testFixtureName, string testName)
        {
            XMLReader xmlReader = new XMLReader("../../AllTestData.xml");
            XmlElement xmlElem = xmlReader.GetXmlElement(testFixtureName, testName);
            if (xmlElem == null)
                throw new ConfigNotFoundException(testFixtureName + ":" + testName);
            else
                return xmlElem;
        }

        /*
         * Delete existing test output directory and its files, 
         * then create a new one.
         */
        public static void ClearDir(string testDir)
        {
            if (Directory.Exists(testDir))
                Directory.Delete(testDir, true);
            Directory.CreateDirectory(testDir);
        }
    }
}

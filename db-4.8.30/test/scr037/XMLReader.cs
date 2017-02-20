/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;
using System.Xml.XPath;

namespace CsharpAPITest
{
    public class XMLReader
    {
        private static string path;

        public XMLReader(string XmlFileName)
        {
            path = XmlFileName;
        }

        public XmlElement GetXmlElement(string className, string testName)
        {
            XmlDocument doc = new XmlDocument();
            doc.Load(path);

            string xpath = string.Format("/Assembly/TestFixture[@name=\"{0}\"]/Test[@name=\"{1}\"]", className, testName);
            XmlElement testCase = doc.SelectSingleNode(xpath) as XmlElement;
            if (testCase == null)
                return null;
            else
                return testCase;
        }

        public static XmlNode GetNode(XmlElement xmlElement,
            string nodeName)
        {
            XmlNodeList xmlNodeList = xmlElement.SelectNodes(nodeName);
            if (xmlNodeList.Count > 1)
                throw new Exception(nodeName + " Configuration Error");
            else
                return xmlNodeList.Item(0);
        }

    }
}

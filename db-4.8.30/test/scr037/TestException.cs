/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.Text;

namespace CsharpAPITest
{
    
    public  class TestException : ApplicationException
    {
        public TestException(string name)
            : base(name)
        { 
        }

        public TestException() : base()
        {
        }
    }

    public class ConfigNotFoundException : TestException
    {
        public ConfigNotFoundException(string name)
            : base(name)
        { 
        }
    }

    public class InvalidConfigException : TestException
    {
        public InvalidConfigException(string name)
            : base(name)
        { 
        }
    }

    public class ExpectedTestException : TestException
    {
        public ExpectedTestException()
            : base()
        {
        }
    }
}

/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.Text;

namespace ex_txn {
    [Serializable()]
    public class PayloadData {
        private int oID;
        private string threadName;
        private double doubleData;

        public PayloadData(int id, string name, double data) {
            oID = id;
            threadName = name;
            doubleData = data;
        }

        public double DoubleData {
            get { return doubleData; }
            set { doubleData = value; } 
        }

        public int ID {
            get { return oID;  }
            set { oID = value;  }
        }

        public string ThreadName {
            get { return threadName;  }
            set { threadName = value;  }
        }
    }
}

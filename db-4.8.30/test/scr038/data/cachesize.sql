CREATE DATABASE numismatics;  /*+ CACHESIZE = 16m */

CREATE TABLE coin (cid INT(8) PRIMARY KEY,
                       unit VARCHAR(20));

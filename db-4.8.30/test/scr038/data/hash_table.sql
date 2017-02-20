CREATE DATABASE numismatics;  /*+ CACHESIZE = 16m */

CREATE TABLE mint (mid INT(8) PRIMARY KEY,
       country VARCHAR(20),
       city VARCHAR(20));   --+ DBTYPE = HASH



CREATE DATABASE numismatics;  /*+ CACHESIZE = 16m */

CREATE TABLE coin (cid INT(8) PRIMARY KEY,
                       unit VARCHAR2(20),
                       value NUMERIC(8,2), -- just a comment here;
                       mintage_year INT(8),
                       mint_id INT(8),
                       CONSTRAINT mint_id_fk FOREIGN KEY(mint_id)
                           REFERENCES mint(mid));

CREATE TABLE mint (mid INT(8) PRIMARY KEY,
       country VARCHAR2(20),
       city VARCHAR2(20));   --+ DBTYPE = HASH

CREATE INDEX unit_index ON coin(unit);

CREATE TABLE random (rid VARCHAR2(20) PRIMARY KEY,
       chunk bin(127));

/* SELECT * from mint;*/  

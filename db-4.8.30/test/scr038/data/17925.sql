CREATE DATABASE numismatics;  

CREATE TABLE coin (cid INT(8) PRIMARY KEY,
                       unit VARCHAR2(20),
                       value NUMERIC(8,2),  
                       mintage_year INT(8),
                       mint_id INT(8),
                       CONSTRAINT mint_id_fk FOREIGN KEY(mint_id)
                           REFERENCES mint(mid));

CREATE INDEX unit_index ON coin(unit);

CREATE TABLE mint (mid INT(8) PRIMARY KEY,
       country VARCHAR2(20),
       city VARCHAR2(20)); 

CREATE INDEX mid_index ON mint(mid); 

CREATE TABLE random (rid VARCHAR2(20) PRIMARY KEY,
       chunk bin(127));

CREATE TABLE table1 (att_int INT(8) PRIMARY KEY,
		 	att_char CHAR(20),
			att_varchar VARCHAR(20),
			att_bit BIT,
			att_tinyint TINYINT,
			att_smallint SMALLINT(2),
			att_integer INTEGER(4),
			att_bigint BIGINT,
			att_real REAL,
			att_double DOUBLE,
			att_float FLOAT,
			att_decimal DECIMAL,
			att_numeric NUMERIC,
			att_bin bin(5));

CREATE TABLE table2(att_int INT(8) PRIMARY KEY);
		
CREATE TABLE table3(att_char CHAR(20) PRIMARY KEY);

CREATE TABLE table4(att_bin bin(10) PRIMARY KEY);

CREATE TABLE table5(att_bin bin(10) PRIMARY KEY,
			att_bit BIT);

CREATE TABLE table6(att_bin bin(10),
                        att_bit BIT PRIMARY KEY); 

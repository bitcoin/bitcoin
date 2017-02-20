
CREATE DATABASE numismatics;

CREATE TABLE table1 (att_int INT(8) PRIMARY KEY,
			att_char CHAR(2),
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
			att_bin bin(10),
			CONSTRAINT table8_fk FOREIGN KEY(att_integer)
                           REFERENCES table8(att_integer));

CREATE TABLE table2 (att_int INT(8) PRIMARY KEY);

CREATE TABLE table3 (att_char CHAR(2) PRIMARY KEY);

CREATE TABLE table4 (att_varchar VARCHAR(20) PRIMARY KEY);

CREATE TABLE table5 (att_bit BIT PRIMARY KEY);

CREATE TABLE table6 (att_tinyint TINYINT PRIMARY KEY);

CREATE TABLE table7 (att_smallint SMALLINT(2) PRIMARY KEY);

CREATE TABLE table8 (att_integer INTEGER(4) PRIMARY KEY);

CREATE TABLE table9 (att_bigint BIGINT PRIMARY KEY);

CREATE TABLE table10 (att_real REAL PRIMARY KEY);

CREATE TABLE table11 (att_double DOUBLE PRIMARY KEY);

CREATE TABLE table12 (att_float FLOAT PRIMARY KEY);

CREATE TABLE table13 (att_decimal DECIMAL PRIMARY KEY);

CREATE TABLE table14 (att_numeric NUMERIC PRIMARY KEY);

CREATE TABLE table15 (att_binary bin(10) PRIMARY KEY);

CREATE TABLE table16 (att_int INT(8) PRIMARY KEY,
			att_char CHAR(2),
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
			att_bin bin(10),
			CONSTRAINT table17_fk FOREIGN KEY(att_int)
                           REFERENCES table17(att_int));

CREATE TABLE table17 (att_int INT(8) PRIMARY KEY);

CREATE TABLE table18 (att_char CHAR(2) PRIMARY KEY);

CREATE TABLE table19 (att_varchar VARCHAR(20) PRIMARY KEY);

CREATE TABLE table20 (att_bit BIT PRIMARY KEY);

CREATE TABLE table21 (att_tinyint TINYINT PRIMARY KEY);

CREATE TABLE table22 (att_smallint SMALLINT(2) PRIMARY KEY);

CREATE TABLE table23 (att_integer INTEGER(4) PRIMARY KEY);

CREATE TABLE table24 (att_bigint BIGINT PRIMARY KEY);

CREATE TABLE table25 (att_real REAL PRIMARY KEY);

CREATE TABLE table26 (att_double DOUBLE PRIMARY KEY);

CREATE TABLE table27 (att_float FLOAT PRIMARY KEY);

CREATE TABLE table28 (att_decimal DECIMAL PRIMARY KEY);

CREATE TABLE table29 (att_numeric NUMERIC PRIMARY KEY);

CREATE TABLE table30 (att_binary bin(100) PRIMARY KEY);


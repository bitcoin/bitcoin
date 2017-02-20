CREATE DATABASE numismatics;  

CREATE TABLE table1 (att_int INT(8) PRIMARY KEY,
			att_char CHAR(2));

CREATE INDEX int_index ON table1(att_int);

CREATE INDEX int_index_1 ON table1(att_int);



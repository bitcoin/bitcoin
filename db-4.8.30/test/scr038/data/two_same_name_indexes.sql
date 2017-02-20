CREATE DATABASE numismatics;  

CREATE TABLE coin (cid INT(8) PRIMARY KEY,
                       unit VARCHAR(20),
                       value NUMERIC(8,2), 
                       mintage_year INT(8));

CREATE INDEX unit_index ON coin(unit);

CREATE INDEX unit_index1 ON coin(unit);


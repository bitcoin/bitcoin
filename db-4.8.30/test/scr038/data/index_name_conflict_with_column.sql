CREATE DATABASE numismatics;  

CREATE TABLE coin (cid INT(8) PRIMARY KEY,
                       unit VARCHAR(20),
                       value NUMERIC(8,2), 
                       mintage_year INT(8),
                       mint_id INT(8),
                       CONSTRAINT mint_id_fk FOREIGN KEY(mint_id)
                           REFERENCES mint(mid));

CREATE TABLE mint (mid INT(8) PRIMARY KEY,
       country VARCHAR(20),
       city VARCHAR(20));   

CREATE INDEX coin ON coin(unit);




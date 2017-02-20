CREATE DATABASE numismatics;  /*+ CACHESIZE = 16m */

CREATE TABLE coin (cid INT(8) PRIMARY KEY,
                       unit VARCHAR(20),
                       value NUMERIC(8,2), -- just a comment here;
                       mintage_year INT(8),
                       mint_id INT(8),
                       CONSTRAINT mint_id_fk FOREIGN KEY(mint_id)
                           REFERENCES mint(mid));

CREATE TABLE mint (mid INT(8) PRIMARY KEY,
       country VARCHAR(20),
       city VARCHAR(20),
	zip_code INT(8));   --+ DBTYPE = HASH



CREATE INDEX value_index ON coin(value);

CREATE INDEX mint_id_index ON coin(mint_id);

CREATE INDEX mintage_year_index ON coin(mintage_year);

CREATE INDEX zip_code_index ON mint(zip_code);

/* SELECT * from mint;*/

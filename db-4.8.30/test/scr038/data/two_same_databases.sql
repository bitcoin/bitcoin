CREATE DATABASE numismatics;  

CREATE DATABASE numismatics;

CREATE TABLE coin (cid INT(8) PRIMARY KEY,
                       unit VARCHAR(20),
                       value NUMERIC(8,2), -- just a comment here;
                       mintage_year INT(8),
                       mint_id INT(8),
                       CONSTRAINT mint_id_fk FOREIGN KEY(mint_id)
                           REFERENCES mint(mid));

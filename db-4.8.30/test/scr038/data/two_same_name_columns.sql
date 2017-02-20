CREATE DATABASE numismatics; 

CREATE TABLE coin (cid INT(8) PRIMARY KEY,
                       unit VARCHAR(20),
                       unit NUMERIC(8,2), 
                       mintage_year INT(8),
                       mint_id INT(8),
                       CONSTRAINT mint_id_fk FOREIGN KEY(mint_id)
                           REFERENCES mint(mid));

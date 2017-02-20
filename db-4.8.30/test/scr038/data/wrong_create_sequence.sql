
CREATE INDEX unit_index ON coin(unit);

CREATE TABLE coin (cid INT(8) PRIMARY KEY,
                       unit VARCHAR(20),
                       value NUMERIC(8,2), -- just a comment here;
                       mintage_year INT(8),
                       mint_id INT(8));

CREATE DATABASE numismatics;  

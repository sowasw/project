# Create webay_commodity table  

DROP TABLE IF EXISTS webay_commodity;
#@ _CREATE_TABLE_
CREATE TABLE webay_commodity
(
  commodity_id      INT UNSIGNED NOT NULL AUTO_INCREMENT,
  PRIMARY KEY (commodity_id),
  name              VARCHAR(20) NOT NULL,
  price             DOUBLE(10,2) UNSIGNED NOT NULL,
  quantity          INT UNSIGNED NOT NULL
) ENGINE=InnoDB;
#@ _CREATE_TABLE_

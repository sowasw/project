# create webay_user table 

DROP TABLE IF EXISTS webay_user;
#@ _CREATE_TABLE_
CREATE TABLE webay_user
(
  user_id       INT UNSIGNED NOT NULL AUTO_INCREMENT,
  PRIMARY KEY (user_id),
  name          VARCHAR(20) NOT NULL,
  user_password VARCHAR(20) NOT NULL,
  email         VARCHAR(36) NULL
) ENGINE=InnoDB;
#@ _CREATE_TABLE_

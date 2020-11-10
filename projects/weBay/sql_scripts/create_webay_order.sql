# Create webay_order table  

DROP TABLE IF EXISTS webay_order;
#@ _CREATE_TABLE_
CREATE TABLE webay_order
(
  order_id          INT UNSIGNED NOT NULL AUTO_INCREMENT,
  PRIMARY KEY (order_id),
  user_id           INT UNSIGNED NOT NULL,
  INDEX(user_id),
  FOREIGN KEY (user_id) REFERENCES webay_user (user_id),
  date_time         DATETIME NOT NULL
) ENGINE=InnoDB;
#@ _CREATE_TABLE_

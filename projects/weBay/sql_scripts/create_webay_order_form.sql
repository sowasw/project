# Create webay_order_form table  

DROP TABLE IF EXISTS webay_order_form;
#@ _CREATE_TABLE_
CREATE TABLE webay_order_form
(
  order_id          INT UNSIGNED NOT NULL,
  commodity_id      INT UNSIGNED NOT NULL,
  PRIMARY KEY (order_id,commodity_id),
  INDEX(commodity_id),
  FOREIGN KEY (order_id) REFERENCES webay_order (order_id),
  FOREIGN KEY (commodity_id) REFERENCES webay_commodity (commodity_id),
  quantity          INT UNSIGNED NOT NULL,
  sum_of_money      DOUBLE(10,2) UNSIGNED NOT NULL
) ENGINE=InnoDB;
#@ _CREATE_TABLE_

# sudo mysql -p -u root;
CREATE USER 'webay'@'localhost' IDENTIFIED BY 'webay123';
GRANT ALL ON webay_db.* TO 'webay'@'localhost';

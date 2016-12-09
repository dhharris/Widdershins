-- CREATE USER 'user'@'localhost' IDENTIFIED BY 'password';
GRANT SELECT ON Search.url_list TO 'user'@'localhost';
GRANT INSERT ON Search.url_list TO 'user'@'localhost';
GRANT UPDATE ON Search.url_list TO 'user'@'localhost';
GRANT DELETE ON Search.url_list TO 'user'@'localhost';

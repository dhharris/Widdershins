-- CREATE DATABASE Search;
USE Search;
CREATE TABLE url_list(
        id INT AUTO_INCREMENT PRIMARY KEY,
        title VARCHAR(100) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NOT NULL, -- HTML <title> element
        description VARCHAR(300) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NOT NULL, -- <meta name="description" ...>
        url VARCHAR(100) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci UNIQUE NOT NULL,
        data VARCHAR(2048) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NOT NULL
);

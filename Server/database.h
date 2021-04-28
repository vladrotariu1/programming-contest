//
// Created by vlad on 1/5/21.
//

#ifndef SERVER_DATABASE_H
#define SERVER_DATABASE_H

#define DB_PATH "/home/vlad/CLionProjects/Server/identifier.sqlite"

int create_normal_user (sqlite3 * DB, char * user_name, char * password, char * err_msg);
int user_exists (sqlite3 * DB, char * user_name, char ** err_msg);
int user_password_match(struct sqlite3 * DB, char * user_name, char * password, char ** err_msg);


#endif //SERVER_DATABASE_H

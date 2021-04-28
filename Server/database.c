//
// Created by vlad on 1/5/21.
//

#include <stdio.h>
#include <errno.h>
#include <sqlite3.h>
#include <string.h>
#include <stdlib.h>
#include "database.h"

extern int errno;

int create_normal_user(sqlite3 * DB, char * user_name, char * password, char * err_msg) {

    // Create SQL query that should look like this:
    // INSERT INTO users (user_name, password, admin) VALUES ('user_name', 'password', 0);
    char sql_query[1024];
    strcpy(sql_query, "INSERT INTO users (user_name, password, admin) VALUES ");
    strcat(sql_query, "(");
    strcat(sql_query, "'");
    strcat(sql_query, user_name);
    strcat(sql_query, "'");
    strcat(sql_query, ", ");
    strcat(sql_query, "'");
    strcat(sql_query, password);
    strcat(sql_query, "'");
    strcat(sql_query, ", ");
    strcat(sql_query, "0");
    strcat(sql_query, ");");

    // Execute query
    return sqlite3_exec(DB, sql_query, 0, 0, &err_msg);
}


int user_exists (sqlite3 * DB, char * user_name, char ** err_msg) {

    // Create SQL statement and SQL query variables
    sqlite3_stmt * stmt;
    char sql_query[1024];


    // Crete SQL query that should look like this:
    // SELECT user_name FROM users WHERE user_name LIKE 'user_name';
    strcpy(sql_query, "SELECT user_name FROM users WHERE user_name LIKE ");
    strcat(sql_query, "'");
    strcat(sql_query, user_name);
    strcat(sql_query, "'");
    strcat(sql_query, ";");


    // Prepare statement
    sqlite3_prepare(DB, sql_query, -1, &stmt, 0);
    char * sql_user_name;


    // Get any errors
    char * message = sqlite3_errmsg(DB);

    if (strcmp(message, "not an error") != 0) {
        *err_msg = malloc(strlen(message));
        strcpy(*err_msg, message);
        return -1;
    }


    // Make query step
    sqlite3_step(stmt);
    sql_user_name = sqlite3_column_text(stmt, 0);


    // Return true or false
    if (sql_user_name != NULL) return 1;
    else return 0;
}


int user_password_match(struct sqlite3 * DB, char * user_name, char * password, char ** err_msg) {

    // Create SQL statement and SQL query variables
    sqlite3_stmt *stmt;
    char sql_query[1024];


    // Crete SQL query that should look like this:
    // SELECT user_id FROM users WHERE user_name LIKE 'user_name' AND password LIKE 'password';
    strcpy(sql_query, "SELECT user_id FROM users WHERE user_name LIKE ");
    strcat(sql_query, "'");
    strcat(sql_query, user_name);
    strcat(sql_query, "' ");
    strcat(sql_query, "AND password LIKE ");
    strcat(sql_query, "'");
    strcat(sql_query, password);
    strcat(sql_query, "';");


    // Prepare statement
    sqlite3_prepare(DB, sql_query, -1, &stmt, 0);
    int sql_user_id = -1;


    // Get any errors
    char * message = sqlite3_errmsg(DB);

    if (strcmp(message, "not an error") != 0) {
        *err_msg = malloc(strlen(message));
        strcpy(*err_msg, message);
        return -1;
    }


    // Make query step
    sqlite3_step(stmt);
    sql_user_id = sqlite3_column_int(stmt, 0);


    // Return true or false
    return sql_user_id;
}

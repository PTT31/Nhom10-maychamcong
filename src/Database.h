#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <SPI.h>
#include <FS.h>
#include "SD.h"

#define USER_DB "/sd/Database/User.db"
class User_if {
public:
    char name[30] = "";  // Sử dụng con trỏ char thay vì const char để có thể sử dụng strdup
    int finger_id;
};
static int callback(void *data, int argc, char **argv, char **azColName);
int openDb(sqlite3 **db);
int db_exec(sqlite3 *db, const char *sql);
int db_query(int data,User_if *user);
int db_insert(uint8_t id,String name,String role);
int db_delete(String data);
#include "Database.h"
// char *zErrMsg = 0;
// const char *data = "Callback function called";
int rc;
// static int callback(void *data, int argc, char **argv, char **azColName)
// {
//     int i;
//     Serial.printf("%s: ", (const char *)data);
//     for (i = 0; i < argc; i++)
//     {
//         Serial.printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
//     }
//     Serial.printf("\n");
//     return 0;
// }
// int openDb(sqlite3 **db)
// {
//     int rc = sqlite3_open(USER_DB, db);
//     if (rc)
//     {
//         Serial.printf("Can't open database: %s\n", sqlite3_errmsg(*db));
//         return rc;
//     }
//     else
//     {
//         Serial.printf("Opened database successfully\n");
//         sqlite3_close(*db);
//     }
//     return rc;
// }
// char *zErrMsg = 0;
//  int db_exec(sqlite3 *db, const char *sql)
//  {
//      Serial.println(sql);
//      long start = micros();
//      int rc = sqlite3_exec(db, sql, callback, (void *)data, &zErrMsg);
//      if (rc != SQLITE_OK)
//      {
//          Serial.printf("SQL error: %s\n", zErrMsg);
//          sqlite3_free(zErrMsg);
//      }
//      else
//      {
//          Serial.printf("Operation done successfully\n");
//      }
//      Serial.print(F("Time taken:"));
//      Serial.println(micros() - start);
//      return rc;
//  }
// tìm user
int db_query(int data, User_if *user)
{
    sqlite3 *db1;
    sqlite3_stmt *res;
    const int bufferSize = 64; // Kích thước tối đa của chuỗi char
    char sqlQuery[bufferSize]; // Mảng char để lưu trữ chuỗi SQL
    snprintf(sqlQuery, bufferSize, "Select * from users where finger_id = '%d'", data);
    sqlite3_open(USER_DB, &db1);
    rc = sqlite3_prepare_v2(db1, sqlQuery, -1, &res, NULL);
    if (rc != SQLITE_OK)
    {
        Serial.println("Dont open database");
        sqlite3_finalize(res); // Cleanup
        sqlite3_close(db1);
        return -1;
    }
    if (sqlite3_step(res) == SQLITE_ROW)
    {
        strcpy(user->name, (const char *)sqlite3_column_text(res, 1));
        user->finger_id = sqlite3_column_int(res, 0);
        Serial.println(user->name);
        sqlite3_finalize(res); // Cleanup
        sqlite3_close(db1);
        return 1;
    }
    else
    {
        Serial.println("No data found");
        sqlite3_finalize(res); // Cleanup
        sqlite3_close(db1);
        return -1;
    }
}
// Thêm user
int db_insert(uint8_t id, String name, String role)
{
    sqlite3 *db1;
    sqlite3_stmt *res;
    String sqlInsert = "INSERT INTO users (finger_id, name, role) VALUES (";
    sqlInsert += id;
    sqlInsert += ", '";
    sqlInsert += name;
    sqlInsert += "', '";
    sqlInsert += role;
    sqlInsert += "')";
    Serial.println(sqlInsert);
    sqlite3_open(USER_DB, &db1);
    rc = sqlite3_prepare_v2(db1, sqlInsert.c_str(), -1, &res, NULL);
    if (rc != SQLITE_OK)
    {
        Serial.println("Dont open database");
        
        sqlite3_finalize(res); // Cleanup
        sqlite3_close(db1);
        return -1;
    }
    sqlite3_finalize(res); // Cleanup
    sqlite3_close(db1);
    return 1;
}
// //xóa user
int db_delete(String data)
{

    // Đọc dữ liệu từ cổng serial
    sqlite3 *db1;
    sqlite3_stmt *res;
    String sqlDel = "delete from users where finger_id = ";
    sqlDel += data + " ;";
    Serial.println(sqlDel);
    sqlite3_open(USER_DB, &db1);
    rc = sqlite3_prepare_v2(db1, sqlDel.c_str(), -1, &res, NULL);
    if (rc != SQLITE_OK)
    {
        Serial.println("Dont open database");
        sqlite3_finalize(res); // Cleanup
        sqlite3_close(db1);
        return -1;
    }
    sqlite3_finalize(res); // Cleanup
    sqlite3_close(db1);
    Serial.println("User with ID " + String(data) + " has been deleted from the database.");
    return 1;
}
uint8_t isIDPresent(String nameValue)
{
    sqlite3 *db;
    sqlite3_stmt *res;
    const int bufferSize = 64; // Kích thước tối đa của chuỗi char
    char sqlQuery[bufferSize];
    snprintf(sqlQuery, bufferSize, "SELECT * FROM users WHERE name = '%s'", nameValue.c_str());

    rc = sqlite3_prepare_v2(db, sqlQuery, -1, &res, NULL);
    if (rc != SQLITE_OK)
    {
        Serial.println("Dont open database");
        sqlite3_finalize(res);
        sqlite3_close(db);
        return -1;
    }
    uint8_t id;
    if (sqlite3_step(res) == SQLITE_ROW)
    {
        id = sqlite3_column_int(res, 0);
        sqlite3_finalize(res);
        sqlite3_close(db);
        return id;
    }
    else
    {
        sqlite3_finalize(res);
        sqlite3_close(db);
        return -1;
    }
}
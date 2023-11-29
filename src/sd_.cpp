#include "sd_.h"
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
// int openDb(const char *filename, sqlite3 **db)
// {
//     int rc = sqlite3_open(filename, db);
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
int db_insert(char *id, char *name, char *role)
{
    sqlite3 *db1;
    sqlite3_stmt *res;
    // Đọc dữ liệu từ cổng serial
    const int bufferSize = 64;  // Kích thước tối đa của chuỗi char
    char sqlInsert[bufferSize]; // Mảng char để lưu trữ chuỗi SQL
    // In chuỗi SQL lên Serial Monitor
    snprintf(sqlInsert, bufferSize, "INSERT INTO users (uuid,name,finger_id, role) VALUES (%s,%s, %s, %s)", NULL, id, name, role);
    Serial.println(sqlInsert);
    openDb(USER_DB, &db1);
    rc = sqlite3_prepare_v2(db1, sqlInsert, -1, &res, NULL);
    sqlite3_close(db1);
    return 1;
}
// //xóa user
// int db_delete(char *data){

//         // Đọc dữ liệu từ cổng serial
//         const int bufferSize = 64; // Kích thước tối đa của chuỗi char
//         char sqlQuery[bufferSize]; // Mảng char để lưu trữ chuỗi SQL
//         // In chuỗi SQL lên Serial Monitor
//         snprintf(sqlQuery, bufferSize, "DELETE FROM user WHERE finger_id = %s", data);
//         Serial.println(sqlQuery);
//         openDb(USER_DB, &db1);
//         rc = db_exec(db1, sqlQuery);
//         if (rc != SQLITE_OK)
//         {
//             sqlite3_close(db1);
//             return 0;
//         }
//         sqlite3_close(db1);
//         Serial.println("User with ID " + String(data) + " has been deleted from the database.");
//         return 1;
// }

#ifndef DB_BUILDER_H
#define DB_BUILDER_H

#include <stdlib.h>
#include <string>
#include "sqlite3.h"

/*
 * DB DSC
 *    ID   |    DATE0    |    DATE1    |    DATE2    |
 * 600000  | id_20131217 | id_20131218 | id_20131219 |
 *
 * TAB DSC (id_DATE_i)
 *     B    |    S     |  TARGET_B  | TARGET_S
 * AMOUNT_B | AMOUNT_S |  AMOUNT_TB | AMOUNT_TS
 *
 * Search 1:
 *        : 'AMOUNT_TB' > THRESHOLD for everyday every ID
 *        : 'AMOUNT_TS' > THRESHOLD for everyday every ID
 *        : 'AMOUNT_TS - AMOUNT_TB' > THRESHOLD for everyday every ID
 *
 * Search 2:
 *        : How long does each of them last ?
**/

class DBFilter {
  public:
    friend class DBSearcher;

    DBFilter();
    ~DBFilter();

    bool openOriginDB(std::string& name);
    bool closeOriginDB();
    bool selectElements(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback);

    bool openFilterDB(std::string& name);
    bool closeFilterDB(std::string& name);

    bool createTable1(std::string& DBName, std::string& tableName);
    bool createTable2(std::string& DBName, std::string& tableName);
    bool selectARowByColumns(std::string& DBName, std::string& tableName, std::string& condition, sqlite3_callback* fCallback);
    bool selectAColumnByRows(std::string& DBName, std::string& tableName, std::string& condition, sqlite3_callback* fCallback);

    bool insertElement(std::string& DBName, std::string& tableName, std::string& KeyAndValues, sqlite3_callback fCallback);
    bool deleteElement(std::string& DBName, std::string& tableName, std::string& condition, sqlite3_callback fCallback);
    bool joinTables(std::string& DBName, std::string& srcTableName, std::string& targetTableName, sqlite3_callback fCallback);
    bool updateElement(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback);

    bool insertColumns(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback);
    bool deleteColumns(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback);
    bool updateColumns(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback);

    bool insertRows(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback);
    bool deleteRows(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback);
    bool updateRows(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback);

  private:
    sqlite3* getDBByName(std::string& DBName);
    bool isTableExist(std::string& DBName, std::string& tableName);

  private:
    sqlite3* mOriginDB;
    sqlite3* mFilterDB;
};

#endif

#ifndef DB_BUILDER_H
#define DB_BUILDER_H

#include <stdlib.h>
#include <list>
#include <map>
#include <string>
#include "sqlite3.h"

/*
 * There are more than one Database in this App, and all the operations to
 * these Databases are completed through the DBWrapper.
 * So, the following features should be supported by the Class DBWrapper:
 *   1) A map from database name to its entity, 
 *   2) A static Database, which contains the info of all the active Databases.
 *      And it is used to load the informations fo the existing database at the
 *      start of the App.
 *
**/

class DBWrapper {
  public:
    enum {
      ORIGIN_TABLE = 0,
      FILTER_TABLE,
      FINAL_TABLE
    };
    DBWrapper();
    ~DBWrapper();

    static DBWrapper* getInstance();
    static bool openTable(int32_t typeOfTable, std::string& DBName, std::string& tableName);
    static bool openDB(const std::string& DBName, sqlite3** outDB);
    static bool closeDB(const std::string& DBName);
    static bool beginBatch(sqlite3* db);
    static bool endBatch(sqlite3* db);

    static bool insertElement(std::string& DBName, std::string& tableName, std::string& KeyAndValues, sqlite3_callback fCallback);
    static bool insertElementsInBatch(std::string& DBName, std::string& tableName, std::list<std::string>& values, sqlite3_callback fCallback);
    static bool deleteElement(std::string& DBName, std::string& tableName, std::string& condition, sqlite3_callback fCallback);
    static bool joinTables(std::string& DBName, std::string& srcTableName, std::string& targetTableName, sqlite3_callback fCallback);
    static bool updateElement(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback);

    static bool insertColumns(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback);
    static bool deleteColumns(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback);
    static bool updateColumns(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback);

    static bool insertRows(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback);
    static bool deleteRows(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback);
    static bool updateRows(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback);

  private:
    static sqlite3* getDBByName(const std::string& DBName);
    static bool isTableExist(std::string& DBName, std::string& tableName);
    static std::map<std::string, sqlite3*> mDatabaseMap;
};

#endif

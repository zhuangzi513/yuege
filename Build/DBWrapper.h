#ifndef DB_WRAPPER_H
#define DB_WRAPPER_H

#include <stdlib.h>
#include <list>
#include <map>
#include <string>
#include <vector>
#include "sqlite3.h"

#include "XLSReader.h"
#include "DBFilter.h"

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
      FILTER_TRUNOVER_TABLE,
      FILTER_RESULT_TABLE,
      FILTER_TABLE,
      FINAL_TABLE
    };
    enum OPEN_TABLE_RET {
      FAIL_OPEN_TABLE = 0, // Fail to open table
      SUCC_OPEN_TABLE,     // Success to open table
      ALRD_OPEN_TABLE      // The requesting table is already existing, in which case OriginHelper should return false but Filter return true
    };
    DBWrapper();
    ~DBWrapper();

    static DBWrapper* getInstance();
    static int openTable(int32_t typeOfTable, const std::string& DBName, const std::string& tableName);
    static bool openDB(const std::string& DBName, sqlite3** outDB);
    static bool closeDB(const std::string& DBName);
    static bool beginBatch(sqlite3* db);
    static bool endBatch(sqlite3* db);
    static bool getAllTablesOfDB(const std::string& aDBName, std::list<std::string>& tableNames);

    static bool getSumTurnOverOfTable(const std::string& aDBName, const std::string& tableName, std::vector<double>& outTurnOvers);
    static bool getBankerTurnOverOfTable(const std::string& aDBName, const std::string& tableName, std::vector<double>& outTurnOvers);

    static bool insertElement(std::string& DBName, std::string& tableName, std::string& KeyAndValues, sqlite3_callback fCallback);
    static bool insertElementsInBatch(std::string& DBName, std::string& tableName, std::list<std::string>& values, std::list<XLSReader::XLSElement*>& xlsValues, sqlite3_callback fCallback);
    static bool insertFilterResultsInBatch(const std::string& DBName, const std::string& tableName, std::list<std::string>& values, std::list<DBFilter::BaseResultData>& filterResultValues, sqlite3_callback fCallback);
    static bool deleteElement(std::string& DBName, std::string& tableName, std::string& condition, sqlite3_callback fCallback);
    static bool joinTables(std::string& DBName, std::string& srcTableName, std::string& targetTableName, sqlite3_callback fCallback);
    static bool updateElement(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback);

    static bool insertColumns(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback);
    static bool deleteColumns(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback);
    static bool updateColumns(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback);

    static bool insertRows(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback);
    static bool deleteRows(std::string& DBName, std::string& tableName, std::string& keyColumn, std::list<std::string>& targetRows);
    static bool updateRows(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback);

  private:
    static sqlite3* getDBByName(const std::string& DBName);
    static bool isTableExist(std::string& DBName, std::string& tableName);
    static std::map<std::string, sqlite3*> mDatabaseMap;
};

#endif

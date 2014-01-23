#include "DBWrapper.h"
#include "DBOperations.h"

#include <stdlib.h>
#include <stdio.h>

#define DEFAULT_VALUE_FOR_INT -1
#define ORIGIN_SQLITE_NAME "test.db"

static char* sqlERR = NULL;

//static
std::map<std::string, sqlite3*> DBWrapper::mDatabaseMap;

DBWrapper* DBWrapper::getInstance() {
   return NULL;
}

DBWrapper::DBWrapper() {
   std::string test("");
}

DBWrapper::~DBWrapper() {
}

bool DBWrapper::createDB(const std::string& DBName, sqlite3** ppDB) {
    int ret = DEFAULT_VALUE_FOR_INT;
    ret = sqlite3_open(DBName.c_str(), ppDB);
    printf("ret:%d\n", ret);
    if (ret == SQLITE_OK) {
        //FIXME: The map should not be DBName:DBName
        printf("push key:%s db:%p\n", DBName.c_str(), *ppDB);
        mDatabaseMap.insert(
            std::map<std::string, sqlite3*>::value_type(DBName, *ppDB));
        return true;
    }
    return false;
}

bool DBWrapper::createTable(int32_t typeOfTable, std::string& DBName, std::string& tableName) {
    int ret = DEFAULT_VALUE_FOR_INT;
    std::string tableFormat;

    switch(typeOfTable) {
      case ORIGIN_TABLE: {
        tableFormat = TABLE_FORMAT_ORIGIN;
        break;
      }
      case FILTER_TABLE: {
        break;
      }
      case FINAL_TABLE: {
        break;
      }

      default:
        //LOG ERR
        printf("Unknown table type\n");
        return false;
    }

    sqlite3* targetDB = getDBByName(DBName);
    if (!targetDB && !createDB(DBName, &targetDB)) {
        printf("Fail to get DB:%s\n", DBName.c_str());
        return false;
    }

    //{
    //   //FIXME: dbInfo is DBName for now
    //   std::string dbInfo = getDBByName(DBName);

    //   if (dbInfo.empty() && !createDB(DBName, &targetDB)) {
    //       printf("Fail to get DB:%s\n", DBName.c_str());
    //       return false;
    //   }
    //   if (SQLITE_OK != sqlite3_open(dbInfo.c_str(), &targetDB)) {
    //       return false;
    //   }
    //}

    std::string sql(CREATE_TABLE + tableName + tableFormat);
    printf("sql:%s\n", sql.c_str());
    ret = sqlite3_exec(targetDB, sql.c_str(), NULL, NULL, &sqlERR);
    if (ret != SQLITE_OK) {
        std::string error(sqlERR);
        if (error.find("already exists")) {
            printf("Table:%s exists in DB:%s\n", tableName.c_str(), DBName.c_str());
            printf("DB Error :%s\n", error.c_str());
            //sqlite3_close(targetDB);
            return true;
        }
        printf("Fail to create Table:%s DB:%s\n", tableName.c_str(), DBName.c_str());
        //sqlite3_close(targetDB);
        return false;
    }
    //sqlite3_close(targetDB);

    return true;
}

bool DBWrapper::insertElement(std::string& DBName, std::string& tableName, std::string& KeyAndValues, sqlite3_callback fCallback) {
    int ret = DEFAULT_VALUE_FOR_INT;
    std::string sql("");
    sqlite3* targetDB = NULL;
    //{
    //    std::string dbInfo = getDBByName(DBName);

    //    if (SQLITE_OK != sqlite3_open(dbInfo.c_str(), &targetDB)) {
    //        printf("open DB:%s fail \n", dbInfo.c_str());
    //        return false;
    //    }

    //    if (targetDB == NULL) {
    //        return false;
    //    }
    //}

    sql = INSERT_OP + tableName + KeyAndValues;
    targetDB = getDBByName(DBName);
    //printf("targetDB:%p, sql:%s\n", targetDB, sql.c_str());
    ret = sqlite3_exec(targetDB, sql.c_str(), *fCallback, NULL, &sqlERR);
    if (ret != SQLITE_OK) {
        printf("sqlErr:%s\n", sqlERR);
        return false;
    }

    return true;
}

bool DBWrapper::deleteElement(std::string& DBName, std::string& tableName, std::string& condition, sqlite3_callback fCallback) {
    int ret = DEFAULT_VALUE_FOR_INT;
    std::string sql("");
    sqlite3* targetDB = NULL;
    //{
    //    std::string dbInfo = getDBByName(DBName);

    //    if (SQLITE_OK != sqlite3_open(dbInfo.c_str(), &targetDB)) {
    //        return false;
    //    }

    //    if (targetDB == NULL) {
    //        return false;
    //    }
    //}
 
    targetDB = getDBByName(DBName);
    sql = DELETE_OP + tableName + condition;
    ret = sqlite3_exec(targetDB, sql.c_str(), *fCallback, NULL, &sqlERR);
    if (ret != SQLITE_OK) {
        return false;
    }
 
    return true;
}

bool DBWrapper::updateElement(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback) {
    return true;
}

bool DBWrapper::joinTables(std::string& DBName, std::string& srcTableName, std::string& targetTableName, sqlite3_callback fCallback) {
    int ret = DEFAULT_VALUE_FOR_INT;
    std::string sql("");
    sqlite3* targetDB = NULL;
    //{
    //    std::string dbInfo = getDBByName(DBName);

    //    if (SQLITE_OK != sqlite3_open(dbInfo.c_str(), &targetDB)) {
    //        return false;
    //    }

    //    if (targetDB == NULL) {
    //        return false;
    //    }
    //}
    targetDB = getDBByName(DBName);

    if (!DBWrapper::isTableExist(DBName, srcTableName) ||
        !DBWrapper::isTableExist(DBName, targetTableName)) {
        printf("Table doesn't exist\n");
        return false;
    }

    //sql = "SELECT * FROM " + srcTableName + " INNER JOIN " + targetTableName + " ON " + srcTableName + ".StockID == " + targetTableName + ".StockID";
    //printf("sql:%s\n", sql.c_str());
    //ret = sqlite3_exec(targetDB, sql.c_str(), *fCallback, NULL, &sqlERR);

    sql = "SELECT * INTO " + srcTableName + " FROM " + targetTableName;
    printf("sql:%s\n", sql.c_str());
    ret = sqlite3_exec(targetDB, sql.c_str(), *fCallback, NULL, &sqlERR);
    if (ret != SQLITE_OK) {
        printf("sqlERR:%s\n", sqlERR);
        return false;
    }

    return true;
}

bool DBWrapper::insertColumns(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback) {
    return true;
}

bool DBWrapper::deleteColumns(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback) {
    return true;
}

bool DBWrapper::updateColumns(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback) {
    return true;
}

bool DBWrapper::insertRows(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback) {
    return true;
}

bool DBWrapper::deleteRows(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback) {
    return true;
}

bool DBWrapper::updateRows(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback) {
    return true;
}


//=======private
sqlite3* DBWrapper::getDBByName(std::string& DBName) {
    std::map<std::string, sqlite3*>::iterator itr = mDatabaseMap.find(DBName);
    sqlite3* ret = itr->second;
    //printf("DBName:%s, DB:%p\n", DBName.c_str(), ret);
    return ret;
}

bool DBWrapper::isTableExist(std::string& DBName, std::string& tableName) {
    return true;
}

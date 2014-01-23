#include "DBFilter.h"
#include "DBOperations.h"

#include <stdlib.h>
#include <stdio.h>

#define DEFAULT_VALUE_FOR_INT -1
#define ORIGIN_SQLITE_NAME "test.db"

static char* sqlERR = NULL;

DBFilter::DBFilter()
         : mOriginDB(NULL)
         , mFilterDB(NULL) {
   std::string test("");
}

DBFilter::~DBFilter() {
    if (mOriginDB) {
        delete mOriginDB;
        mOriginDB = NULL;
    }

    if (mFilterDB) {
        delete mFilterDB;
        mFilterDB = NULL;
    }
}

bool DBFilter::openOriginDB(std::string& name) {
    int ret = DEFAULT_VALUE_FOR_INT;
    ret = sqlite3_open(ORIGIN_SQLITE_NAME, &mOriginDB);
    return (ret == SQLITE_OK) ? true : false;
}

bool DBFilter::closeOriginDB() {
    int ret = DEFAULT_VALUE_FOR_INT;
    ret = sqlite3_close(mOriginDB);
    return (ret == SQLITE_OK) ? true : false;
}

bool DBFilter::openFilterDB(std::string& name) {
    //CHECK name
    int ret = DEFAULT_VALUE_FOR_INT;
    ret = sqlite3_open(name.c_str(), &mFilterDB);
    if (ret != SQLITE_OK) {
        printf("Fail to open DB:%s\n", name.c_str());
        return false;
    }
    //sqlite3_close(mFilterDB);

    return true;
}

bool DBFilter::closeFilterDB(std::string& name) {
    //CHECK name
    int ret = DEFAULT_VALUE_FOR_INT;
    ret = sqlite3_close(mFilterDB);
    return (ret == SQLITE_OK) ? true : false;
}

bool DBFilter::selectElements(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback) {
    int ret = DEFAULT_VALUE_FOR_INT;
    sqlite3* targetDB = getDBByName(DBName);
    ret = sqlite3_exec(targetDB, description.c_str(), *fCallback, NULL, &sqlERR);
    if (ret != SQLITE_OK) {
        return false;
    }

    return true;
}

bool DBFilter::createTable1(std::string& DBName, std::string& tableName) {
    int ret = DEFAULT_VALUE_FOR_INT;
    std::string sql(CREATE_TABLE + tableName + TABLE_FORMAT2);
    sqlite3* targetDB = getDBByName(DBName);

    if (targetDB == NULL) {
        printf("Fail to get DB:%s\n", DBName.c_str());
        return false;
    }

    printf("sql:%s\n", sql.c_str());
    ret = sqlite3_exec(targetDB, sql.c_str(), NULL, NULL, &sqlERR);
    if (ret != SQLITE_OK) {
        std::string error(sqlERR);
        if (error.find("already exists")) {
            printf("Table:%s exists in DB:%s\n", tableName.c_str(), DBName.c_str());
            return true;
        }
        printf("Fail to create Table:%s DB:%s\n", tableName.c_str(), DBName.c_str());
        return false;
    }

    return true;
}

bool DBFilter::createTable2(std::string& DBName, std::string& tableName) {
    int ret = DEFAULT_VALUE_FOR_INT;
    std::string sql(CREATE_TABLE + tableName + TABLE_FORMAT2);
    sqlite3* targetDB = getDBByName(DBName);

    if (targetDB == NULL) {
        printf("Fail to get DB:%s\n", DBName.c_str());
        return false;
    }

    printf("sql:%s\n", sql.c_str());
    ret = sqlite3_exec(targetDB, sql.c_str(), NULL, NULL, &sqlERR);
    if (ret != SQLITE_OK) {
        std::string error(sqlERR);
        if (error.find("already exists")) {
            printf("Table:%s exists in DB:%s\n", tableName.c_str(), DBName.c_str());
            return true;
        }
        printf("Fail to create Table:%s DB:%s\n", tableName.c_str(), DBName.c_str());
        return false;
    }

    return true;
}

bool DBFilter::selectARowByColumns(std::string& DBName, std::string& tableName, std::string& condition, sqlite3_callback* fCallback) {
    return true;
}

bool DBFilter::selectAColumnByRows(std::string& DBName, std::string& tableName, std::string& condition, sqlite3_callback* fCallback) {
    return true;
}

bool DBFilter::insertElement(std::string& DBName, std::string& tableName, std::string& KeyAndValues, sqlite3_callback fCallback) {
    int ret = DEFAULT_VALUE_FOR_INT;
    std::string sql("");
    sqlite3* targetDB = getDBByName(DBName);

    if (targetDB == NULL) {
        return false;
    }

    sql = INSERT_OP + tableName + KeyAndValues;
    ret = sqlite3_exec(targetDB, sql.c_str(), *fCallback, NULL, &sqlERR);
    if (ret != SQLITE_OK) {
        printf("sqlErr:%s\n", sqlERR);
        return false;
    }

    return true;
}

bool DBFilter::deleteElement(std::string& DBName, std::string& tableName, std::string& condition, sqlite3_callback fCallback) {
    int ret = DEFAULT_VALUE_FOR_INT;
    std::string sql("");
    sqlite3* targetDB = getDBByName(DBName);
 
    if (targetDB == NULL) {
        return false;
    }
 
    sql = DELETE_OP + tableName + condition;
    ret = sqlite3_exec(targetDB, sql.c_str(), *fCallback, NULL, &sqlERR);
    if (ret != SQLITE_OK) {
        return false;
    }
 
    return true;
}

bool DBFilter::updateElement(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback) {
    return true;
}

bool DBFilter::joinTables(std::string& DBName, std::string& srcTableName, std::string& targetTableName, sqlite3_callback fCallback) {
    int ret = DEFAULT_VALUE_FOR_INT;
    std::string sql("");

    sqlite3* targetDB = getDBByName(DBName);
    if (!targetDB) {
        printf("DB doesn't exist\n");
        return false;
    }

    if (!isTableExist(DBName, srcTableName) ||
        !isTableExist(DBName, targetTableName)) {
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

bool DBFilter::insertColumns(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback) {
    return true;
}

bool DBFilter::deleteColumns(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback) {
    return true;
}

bool DBFilter::updateColumns(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback) {
    return true;
}

bool DBFilter::insertRows(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback) {
    return true;
}

bool DBFilter::deleteRows(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback) {
    return true;
}

bool DBFilter::updateRows(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback) {
    return true;
}


//=======private
sqlite3* DBFilter::getDBByName(std::string& DBName) {
    //JUST FOR NOW
    return mFilterDB;
}

bool DBFilter::isTableExist(std::string& DBName, std::string& tableName) {
    return true;
}

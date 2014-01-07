#include <stdlib.h>
#include <stdio.h>




#include "DBBuilder.h"
#include "DBOperations.h"

#define DEFAULT_VALUE_FOR_INT -1
#define ORIGIN_SQLITE_NAME "test.db"

static char* sqlERR = NULL;

DBBuilder::DBBuilder()
         : mOriginDB(NULL)
         , mFilterDB(NULL) {
   std::string test("");
}

DBBuilder::~DBBuilder() {
    if (mOriginDB) {
        delete mOriginDB;
        mOriginDB = NULL;
    }

    if (mFilterDB) {
        delete mFilterDB;
        mFilterDB = NULL;
    }
}

bool DBBuilder::openOriginDB(std::string& name) {
    int ret = DEFAULT_VALUE_FOR_INT;
    ret = sqlite3_open(ORIGIN_SQLITE_NAME, &mOriginDB);
    return (ret == SQLITE_OK) ? true : false;
}

bool DBBuilder::closeOriginDB() {
    int ret = DEFAULT_VALUE_FOR_INT;
    ret = sqlite3_close(mOriginDB);
    return (ret == SQLITE_OK) ? true : false;
}

bool DBBuilder::openFilterDB(std::string& name) {
    //CHECK name
    int ret = DEFAULT_VALUE_FOR_INT;
    ret = sqlite3_open(name.c_str(), &mFilterDB);
    if (ret != SQLITE_OK) {
        printf("Fail to open DB:%s\n", name.c_str());
        return false;
    }

    return true;
}

bool DBBuilder::closeFilterDB(std::string& name) {
    //CHECK name
    int ret = DEFAULT_VALUE_FOR_INT;
    ret = sqlite3_close(mFilterDB);
    return (ret == SQLITE_OK) ? true : false;
}

bool DBBuilder::selectElements(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback) {
    int ret = DEFAULT_VALUE_FOR_INT;
    sqlite3* targetDB = getDBByName(DBName);
    ret = sqlite3_exec(targetDB, description.c_str(), *fCallback, NULL, &sqlERR);
    if (ret != SQLITE_OK) {
        return false;
    }

    return true;
}

bool DBBuilder::createTable1(std::string& DBName, std::string& tableName) {
    int ret = DEFAULT_VALUE_FOR_INT;
    std::string sql(CREATE_TABLE + tableName + TABLE_FORMAT1);
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

bool DBBuilder::createTable2(std::string& DBName, std::string& tableName) {
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

bool DBBuilder::selectARowByColumns(std::string& DBName, std::string& tableName, std::string& condition, sqlite3_callback* fCallback) {
    return true;
}

bool DBBuilder::selectAColumnByRows(std::string& DBName, std::string& tableName, std::string& condition, sqlite3_callback* fCallback) {
    return true;
}

bool DBBuilder::insertElement(std::string& DBName, std::string& tableName, std::string& KeyAndValues, sqlite3_callback fCallback) {
    int ret = DEFAULT_VALUE_FOR_INT;
    std::string sql("");
    sqlite3* targetDB = getDBByName(DBName);

    if (targetDB == NULL) {
        return false;
    }

    sql = INSERT_OP + tableName + KeyAndValues;
    printf("sql:%s\n", sql.c_str());
    ret = sqlite3_exec(targetDB, sql.c_str(), *fCallback, NULL, &sqlERR);
    if (ret != SQLITE_OK) {
        printf("sqlErr:%s\n", sqlERR);
        return false;
    }

    return true;
}

bool DBBuilder::deleteElement(std::string& DBName, std::string& tableName, std::string& condition, sqlite3_callback fCallback) {
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

bool DBBuilder::updateElement(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback) {
    return true;
}

bool DBBuilder::joinTables(std::string& DBName, std::string& srcTableName, std::string& targetTableName, sqlite3_callback fCallback) {
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

bool DBBuilder::insertColumns(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback) {
    return true;
}

bool DBBuilder::deleteColumns(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback) {
    return true;
}

bool DBBuilder::updateColumns(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback) {
    return true;
}

bool DBBuilder::insertRows(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback) {
    return true;
}

bool DBBuilder::deleteRows(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback) {
    return true;
}

bool DBBuilder::updateRows(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback) {
    return true;
}


//=======private
sqlite3* DBBuilder::getDBByName(std::string& DBName) {
    //JUST FOR NOW
    return mFilterDB;
}

bool DBBuilder::isTableExist(std::string& DBName, std::string& tableName) {
    return true;
}

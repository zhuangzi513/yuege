#include "DBFilter.h"

#include "DBOperations.h"
#include "DBWrapper.h"
#include "ErrorDefines.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#define LOGTAG   "DBFilter"
#define DEFAULT_VALUE_FOR_INT -1
#define TURNOVER "TurnOver"

static char* sqlERR = NULL;

sqlite3* DBFilter::mOriginDB = NULL;
std::list<std::string> DBFilter::mTableNames;
std::string DBFilter::mResultDBName            = "";
std::string DBFilter::mBigSaleTableName        = "BigSaleTurnOver";
std::string DBFilter::mBigSalePriceTableName   = "BigSalePrice";
std::string DBFilter::mBigBuyTableName         = "BigBuyTurnOver";
std::string DBFilter::mBigBuyPriceTableName    = "BigBuyPrice";
std::string DBFilter::mDiffBigBuySaleTableName = "";


static std::string SELECT_BIG_SALE_INTO(const std::string& srcTable,
                                        const std::string& targetTable,
                                        const std::string& columnName,
                                        const std::string& arg) {
    std::string command("");
    command += " INSERT INTO ";
    command += targetTable;
    command += " SELECT ";
    command += columnName;
    command += " FROM ";
    command += srcTable;
    command += " WHERE ";
    command += columnName;
    command += " >= ";
    command += arg;
    return command;
}

static std::string SELECT_IN(const std::string& srcTable, const std::string& targetDBName) {
    std::string command("");
    command += " SELECT * INTO ";
    command += srcTable;
    command += " IN ";
    command += targetDBName;
    return command;
}

static std::string GET_TABLES() {
    std::string command("");
    command += " SELECT name FROM ";
    command += " sqlite_master ";
    command += " WHERE TYPE=\"table\"";
    command += " ORDER BY name ";
    return command;
}

DBFilter::DBFilter() {
   std::string test("");
}

DBFilter::~DBFilter() {
    if (mOriginDB) {
        delete mOriginDB;
        mOriginDB = NULL;
    }
}

bool DBFilter::filterOriginDBByTurnOver(const std::string& aDBName, const int aMinTurnover, const int aMaxTurnOver) {
    LOGI(LOGTAG, "DBName:%s", aDBName.c_str());
    if (!openOriginDB(aDBName)) {
        LOGI(LOGTAG, "Fail to open db:%s", aDBName.c_str());
        return false;
    }

    //Step 1: get all the tables in the OriginDB
    if (!getAllTablesOfDB(aDBName)) {
        LOGI(LOGTAG, "Failt to get tables from :%s", aDBName.c_str());
        return false;
    }

    //Step 2: Filter all the tables of the OriginDB and save them in a 'tmp table'
    if (!filterAllTablesByTurnOver(aDBName, aMinTurnover)) {
        LOGI(LOGTAG, "Fail to filter tables of :%s", aDBName.c_str());
        return false;
    }

    closeOriginDB(aDBName);
    return true;
}

//=======private
bool DBFilter::openOriginDB(const std::string& name) {
    return DBWrapper::openDB(name.c_str(), &mOriginDB);
}

bool DBFilter::closeOriginDB(const std::string& aDBName) {
    int ret = DEFAULT_VALUE_FOR_INT;
    ret = DBWrapper::closeDB(aDBName);
    return (ret == SQLITE_OK) ? true : false;
}

sqlite3* DBFilter::getDBByName(const std::string& DBName) {
    //JUST FOR NOW
    return NULL;
}

bool DBFilter::isTableExist(const std::string& DBName, const std::string& tableName) {
    return true;
}

bool DBFilter::getAllTablesOfDB(const std::string& aDBName) {
    std::string sql = GET_TABLES();
    //LOG sql
    sqlite3_stmt* stmt = NULL;
    int ret = -1;

    ret = sqlite3_prepare(mOriginDB,
                          sql.c_str(),
                          -1,
                          &stmt,
                          NULL);
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "Fail to prepare stmt to retrieve all the tables in:%s, errno:%d", aDBName.c_str(), errno);
        return false;
    }

    while (true) {
        ret = sqlite3_step(stmt);
        if (ret == SQLITE_ROW) {
            std::string tableName = (char*)sqlite3_column_text(stmt, 0);
            mTableNames.push_back(tableName);
        } else if (ret == SQLITE_DONE) {
            LOGI(LOGTAG, "DONE, value:%d", ret);
            break;
        } else {
            LOGI(LOGTAG, "OTHER, value:%d", ret);
            break;
        }
    }

    if (SQLITE_OK != sqlite3_finalize(stmt)) {
        LOGI(LOGTAG, "Fail to finalize the stmt to retrieve tables in DB:%s", aDBName.c_str());
        return false;
    }
    return true;
}

bool DBFilter::fileterTableByTurnOver(const std::string& tableName, const int aMinTurnOver) {
    LOGI(LOGTAG, "filterTableByTurnOver for table:%s", tableName.c_str());
    char strTurnOver[8] = {0};

    if (aMinTurnOver < 100000) {
        sprintf(strTurnOver, "%d", aMinTurnOver);
    } else {
        LOGI(LOGTAG, "overflow of aMinTurnOver:%d", aMinTurnOver);
        return false;
    }

    std::string sql = SELECT_BIG_SALE_INTO(tableName, mBigSaleTableName, TURNOVER, strTurnOver);
    LOGI(LOGTAG, "sql:%s", sql.c_str());
    return true;
}

bool DBFilter::filterAllTablesByTurnOver(const std::string& aDBName, const int aMinTurnover) {
    DBWrapper::openTable(DBWrapper::FILTER_SALE_TRUNOVER_TABLE, aDBName, mBigSaleTableName);
    std::list<std::string>::iterator iterOfTableName;
    for (iterOfTableName = mTableNames.begin(); iterOfTableName != mTableNames.end(); iterOfTableName++) {
         if (!fileterTableByTurnOver(*iterOfTableName, aMinTurnover)) {
             return false;
         }
    }
    return true;
}


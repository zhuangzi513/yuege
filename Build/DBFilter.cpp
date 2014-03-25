#include "DBFilter.h"

#include "DBOperations.h"
#include "DBWrapper.h"
#include "ErrorDefines.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#define LOGTAG   "DBFilter"
#define DEFAULT_VALUE_FOR_INT -1

#define VOLUME   " Volume "
#define TURNOVER " TurnOver "
#define SALE_BUY " SaleBuy "

static char* sqlERR = NULL;

sqlite3* DBFilter::mOriginDB = NULL;
std::list<std::string> DBFilter::mTableNames;
std::string DBFilter::mResultTableName         = "FilterResult";
std::string DBFilter::mTmpResultTableName      = "MiddleWareTable";
std::string DBFilter::mDiffBigBuySaleTableName = "";


static std::string SELECT_TURNOVER_INTO(const std::string& srcTable,
                                        const std::string& targetTable,
                                        const std::string& columnNames,
                                        const std::string& arg) {
    std::string command("");
    command += " INSERT INTO ";
    command += targetTable;
    command += " SELECT ";
    command += columnNames;
    command += " FROM ";
    command += srcTable;
    command += " WHERE ";
    command += TURNOVER;
    command += " >= ";
    command += arg;
    return command;
}

static std::string SUM_FILTER_RESULT_TABLE(const std::string& srcTable,
                                           const std::string& columnNames,
                                           const std::string& arg) {
    std::string command("");
    command += " SELECT ";
    command += columnNames;
    command += " FROM ";
    command += srcTable;
    command += " GROUP BY ";
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

static std::string DELETE_FROM_TABLE(const std::string& srcTable) {
    std::string command("");
    command += " DELETE FROM ";
    command += srcTable;

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

bool DBFilter::computeResultFromTable(const std::string& aDBName, const std::string& tableName) {
    //FIXME:MiddleWare and it will be merged to other function
    //FIXME: the 'tableName" MUST same to mTmpResultTableName
    int ret = 0;
    sqlite3_stmt* stmt = NULL;
    std::string sql;
    std::string columns("");
    std::string keyColumn("");

    columns += " SUM(Volume),";
    columns += " SUM(TurnOver), ";
    columns += SALE_BUY;

    keyColumn += SALE_BUY;

    sql = SUM_FILTER_RESULT_TABLE(tableName, columns, keyColumn);

    ret = sqlite3_prepare(mOriginDB,
                          sql.c_str(),
                          -1,
                          &stmt,
                          NULL);
    if (ret != SQLITE_OK) {
        return false;
    }

    {
        //FIXME: The first raw is Buy, second raw is Sale
        int saleTurnOver = 0;
        int buyTurnOver = 0;
        double avgSalePrice = 0.0;
        double avgBuyPrice = 0.0;

        while(sqlite3_step(stmt) == SQLITE_ROW) {
            std::string sale_buy = (char*)sqlite3_column_text(stmt, 2);
            LOGI(LOGTAG, "%s", sale_buy.c_str());

            if (sale_buy == std::string("true")) {
                saleTurnOver = sqlite3_column_int(stmt, 1);
                saleTurnOver = sqlite3_column_double(stmt, 2);
            } else if (sale_buy == std::string("false")) {
                buyTurnOver = sqlite3_column_int(stmt, 1);
                buyTurnOver = sqlite3_column_double(stmt, 2);
            } else {
                // Not buy, not sale, just a normal.
                // The sale_buy should be empty
                if (sale_buy.empty()) {
                    LOGI(LOGTAG, "NON-BUY-SALE");
                    continue;
                }
                LOGI(LOGTAG, "%s", sale_buy.c_str());
                return false;
            }
        }

        //saveResultToResultTable(aDBName, mResultTableName, saleTurnOver, avgSalePrice, buyTurnOver, avgBuyPrice);
    }

    ret = sqlite3_finalize(stmt);
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "Fail to finalize the stmt to finalize table:%s", tableName.c_str());
        return false;
    }

    return true;
}

bool DBFilter::saveResultToResultTable(const std::string& aDBName,
                                       const std::string& tableName,
                                       const int turnoverSale,
                                       const float avgPriceSale,
                                       const int turnoverBuy,
                                       const float avgPriceBuy) {
    //FIXME: the 'tableName" MUST same to mResultTableName
    DBWrapper::openTable(DBWrapper::FILTER_RESULT_TABLE, aDBName, tableName);

    int ret = 0;
    sqlite3_stmt* stmt = NULL;
    std::string sql;
    std::string columns("");
    std::string keyColumn("");

    return true;
}

bool DBFilter::clearTable(const std::string& tableName) {
    int ret = 0;
    std::string sql("");
    sqlite3_stmt* stmt = NULL;

    sql = DELETE_FROM_TABLE(tableName); 

    ret = sqlite3_prepare(mOriginDB,
                          sql.c_str(),
                          -1,
                          &stmt,
                          NULL);

    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "Fail to prepare stmt to clearTable:%s", tableName.c_str());
        return false;
    }

    ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE) {
        return false;
    }

    ret = sqlite3_finalize(stmt);
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "Fail to finalize the stmt to finalize table:%s", tableName.c_str());
        return false;
    }

    return true;
}

bool DBFilter::filterTableByTurnOver(const std::string& tableName, const int aMinTurnOver) {
    LOGI(LOGTAG, "filterTableByTurnOver for table:%s", tableName.c_str());
    char strTurnOver[8] = {0};

    if (aMinTurnOver < 100000) {
        sprintf(strTurnOver, "%d", aMinTurnOver);
    } else {
        LOGI(LOGTAG, "overflow of aMinTurnOver:%d", aMinTurnOver);
        return false;
    }

    sqlite3_stmt* stmt = NULL;
    int ret = -1;
    std::string targetColumns = "";
    targetColumns += VOLUME;
    targetColumns += ",";
    targetColumns += TURNOVER;
    targetColumns += ",";
    targetColumns += SALE_BUY;

    std::string sql = SELECT_TURNOVER_INTO(tableName, mTmpResultTableName, targetColumns, strTurnOver);
    LOGI(LOGTAG, "sql:%s", sql.c_str());

    ret = sqlite3_prepare(mOriginDB,
                          sql.c_str(),
                          -1,
                          &stmt,
                          NULL);

    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "Fail to prepare stmt to filterTable:%s", tableName.c_str());
        return false;
    }

    ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE) {
        return false;
    }

    ret = sqlite3_finalize(stmt);
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "Fail to finalize the stmt to finalize table:%s", tableName.c_str());
        return false;
    }

    return true;
}

bool DBFilter::filterAllTablesByTurnOver(const std::string& aDBName, const int aMinTurnover) {
    //FIXME: Assuming that process all the tables in one DB and then the tables of the other DB.
    DBWrapper::openTable(DBWrapper::FILTER_TRUNOVER_TABLE, aDBName, mTmpResultTableName);
    std::list<std::string>::iterator iterOfTableName;
    for (iterOfTableName = mTableNames.begin(); iterOfTableName != mTableNames.end(); iterOfTableName++) {
         if ((*iterOfTableName) != (mResultTableName) &&
             (*iterOfTableName) != (mTmpResultTableName)) {
             //Step 1: filter
             if (!filterTableByTurnOver(*iterOfTableName, aMinTurnover)) {
                 return false;
             }

             //Step 2: compute && save result 
             if (!computeResultFromTable(aDBName, mTmpResultTableName)) {
                 return false;
             }

             ////Step 3: save to final result 
             //if (!saveResultToResultTable(aDBName, mResultTableName)) {
             //    return false;
             //}

             if (!clearTable(mTmpResultTableName)) {
                 return false;
             }
         }
    }
    DBWrapper::closeDB(aDBName);
    return true;
}


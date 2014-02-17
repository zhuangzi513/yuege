#include "DBFilter.h"
#include "DBOperations.h"
#include "ErrorDefines.h"

#include <stdlib.h>
#include <stdio.h>

#define LOGTAG   "DBFilter"
#define DEFAULT_VALUE_FOR_INT -1
#define ORIGIN_SQLITE_NAME "test.db"

static char* sqlERR = NULL;

sqlite3* DBFilter::mOriginDB = NULL;
std::string DBFilter::mResultDBName            = "";
std::string DBFilter::mBigSaleTableName        = "";
std::string DBFilter::mBigSalePriceTableName   = "";
std::string DBFilter::mBigBuyTableName         = "";
std::string DBFilter::mBigBuyPriceTableName    = "";
std::string DBFilter::mDiffBigBuySaleTableName = "";

static std::string SELECT_INTO(const std::string& srcTable, const std::string& targetTable) {
    std::string command("");
    command += " SELECT * INTO ";
    command += targetTable;
    command += " FROM ";
    command += srcTable;
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
    command += " SELECT * FROM ";
    command += " table_master ";
    command += " WHERE type=\"table\"";
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

bool DBFilter::filterOriginDBByTurnOver(const std::string& aDBName, int aMinTurnover, int aMaxTurnOver) {
    if (!openOriginDB(aDBName)) {
        //LOG ERR
        return false;
    }

    //Step 1: get all the tables in the OriginDB
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
        //LOG ERR
        return false;
    }

    while (SQLITE_ROW == sqlite3_step(stmt)) {
        int column_type = sqlite3_column_type(stmt, 0);
        //std::string tableName = sqlite3_column_text(stmt, 0)
        LOGI(LOGTAG, "column_type:%d", column_type);
    }

    return (SQLITE_OK == sqlite3_finalize(stmt));

    //Step 2: Filter all the tables of the OriginDB and save them in a 'tmp table'

    //Step 3: Retrieve the info we want from the 'tmp table.

    //Step 4: Save the info(es) into the corresponding table in the resultDB
}

//=======private
bool DBFilter::openOriginDB(const std::string& name) {
    int ret = DEFAULT_VALUE_FOR_INT;
    ret = sqlite3_open(ORIGIN_SQLITE_NAME, &mOriginDB);
    return (ret == SQLITE_OK) ? true : false;
}

bool DBFilter::closeOriginDB() {
    int ret = DEFAULT_VALUE_FOR_INT;
    ret = sqlite3_close(mOriginDB);
    return (ret == SQLITE_OK) ? true : false;
}

sqlite3* DBFilter::getDBByName(const std::string& DBName) {
    //JUST FOR NOW
    return NULL;
}

bool DBFilter::isTableExist(const std::string& DBName, const std::string& tableName) {
    return true;
}

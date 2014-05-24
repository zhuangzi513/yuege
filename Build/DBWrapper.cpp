#include "DBWrapper.h"
#include "DBOperations.h"
#include "ErrorDefines.h"

#include <stdlib.h>
#include <stdio.h>

#include <errno.h>

#define LOGTAG "DBWrapperSqlite3"

#define DEFAULT_VALUE_FOR_INT -1
#define ORIGIN_SQLITE_NAME "test.db"

static char* sqlERR = NULL;

static std::string GET_TABLES() {
    std::string command("");
    command += " SELECT name FROM ";
    command += " sqlite_master ";
    command += " WHERE TYPE=\"table\"";
    command += " ORDER BY name ";
    return command;
}

static std::string DELETE_ROWS_FROM_TABLE(const std::string& srcTable,
                                          const std::string& keyColumn,
                                          const std::string& rowID) {
    std::string command("");
    command += " DELETE FROM ";
    command += srcTable;
    command += " WHERE ";
    command += keyColumn;
    command += " IN ( ";
    command += rowID;
    command += " ) ";

    return command;
}



static bool bindCommand(XLSReader::XLSElement* xlsElement, sqlite3_stmt* stmt) {
    int ret = -1;
    //FIXME: For now, time is not used.
    ret = sqlite3_bind_text(stmt, 1, xlsElement->mTime.c_str(), -1, NULL);
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "bind Time Fail: %s", xlsElement->mTime.c_str());
        return false;
    }
    ret = sqlite3_bind_double(stmt, 2, atof(xlsElement->mPrice.c_str()));
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "bind Price Fail: %s", xlsElement->mPrice.c_str());
        return false;
    }
    ret = sqlite3_bind_double(stmt, 3, atof(xlsElement->mFloat.c_str()));
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "bind Float Fail: %s", xlsElement->mFloat.c_str());
        return false;
    }
    ret = sqlite3_bind_int(stmt, 4, atoi(xlsElement->mVolume.c_str()));
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "bind Volume Fail: %s", xlsElement->mVolume.c_str());
        return false;
    }
    ret = sqlite3_bind_double(stmt, 5, atof(xlsElement->mTurnOver.c_str()));
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "bind TurnOver Fail: %s", xlsElement->mTurnOver.c_str());
        return false;
    }
    ret = sqlite3_bind_text(stmt, 6, xlsElement->mSB.c_str(), -1, NULL);
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "bind SB Fail: %s", xlsElement->mSB.c_str());
        return false;
    }
    return true;
}

static bool bindCommand(const DBFilter::BaseResultData& filterResultElement, sqlite3_stmt* stmt) {
    int ret = -1;
    ret = sqlite3_bind_text(stmt, 1, filterResultElement.mDate.c_str(), -1, NULL);
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "bind Date Fail: %s", filterResultElement.mDate.c_str());
        return false;
    }

    ret = sqlite3_bind_int(stmt, 2, filterResultElement.mSaleVolume);
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "bind SaleVolume Fail: %d", filterResultElement.mSaleVolume);
        return false;
    }

    ret = sqlite3_bind_double(stmt, 3, filterResultElement.mSaleTurnOver);
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "bind SaleTurnOver Fail: %f", filterResultElement.mSaleTurnOver);
        return false;
    }

    ret = sqlite3_bind_double(stmt, 4, filterResultElement.mSalePrice);
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "bind BuyTurnOver Fail: %f", filterResultElement.mSalePrice);
        return false;
    }

    ret = sqlite3_bind_int(stmt, 5, filterResultElement.mBuyVolume);
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "bind BuyVolume Fail: %d", filterResultElement.mBuyVolume);
        return false;
    }

    ret = sqlite3_bind_double(stmt, 6, filterResultElement.mBuyTurnOver);
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "bind BuyTurnOver Fail: %f", filterResultElement.mBuyTurnOver);
        return false;
    }

    ret = sqlite3_bind_double(stmt, 7, filterResultElement.mBuyPrice);
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "bind BuyTurnOver Fail: %f", filterResultElement.mBuyPrice);
        return false;
    }

    ret = sqlite3_bind_double(stmt, 8, filterResultElement.mPureFlowInOneDay);
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "bind FlowInOneDay Fail: %f", filterResultElement.mPureFlowInOneDay);
        return false;
    }

    ret = sqlite3_bind_double(stmt, 9, filterResultElement.mSumFlowInTenDays);
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "bind SumFlowInTenDays Fail: %f", filterResultElement.mSumFlowInTenDays);
        return false;
    }

    ret = sqlite3_bind_double(stmt, 10, filterResultElement.mBeginPrice);
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "bind SumFlowInTenDays Fail: %f", filterResultElement.mBeginPrice);
        return false;
    }

    ret = sqlite3_bind_double(stmt, 11, filterResultElement.mEndPrice);
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "bind SumFlowInTenDays Fail: %f", filterResultElement.mEndPrice);
        return false;
    }

    return true;
}

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

bool DBWrapper::openDB(const std::string& DBName, sqlite3** ppDB) {
    errno = 0;
    sqlite3* targetDB = getDBByName(DBName);
    if (targetDB != NULL) {
        *ppDB = targetDB;
        return true;
    }
    int ret = DEFAULT_VALUE_FOR_INT;
    errno = 0;
    ret = sqlite3_open(DBName.c_str(), ppDB);
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "Fail to open Database with the name of %s, errno:%d", DBName.c_str(), errno);
        exit(1);
        return false;
    }

    //FIXME: The map should not be DBName:DBName
    mDatabaseMap.insert(
        std::map<std::string, sqlite3*>::value_type(DBName, *ppDB));
    return true;
}

bool DBWrapper::closeDB(const std::string& DBName) {
    errno = 0;
    sqlite3* targetDB = getDBByName(DBName);
    if (targetDB == NULL) {
        LOGI(LOGTAG, "The Database with the name of %s, has been closed, errno:%d", DBName.c_str(), errno);
        return true;
    }
    int ret = sqlite3_close(targetDB);
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "Fail to close Database with the name of %s, ret:%d, errmessage:%s", DBName.c_str(), ret, sqlite3_errmsg(targetDB));
        return false;
    }
    mDatabaseMap.erase(DBName);
    return true;
}

bool DBWrapper::beginBatch(sqlite3* db) {
    sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);  
    return true;
}

bool DBWrapper::endBatch(sqlite3* db) {
    sqlite3_exec(db, "COMMIT TRANSACTION;", NULL, NULL, NULL);
    return true;
}

bool DBWrapper::openTable(int32_t typeOfTable, const std::string& DBName, const std::string& tableName) {
    int ret = DEFAULT_VALUE_FOR_INT;
    std::string tableFormat;

    switch(typeOfTable) {
      case ORIGIN_TABLE: {
        tableFormat = TABLE_FORMAT_ORIGIN_DEF;
        break;
      }
      case FILTER_TRUNOVER_TABLE: {
        tableFormat = TABLE_FORMAT_FILTER_TURNOVER_DEF;
        break;
      }
      case FILTER_RESULT_TABLE: {
        tableFormat = TABLE_FORMAT_FILTER_RESULT_DEF;
        break;
      }
      case FILTER_TABLE: {
        break;
      }
      case FINAL_TABLE: {
        break;
      }

      default:
        LOGI(LOGTAG, "Unknown Table type :%d \n", typeOfTable);
        return false;
    }

    sqlite3* targetDB = getDBByName(DBName);
    std::string sql(CREATE_TABLE + tableName + tableFormat);

    LOGI(LOGTAG, "targetDB:%p", targetDB);
    if (!targetDB && !openDB(DBName, &targetDB)) {
        LOGI(LOGTAG, "Fail to get Database with name %s\n", DBName.c_str());
        LOGI(LOGTAG, "And fail to open a new Database with name %s either\n", DBName.c_str());
        return false;
    }

    ret = sqlite3_exec(targetDB, sql.c_str(), NULL, NULL, &sqlERR);
    if (ret != SQLITE_OK) {
        std::string error(sqlERR);
        if (error.find("already exists")) {
            LOGI(LOGTAG, "%s\n", sqlERR);
            LOGI(LOGTAG, "There is already a Table with name %s\n", tableName.c_str());
            return true;
        }

        LOGI(LOGTAG, "%s\n", sqlERR);
        LOGI(LOGTAG, "Fail to open the table with name %s\n", tableName.c_str());
        return false;
    }

    return true;
}

bool DBWrapper::getAllTablesOfDB(const std::string& aDBName, std::list<std::string>& tableNames) {
    std::string sql = GET_TABLES();
    //LOG sql
    sqlite3_stmt* stmt = NULL;
    int ret = -1;
    sqlite3* targetDB = NULL;
    if (!openDB(aDBName, &targetDB)) {
        LOGI(LOGTAG, "Fail to open DB with name:%s", aDBName.c_str());
        return false;
    }
    ret = sqlite3_prepare(targetDB,
                          sql.c_str(),
                          -1,
                          &stmt,
                          NULL);
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "Fail to prepare stmt to retrieve all the tables in:%s, errno:%d, ret:%d", aDBName.c_str(), errno, ret);
        return false;
    }

    while (true) {
        ret = sqlite3_step(stmt);
        if (ret == SQLITE_ROW) {
            std::string tableName = (char*)sqlite3_column_text(stmt, 0);
            tableNames.push_back(tableName);
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


bool DBWrapper::insertElement(std::string& DBName, std::string& tableName, std::string& KeyAndValues, sqlite3_callback fCallback) {
    int ret = DEFAULT_VALUE_FOR_INT;
    std::string sql = INSERT_OP + tableName + KeyAndValues;
    sqlite3* targetDB = getDBByName(DBName);

    ret = sqlite3_exec(targetDB, sql.c_str(), *fCallback, NULL, &sqlERR);
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "%s\n", sqlERR);
        LOGI(LOGTAG, "Discription is: %s\n", sql.c_str());
        return false;
    }

    return true;
}

bool DBWrapper::insertElementsInBatch(std::string& DBName,
                                      std::string& tableName,
                                      std::list<std::string>& strValues,
                                      std::list<XLSReader::XLSElement*>& xlsValues,
                                      sqlite3_callback fCallback) {
    
    sqlite3_stmt* stmt = NULL;
    sqlite3* targetDB = getDBByName(DBName);
    
    // The first description is the format of values to insert
    std::string format = *(strValues.begin());
    std::string sql = INSERT_OP + tableName + format;
    printf("DBWrapper: sql:%s, targetDB:%p\n", sql.c_str(), targetDB);
    int ret = sqlite3_prepare(targetDB,
                              sql.c_str(),
                              -1,
                              &stmt,
                              NULL); 

    printf("DBWrapper:ret:%d\n", ret);
    if (ret != SQLITE_OK) {
        printf("DBWrapper:ret:%d\n", ret);
        //LOGI
        return false;
    }

    beginBatch(targetDB);
    std::list<XLSReader::XLSElement*>::iterator itrOfValues;
    //FIXME: The first two elements of input arg 'values' are:
    //       1) (Time, Price, Float, Volume,  TurnOver,  SaleBuy) VALUES (?, ?, ?, ?, ?, ?)
    //       2) 成交时间        成交价  价格变动        成交量(手)      成交额(元)      性质
    //       So, ignore them.
    size_t listLen = xlsValues.size();
    size_t i = 0;
    for (itrOfValues = xlsValues.begin(); itrOfValues != xlsValues.end(); itrOfValues++) {
         sqlite3_reset(stmt);
         if (bindCommand(*itrOfValues, stmt) &&
             sqlite3_step(stmt) != SQLITE_DONE){
             //LOGI
             return false;  
         }  
    }
    sqlite3_finalize(stmt);
    endBatch(targetDB);

    return true;
}

bool DBWrapper::insertFilterResultsInBatch(const std::string& DBName,
                                           const std::string& tableName,
                                           std::list<std::string>& strValues,
                                           std::list<DBFilter::BaseResultData>& filterResultValues,
                                           sqlite3_callback fCallback) {
    sqlite3_stmt* stmt = NULL;
    sqlite3* targetDB = getDBByName(DBName);

    // The first description is the format of values to insert
    std::string format = *(strValues.begin());
    std::string sql = INSERT_OP + tableName + format;
    LOGI(LOGTAG, "DBWrapper: sql:%s, targetDB:%p\n", sql.c_str(), targetDB);
    int ret = sqlite3_prepare(targetDB,
                              sql.c_str(),
                              -1,
                              &stmt,
                              NULL);

    LOGI(LOGTAG, "DBWrapper:ret:%d\n", ret);
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "Fail to prepare stmt for sql:%s", sql.c_str());
        return false;
    }

    beginBatch(targetDB);
    std::list<DBFilter::BaseResultData>::iterator itrOfValues;
    size_t listLen = filterResultValues.size();
    size_t i = 0;
    for (itrOfValues = filterResultValues.begin(); itrOfValues != filterResultValues.end(); itrOfValues++) {
         sqlite3_reset(stmt);
         if (bindCommand(*itrOfValues, stmt) &&
             sqlite3_step(stmt) != SQLITE_DONE){
             //FIXME: Maybe we should end the process now
             sqlite3_finalize(stmt);
             endBatch(targetDB);
             LOGI(LOGTAG, "Fail to insert filter results into table: %s in DB: %s", tableName.c_str(), DBName.c_str());
             return false;
         }
    }
    sqlite3_finalize(stmt);
    endBatch(targetDB);

    return true;
}

bool DBWrapper::deleteElement(std::string& DBName, std::string& tableName, std::string& condition, sqlite3_callback fCallback) {
    int ret = DEFAULT_VALUE_FOR_INT;
    std::string sql = DELETE_OP + tableName + condition;
    sqlite3* targetDB = getDBByName(DBName);

    ret = sqlite3_exec(targetDB, sql.c_str(), *fCallback, NULL, &sqlERR);
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "%s\n", sqlERR);
        LOGI(LOGTAG, "Discription is: %s\n", sql.c_str());
        return false;
    }
 
    return true;
}

bool DBWrapper::updateElement(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback) {
    return true;
}

bool DBWrapper::joinTables(std::string& DBName, std::string& srcTableName, std::string& targetTableName, sqlite3_callback fCallback) {
    int ret = DEFAULT_VALUE_FOR_INT;
    std::string sql = "SELECT * INTO " + srcTableName + " FROM " + targetTableName;
    sqlite3* targetDB = getDBByName(DBName);

    if (!DBWrapper::isTableExist(DBName, srcTableName) ||
        !DBWrapper::isTableExist(DBName, targetTableName)) {
        LOGI(LOGTAG, "There is no table named %s or %s in Database %s\n", srcTableName.c_str(), targetTableName.c_str(), DBName.c_str());
        return false;
    }

    ret = sqlite3_exec(targetDB, sql.c_str(), *fCallback, NULL, &sqlERR);
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "%s\n", sqlERR);
        LOGI(LOGTAG, "Discription is: %s\n", sql.c_str());
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

bool DBWrapper::deleteRows(std::string& DBName, std::string& tableName, std::string& keyColumn, std::list<std::string>& targetRows) {
    int ret = -1;
    std::string sql;
    sqlite3* targetDB = NULL;
    sqlite3_stmt* stmt = NULL;

    std::list<std::string>::iterator itrRow;
    std::string deleteCondition;
    for (itrRow = targetRows.begin(); itrRow != targetRows.end();) {
         deleteCondition += "\"";
         deleteCondition += (*itrRow);
         deleteCondition += "\"";
         if (++itrRow != targetRows.end()) {
             deleteCondition += ", ";
         }
    }


    sql = DELETE_ROWS_FROM_TABLE(tableName, keyColumn, deleteCondition);

    DBWrapper::openDB(DBName, &targetDB);
    if (targetDB == NULL) {
        LOGI(LOGTAG, "Fail to open DB:%s in DBWrapper::deleteRows", DBName.c_str());
        return false;
    }

    ret = sqlite3_prepare(targetDB, sql.c_str(), -1, &stmt, NULL);
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "Fail to prepare stmt for DB:%s, sql:%s", DBName.c_str(), sql.c_str());
        return false;
    }

    if (sqlite3_step(stmt) != SQLITE_DONE){
        LOGI(LOGTAG, "Fail to step stmt for delete row:%s in table:%s of DB:%s, sqlite3_errmsg:%s", (*itrRow).c_str(), tableName.c_str(), DBName.c_str(), sqlite3_errmsg(targetDB));
        exit(1);
        return false;
    }
    
    if (SQLITE_OK != sqlite3_finalize(stmt)) {
        return false;
    }
    closeDB(DBName);

    return true;
}

bool DBWrapper::updateRows(std::string& DBName, std::string& tableName, std::string& description, sqlite3_callback fCallback) {
    return true;
}


//=======private
sqlite3* DBWrapper::getDBByName(const std::string& DBName) {
    if (!mDatabaseMap.count(DBName)) {
        return NULL;
    }

    std::map<std::string, sqlite3*>::iterator itr = mDatabaseMap.find(DBName);
    return itr->second;
}

bool DBWrapper::isTableExist(std::string& DBName, std::string& tableName) {
    return true;
}

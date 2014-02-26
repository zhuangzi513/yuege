#include "DBWrapper.h"
#include "DBOperations.h"
#include "ErrorDefines.h"

#include <stdlib.h>
#include <stdio.h>

#define LOGTAG "DBWrapperSqlite3"

#define DEFAULT_VALUE_FOR_INT -1
#define ORIGIN_SQLITE_NAME "test.db"

static char* sqlERR = NULL;

static bool bindCommand(XLSReader::XLSElement* xlsElement, sqlite3_stmt* stmt) {
    int ret = -1;
    //FIXME: For now, time is not used.
    //ret = sqlite3_bind_text(stmt, 0, xlsElement->mTime.c_str(), -1, NULL);
    //if (ret != SQLITE_OK) {
    //    LOGI(LOGTAG, "bind Time Fail: %s", xlsElement->mTime.c_str());
    //    return false;
    //}
    ret = sqlite3_bind_double(stmt, 1, atof(xlsElement->mPrice.c_str()));
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "bind Price Fail: %s", xlsElement->mPrice.c_str());
        return false;
    }
    ret = sqlite3_bind_double(stmt, 2, atof(xlsElement->mFloat.c_str()));
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "bind Float Fail: %s", xlsElement->mFloat.c_str());
        return false;
    }
    ret = sqlite3_bind_int(stmt, 3, atoi(xlsElement->mVolume.c_str()));
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "bind Volume Fail: %s", xlsElement->mVolume.c_str());
        return false;
    }
    ret = sqlite3_bind_double(stmt, 4, atof(xlsElement->mTurnOver.c_str()));
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "bind TurnOver Fail: %s", xlsElement->mTurnOver.c_str());
        return false;
    }
    ret = sqlite3_bind_text(stmt, 5, xlsElement->mSB.c_str(), -1, NULL);
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "bind SB Fail: %s", xlsElement->mSB.c_str());
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
    int ret = DEFAULT_VALUE_FOR_INT;
    ret = sqlite3_open(DBName.c_str(), ppDB);
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "Fail to open Database with the name of %s\n", DBName.c_str());
        return false;
    }

    //FIXME: The map should not be DBName:DBName
    mDatabaseMap.insert(
        std::map<std::string, sqlite3*>::value_type(DBName, *ppDB));
    return true;
}

bool DBWrapper::closeDB(const std::string& DBName) {
    sqlite3* targetDB = getDBByName(DBName);
    sqlite3_close(targetDB);
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

bool DBWrapper::openTable(int32_t typeOfTable, std::string& DBName, std::string& tableName) {
    int ret = DEFAULT_VALUE_FOR_INT;
    std::string tableFormat;

    switch(typeOfTable) {
      case ORIGIN_TABLE: {
        tableFormat = TABLE_FORMAT_ORIGIN_DEF;
        break;
      }
      case FILTER_TABLE: {
        break;
      }
      case FINAL_TABLE: {
        break;
      }

      default:
        LOGE(LOGTAG, "Unknown Table type :%d \n", typeOfTable);
        return false;
    }

    sqlite3* targetDB = getDBByName(DBName);
    std::string sql(CREATE_TABLE + tableName + tableFormat);

    LOGI(LOGTAG, "targetDB:%p", targetDB);
    if (!targetDB && !openDB(DBName, &targetDB)) {
        LOGE(LOGTAG, "Fail to get Database with name %s\n", DBName.c_str());
        LOGE(LOGTAG, "And fail to open a new Database with name %s either\n", DBName.c_str());
        return false;
    }

    ret = sqlite3_exec(targetDB, sql.c_str(), NULL, NULL, &sqlERR);
    if (ret != SQLITE_OK) {
        std::string error(sqlERR);
        if (error.find("already exists")) {
            LOGD(LOGTAG, "%s\n", sqlERR);
            LOGD(LOGTAG, "There is already a Table with name %s\n", tableName.c_str());
            return true;
        }

        LOGE(LOGTAG, "%s\n", sqlERR);
        LOGE(LOGTAG, "Fail to open the table with name %s\n", tableName.c_str());
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
        LOGE(LOGTAG, "%s\n", sqlERR);
        LOGD(LOGTAG, "Discription is: %s\n", sql.c_str());
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
        //LOGE
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
             //LOGE
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
        LOGE(LOGTAG, "%s\n", sqlERR);
        LOGD(LOGTAG, "Discription is: %s\n", sql.c_str());
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
        LOGE(LOGTAG, "There is no table named %s or %s in Database %s\n", srcTableName.c_str(), targetTableName.c_str(), DBName.c_str());
        return false;
    }

    ret = sqlite3_exec(targetDB, sql.c_str(), *fCallback, NULL, &sqlERR);
    if (ret != SQLITE_OK) {
        LOGE(LOGTAG, "%s\n", sqlERR);
        LOGD(LOGTAG, "Discription is: %s\n", sql.c_str());
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
sqlite3* DBWrapper::getDBByName(const std::string& DBName) {
    std::map<std::string, sqlite3*>::iterator itr = mDatabaseMap.find(DBName);
    return itr->second;
}

bool DBWrapper::isTableExist(std::string& DBName, std::string& tableName) {
    return true;
}

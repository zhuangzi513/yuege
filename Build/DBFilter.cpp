#include "DBFilter.h"

#include "DBOperations.h"
#include "DBWrapper.h"
#include "ErrorDefines.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#define DEFAULT_LASTING_LEN 4

#define LOGTAG   "DBFilter"
#define DEFAULT_VALUE_FOR_INT -1
#define MIN_TURNOVER 10

#define DATE                " Date "
#define VOLUME              " Volume "
#define TURNOVER            " TurnOver "
#define SALE_BUY            " SaleBuy "
#define BEGIN_PRICE         " BeginPrice "
#define END_PRICE           " EndPrice "
#define SUM_FLOWIN_TEN_DAYS " SumFlowInTenDay "
#define FLOWIN_ONE_DAY      " FlowInOneDay "

static char* sqlERR = NULL;
static double sumShots   = 0.0;
static int sumForcasters = 0;

static std::list<double> sPre15Flowins;

std::list<std::string> DBFilter::mTableNames;
std::string DBFilter::mResultTableName         = "FilterResult";
std::string DBFilter::mTmpResultTableName      = "MiddleWareTable";
std::string DBFilter::mDiffBigBuySaleTableName = "";

static std::string SELECT_COLUMNS(const std::string& tableName, const std::string& targetColumns) {
    std::string command("");
    command += " SELECT ";
    command += targetColumns;
    command += " FROM ";
    command += tableName;
    return command;
}

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

DBFilter::DBFilter(const std::string& aDBName) {
    openOriginDB(aDBName);
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

bool DBFilter::getBuyDateRegionsContinueFlowin(const std::string& aDBName, std::list<DBFilter::DateRegion>& recommandBuyDateRegions) {
    std::string sql, targetColumns;
    sqlite3_stmt* stmt = NULL;
    bool boolRet = false;
    int intRet = -1;

    boolRet = openTable(DBWrapper::FILTER_TRUNOVER_TABLE, aDBName, mResultTableName);
    if (!boolRet) {
        LOGI(LOGTAG, "Fail to open table:%s in DB:%s", mResultTableName.c_str(), aDBName.c_str());
        return false;
    }

    targetColumns += DATE;
    targetColumns += ",";
    targetColumns += SUM_FLOWIN_TEN_DAYS;
    targetColumns += ",";
    targetColumns += FLOWIN_ONE_DAY;
    sql = SELECT_COLUMNS(mResultTableName, targetColumns);

    LOGI(LOGTAG, "sql:%s", sql.c_str());
    intRet = sqlite3_prepare(mOriginDB,
                          sql.c_str(),
                          -1,
                          &stmt,
                          NULL);

    if (intRet != SQLITE_OK) {
        LOGI(LOGTAG, "Fail to prepare stmt to Table:%s", mResultTableName.c_str());
        return false;
    }

    DBFilter::DateRegion tmpDateRegion;
    int count = 0;
    bool startNewBlock = false;
    std::string startDate, endDate;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        double sumFlowinTenDays = sqlite3_column_double(stmt, 1);
        double flowinOneDay = sqlite3_column_double(stmt, 2);

        if (flowinOneDay < 0 && startNewBlock) {
            startNewBlock = false;
            endDate = (char*)sqlite3_column_text(stmt, 0);
            tmpDateRegion.mStartDate = startDate;
            tmpDateRegion.mEndDate   = endDate;
            recommandBuyDateRegions.push_back(tmpDateRegion);
            startDate = endDate = "";
        }

        if (sumFlowinTenDays > 0 && count >= DEFAULT_LASTING_LEN&&
            (flowinOneDay > MIN_TURNOVER || flowinOneDay < -MIN_TURNOVER)) {
            //StartNewBlock
            startNewBlock = true;
            startDate = (char*)sqlite3_column_text(stmt, 0);
            count = 0;
        }

        if (sumFlowinTenDays < 0) {
            count++;
            continue;
        }

    }

    return true;
}

static bool activeEnough(std::list<double> pre15Flowins) {
    int positiveCount = 0;
    std::list<double>::iterator itr;
    for (itr = pre15Flowins.begin(); itr != pre15Flowins.end(); itr++) {
         if ((*itr) > MIN_TURNOVER) {
             positiveCount++;
         }
    }
    return (positiveCount >= 5);
}

bool DBFilter::getBuyDateRegionsContinueFlowinPri(const std::string& aDBName, std::list<DBFilter::DateRegion>& recommandBuyDateRegions) {
    std::string sql, targetColumns;
    sqlite3_stmt* stmt = NULL;
    bool boolRet = false;
    int intRet = -1;

    boolRet = openTable(DBWrapper::FILTER_TRUNOVER_TABLE, aDBName, mResultTableName);
    if (!boolRet) {
        LOGI(LOGTAG, "Fail to open table:%s in DB:%s", mResultTableName.c_str(), aDBName.c_str());
        return false;
    }

    targetColumns += DATE;
    targetColumns += ",";
    targetColumns += SUM_FLOWIN_TEN_DAYS;
    targetColumns += ",";
    targetColumns += FLOWIN_ONE_DAY;
    sql = SELECT_COLUMNS(mResultTableName, targetColumns);

    LOGI(LOGTAG, "sql:%s", sql.c_str());
    intRet = sqlite3_prepare(mOriginDB,
                          sql.c_str(),
                          -1,
                          &stmt,
                          NULL);

    if (intRet != SQLITE_OK) {
        LOGI(LOGTAG, "Fail to prepare stmt to Table:%s", mResultTableName.c_str());
        return false;
    }

    DBFilter::DateRegion tmpDateRegion;
    int count = 0;
    bool startNewBlock = false;
    std::string startDate, endDate;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        double sumFlowinTenDays = sqlite3_column_double(stmt, 1);
        double flowinOneDay = sqlite3_column_double(stmt, 2);

        if (flowinOneDay > MIN_TURNOVER || flowinOneDay < -MIN_TURNOVER) {
            if (sPre15Flowins.size() >= 15) {
                sPre15Flowins.pop_front();
                sPre15Flowins.push_back(flowinOneDay);
            }
        }

        if (flowinOneDay < 0 && startNewBlock) {
            startNewBlock = false;
            endDate = (char*)sqlite3_column_text(stmt, 0);
            tmpDateRegion.mStartDate = startDate;
            tmpDateRegion.mEndDate   = endDate;
            recommandBuyDateRegions.push_back(tmpDateRegion);
            startDate = endDate = "";
        }

        if (sumFlowinTenDays > 0 && count >= DEFAULT_LASTING_LEN &&
            (flowinOneDay > MIN_TURNOVER || flowinOneDay < -MIN_TURNOVER)) {
            //StartNewBlock
            if (activeEnough(sPre15Flowins)) {
                startNewBlock = true;
                startDate = (char*)sqlite3_column_text(stmt, 0);
            }
            count = 0;
        }

        if (sumFlowinTenDays < 0) {
            count++;
            continue;
        }

    }

    return true;
}

bool DBFilter::getRecommandBuyDateRegions(const int throughWhat, const std::string& aDBName, std::list<DBFilter::DateRegion>& recommandBuyDateRegions) {
    LOGI(LOGTAG, "Enter getRecommandBuyDateRegions, aDBName:%s", aDBName.c_str());

    switch(throughWhat) {
      case DBFilter::CONTINUE_FLOWIN:
          getBuyDateRegionsContinueFlowin(aDBName, recommandBuyDateRegions);
          break;
      case DBFilter::CONTINUE_FLOWIN_PRI:
          getBuyDateRegionsContinueFlowinPri(aDBName, recommandBuyDateRegions);
          break;
      default:
          LOGI(LOGTAG, "Unkown type to get RecommandBuyDateRegions:%d", throughWhat);
          return false;
    }
    
    return true;
}

//=======private
bool DBFilter::openOriginDB(const std::string& name) {
    return DBWrapper::openDB(name.c_str(), &mOriginDB);
}

bool DBFilter::closeOriginDB(const std::string& aDBName) {
    int ret = DEFAULT_VALUE_FOR_INT;
    ret = DBWrapper::closeDB(aDBName);
    mTableNames.clear();
    return (ret == SQLITE_OK) ? true : false;
}

bool DBFilter::openTable(int index, const std::string& aDBName, const std::string& aTableName) {
    if (!openOriginDB(aDBName)) {
        return false;
    }

    return DBWrapper::openTable(index, aDBName, aTableName);
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

bool DBFilter::computeResultFromTable(const std::string& aDBName,
                                      const std::string& tmpTableName,
                                      const std::string& originTableName,
                                      const double beginningPrice,
                                      const double endingPrice) {
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

    sql = SUM_FILTER_RESULT_TABLE(tmpTableName, columns, keyColumn);

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
        BaseResultData tempBaseResultData;
        while(sqlite3_step(stmt) == SQLITE_ROW) {
            std::string isBuy("");
            isBuy= (char*)sqlite3_column_text(stmt, 2);
            LOGD(LOGTAG, "%s", isBuy.c_str());

            if (isBuy == std::string("true")) {
                tempBaseResultData.mBuyVolume   = sqlite3_column_int(stmt, 0);
                tempBaseResultData.mBuyTurnOver = sqlite3_column_double(stmt, 1);
                tempBaseResultData.mBuyPrice    = (tempBaseResultData.mBuyTurnOver/(100 * tempBaseResultData.mBuyVolume));
                LOGD(LOGTAG, "sale, volume:%d, turnover:%f, avg:%f", tempBaseResultData.mBuyVolume, tempBaseResultData.mBuyTurnOver, tempBaseResultData.mBuyPrice);
            } else if (isBuy == std::string("false")) {
                tempBaseResultData.mSaleVolume   = sqlite3_column_int(stmt, 0);
                tempBaseResultData.mSaleTurnOver = sqlite3_column_double(stmt, 1);
                tempBaseResultData.mSalePrice    = (tempBaseResultData.mSaleTurnOver/(100 * tempBaseResultData.mSaleVolume));
                LOGD(LOGTAG, "buy, volume:%d, turnover:%f, avg:%f", tempBaseResultData.mSaleVolume, tempBaseResultData.mSaleTurnOver, tempBaseResultData.mSalePrice);
            } else {
                // Not buy, not sale, just a normal.
                // The sale_buy should be empty
                if (isBuy.empty()) {
                    LOGD(LOGTAG, "NON-BUY-SALE");
                    continue;
                }
                LOGD(LOGTAG, "%s", isBuy.c_str());
                return false;
            }
        }
        tempBaseResultData.mDate = originTableName;
        tempBaseResultData.mPureFlowInOneDay = tempBaseResultData.mBuyTurnOver - tempBaseResultData.mSaleTurnOver;
        LOGD(LOGTAG, "turnover: sale:%f, buy:%f, diff:%f", tempBaseResultData.mSaleTurnOver, tempBaseResultData.mBuyTurnOver, tempBaseResultData.mBuyTurnOver - tempBaseResultData.mSaleTurnOver);

        int i = 0;
        std::list<DBFilter::BaseResultData>::iterator itr;

        // Figureout flowin in 10 days
        for (i = 0, itr = mBaseResultDatas.end(); i < 10 && itr != mBaseResultDatas.begin(); itr--) {
            //Only valueable days are counted on
            if ((*itr).mBuyTurnOver > MIN_TURNOVER && (*itr).mSaleTurnOver > MIN_TURNOVER) {
                LOGD(LOGTAG, "TurnOver sale:%f, buy:%f, diff:%f", (*itr).mSaleTurnOver, (*itr).mBuyTurnOver, (*itr).mPureFlowInOneDay)
                //LOGD(LOGTAG, "Date:%s, mPureFlowInOneDay:%f", (*itr).mDate.c_str(), (*itr).mPureFlowInOneDay)
                LOGD(LOGTAG, "tempBaseResultData.mSumFlowInTenDays:%f, mPureFlowInOneDay:%f", tempBaseResultData.mSumFlowInTenDays,  (*itr).mPureFlowInOneDay);
                tempBaseResultData.mSumFlowInTenDays += (*itr).mPureFlowInOneDay;
                i++;
            }
        }

        //TODO: Optimization of Figurout flowin in 10 days
        //if (mBaseResultDatas.size() > 10) {
        //    for (i = 0, itr = mBaseResultDatas.end(); i < 10 && itr != mBaseResultDatas.begin(); itr--) {
        //        if ((*itr).mBuyTurnOver > MIN_TURNOVER && (*itr).mSaleTurnOver > MIN_TURNOVER) {
        //            i++;
        //        }
        //    }
        //        LOGD(LOGTAG, "end mSumFlowInTenDays:%f, 10 pre mSumFlowInTenDays:%f", (*mBaseResultDatas.end()).mSumFlowInTenDays,  (*itr).mSumFlowInTenDays);
        //    tempBaseResultData.mSumFlowInTenDays = (*mBaseResultDatas.end()).mSumFlowInTenDays + ((*mBaseResultDatas.end()).mSumFlowInTenDays - (*itr).mSumFlowInTenDays);
        //} else {
        //    for (i = 0, itr = mBaseResultDatas.end(); i < 10 && itr != mBaseResultDatas.begin(); itr--) {
        //        if ((*itr).mBuyTurnOver > MIN_TURNOVER && (*itr).mSaleTurnOver > MIN_TURNOVER) {
        //            tempBaseResultData.mSumFlowInTenDays += (*itr).mPureFlowInOneDay;
        //            i++;
        //        }
        //    }
        //}

        tempBaseResultData.mBeginPrice = beginningPrice;
        tempBaseResultData.mEndPrice   = endingPrice;
        mBaseResultDatas.push_back(tempBaseResultData);
    }

    ret = sqlite3_finalize(stmt);
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "Fail to finalize the stmt to finalize tmpTable:%s for originTable:%s", tmpTableName.c_str(), originTableName.c_str());
        return false;
    }

    return true;
}

bool DBFilter::saveBaseResultInBatch(const std::string& aDBName, const std::string& tableName) {
    //FIXME: the 'tableName" MUST same to mResultTableName
    int ret = 0;
    if (!openTable(DBWrapper::FILTER_RESULT_TABLE, aDBName, tableName)) {
        LOGI(LOGTAG, "Fail to openTable: %s in DB: %s", tableName.c_str(), aDBName.c_str());
        return false;
    }

    //Format DES
    std::list<std::string> descriptions;
    std::string singleDes;
    singleDes = TABLE_FORMAT_FILTER_RESULT;
    singleDes += D_STMT_FORMAT_FILTER_RESULT;
    descriptions.push_back(singleDes);

    //Result data DES
    std::list<DBFilter::BaseResultData>::iterator iterOfFilterResult;

    for (iterOfFilterResult = mBaseResultDatas.begin(); iterOfFilterResult != mBaseResultDatas.end(); iterOfFilterResult++) {
        std::string values("");
        values += "(";
        values += (*iterOfFilterResult).mSaleVolume;
        values += ", ";
        values += (*iterOfFilterResult).mBuyVolume;
        values += ", ";
        values += (*iterOfFilterResult).mSaleTurnOver;
        values += ", ";
        values += (*iterOfFilterResult).mBuyTurnOver;
        values += ", ";
        values += (*iterOfFilterResult).mSalePrice;
        values += ", ";
        values += (*iterOfFilterResult).mBuyPrice;
        values += ", ";
        values += (*iterOfFilterResult).mPureFlowInOneDay;
        values += ", ";
        values += (*iterOfFilterResult).mSumFlowInTenDays;
        values += ")";
    }
    

    DBWrapper::insertFilterResultsInBatch(aDBName, tableName, descriptions, mBaseResultDatas, NULL);

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

bool DBFilter::getBeginAndEndPrice(const std::string& tableName, double& beginningPrice, double& endingPrice) {
    int ret = -1;
    std::string sql;
    std::string targetColumns = " Price ";
    sqlite3_stmt* stmt = NULL;

    sql = SELECT_COLUMNS(tableName, targetColumns);
    LOGD(LOGTAG, "getBeginAndEndPrice, sql:%s", sql.c_str());
    ret = sqlite3_prepare(mOriginDB,
                          sql.c_str(),
                          -1,
                          &stmt,
                          NULL);

    if (ret != SQLITE_OK) {
        LOGD(LOGTAG, "Fail to prepare stmt to select end Price for table:%s", tableName.c_str());
        return false;
    }

    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        i++;
        if (i == 2) {
            endingPrice = sqlite3_column_double(stmt, 0);
            LOGD(LOGTAG, "getBeginAndEndPrice, endPrice:%f", endingPrice);
            continue;
        }
        beginningPrice = sqlite3_column_double(stmt, 0);    
    }
    LOGD(LOGTAG, "getBeginAndEndPrice, beginPrice:%f", beginningPrice);

    ret = sqlite3_finalize(stmt);
    if (ret != SQLITE_OK) {
        LOGD(LOGTAG, "Fail to finalize the stmt to finalize table:%s", tableName.c_str());
        return false;
    }

    return true;
}

bool DBFilter::filterTableByTurnOver(const std::string& tableName, const int aMinTurnOver) {
    LOGD(LOGTAG, "filterTableByTurnOver for table:%s", tableName.c_str());
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
    LOGD(LOGTAG, "sql:%s", sql.c_str());

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
    if (!openTable(DBWrapper::FILTER_TRUNOVER_TABLE, aDBName, mTmpResultTableName)) {
        LOGI(LOGTAG, "Fail to open table:%s in DB:%s", mTmpResultTableName.c_str(), aDBName.c_str());
        return false;
    }
    std::list<std::string>::iterator iterOfTableName;
    double beginningPrice, endingPrice;
    for (iterOfTableName = mTableNames.begin(); iterOfTableName != mTableNames.end(); iterOfTableName++) {
         if ((*iterOfTableName) != (mResultTableName) &&
             (*iterOfTableName) != (mTmpResultTableName)) {
             //Step 1: get Beginning & ending Price
             beginningPrice = endingPrice = 0.0;
             if (!getBeginAndEndPrice(*iterOfTableName, beginningPrice, endingPrice)) {
                 return false;
             }

             //Step 2: filter
             if (!filterTableByTurnOver(*iterOfTableName, aMinTurnover)) {
                 return false;
             }

             //Step 3: compute && save result 
             if (!computeResultFromTable(aDBName, mTmpResultTableName, *iterOfTableName, beginningPrice, endingPrice)) {
                 return false;
             }

             if (!clearTable(mTmpResultTableName)) {
                 return false;
             }
         }
    }

    ////Step 4: save to final result 
    if (!saveBaseResultInBatch(aDBName, mResultTableName)) {
        return false;
    }

    return true;
}

//FIXME: the recommandBuyRegions should be of const
bool DBFilter::getHitRateOfBuying(const std::string& aDBName, std::list<DBFilter::DateRegion>& recommandBuyRegions) {
    LOGI(LOGTAG, "Enter getHitRateOfBuying, aDBName:%s", aDBName.c_str());
    std::string sql, targetColumns;
    sqlite3_stmt* stmt = NULL;
    bool boolRet = false;
    int intRet = -1;

    boolRet = openTable(DBWrapper::FILTER_TRUNOVER_TABLE, aDBName, mResultTableName);
    if (!boolRet) {
        LOGI(LOGTAG, "Fail to open table:%s in DB:%s", mResultTableName.c_str(), aDBName.c_str());
        return false;
    }

    targetColumns += DATE;
    targetColumns += ", ";
    targetColumns += BEGIN_PRICE;
    targetColumns += ", ";
    targetColumns += END_PRICE;
    sql = SELECT_COLUMNS(mResultTableName, targetColumns);

    LOGI(LOGTAG, "sql:%s", sql.c_str());
    intRet = sqlite3_prepare(mOriginDB,
                             sql.c_str(),
                             -1,
                             &stmt,
                             NULL);

    if (intRet != SQLITE_OK) {
        LOGI(LOGTAG, "Fail to prepare stmt to Table: %s, ret:%d", mResultTableName.c_str(), intRet);
        return false;
    }

    int index = 0;
    std::list<DBFilter::DateRegion>::iterator itr = recommandBuyRegions.begin();
    std::list<DBFilter::PriceRegion> realPriceRegions;
    DBFilter::PriceRegion tmpPriceRegion;

    std::list<DBFilter::DateRegion>::iterator debugItr = recommandBuyRegions.begin();
    while (debugItr != recommandBuyRegions.end()) {
        LOGI(LOGTAG, "mStartDate in recommandRegions:%s, mEndDate:%s", (*debugItr).mStartDate.c_str(), (*debugItr).mEndDate.c_str());
        debugItr++;
    }

    //Step 1: compare the Date
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        //NOTE: Assume that the recommandBuyRegions are ordered by date.
        std::string date = (char*)sqlite3_column_text(stmt, 0);
        if (date < (*itr).mStartDate) {
            // have not reach the edge, need not to compare
            continue;
        } else if (date == (*itr).mStartDate) {
            // record the beginningPrice
            // once beggingPrice is set, a new block of region start.
            // so clear the endPrice.
            LOGI(LOGTAG, "Find the the startDate in recommandRegions:%s", date.c_str());
            tmpPriceRegion.mBeginPrice = sqlite3_column_double(stmt, 1);
            continue;
        } else if (date == (*itr).mEndDate) {
            // record the endPrice
            LOGI(LOGTAG, "Find the the EndDate in recommandRegions:%s", date.c_str());
            tmpPriceRegion.mEndPrice = sqlite3_column_double(stmt, 2);
            LOGI(LOGTAG, "the EndPrice:%f , the BeginPrice:%f in tmpPriceRegion:%f", tmpPriceRegion.mEndPrice, tmpPriceRegion.mBeginPrice);
            realPriceRegions.push_back(tmpPriceRegion);
            itr++;
        } else {
            LOGD(LOGTAG, "Normal Date %s", date.c_str());
        }
    }

    intRet = sqlite3_finalize(stmt);
    if (intRet != SQLITE_OK) {
        LOGI(LOGTAG, "Fail to finalize the stmt to finalize table:%s", mResultTableName.c_str());
        return false;
    }


    //Step 2: ensure UP DOWN
    if (realPriceRegions.size() != recommandBuyRegions.size()) {
        LOGI(LOGTAG, "The size of PriceRegions does not equal to that of recommandBuyRegions");
        LOGI(LOGTAG, "realPriceRegions length:%d, recommandBuyRegions:%d", realPriceRegions.size(), recommandBuyRegions.size());
        return false;
    }

    std::list<DBFilter::PriceRegion>::iterator debugPriceRegionItr = realPriceRegions.begin();
    while (debugPriceRegionItr!= realPriceRegions.end()) {
        LOGI(LOGTAG, "mStartDate in PriceRegions mBeginPrice:%f, mEndPrice:%f", (*debugPriceRegionItr).mBeginPrice, (*debugPriceRegionItr).mEndPrice);
        debugPriceRegionItr++;
    }

    double count = 0.0;
    std::list<DBFilter::PriceRegion>::iterator itrOfPriceRegion = realPriceRegions.begin();
    for (int i = 0; i < realPriceRegions.size(); i++) {
        double diff = (*itrOfPriceRegion).mEndPrice - (*itrOfPriceRegion).mBeginPrice;
        if (diff >= 0.01) {
            count = count + 1.0;
        }
        itrOfPriceRegion++;
        LOGI(LOGTAG, "diff:%f, count:%f", diff, count);
    }

    sumShots += count;
    sumForcasters += realPriceRegions.size();

    //Step 3: compute HintRate
    double hitRate = count/realPriceRegions.size();
    LOGI(LOGTAG, "HitRate for Suggesting Buy:%f, shot count:%f, sum count:%d", hitRate, count, realPriceRegions.size());

    return true;
}

bool DBFilter::getGlobalHitRate(double& hitRate) {
    hitRate = sumShots/sumForcasters;
    return true;
}


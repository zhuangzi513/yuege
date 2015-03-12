#include "DBFilter.h"

#include "DBOperations.h"
#include "DBWrapper.h"
#include "ErrorDefines.h"
#include "UtilsDefines.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

//#define DEFAULT_LASTING_LEN 3

#define LOGTAG   "DBFilter"
#define DEFAULT_VALUE_FOR_INT -1
//#define MIN_TURNOVER 100
#define DAYS_BEFORE_LEV1 5
#define DAYS_BEFORE_LEV2 10
#define DAYS_BEFORE_LEV3 20

static char* sqlERR = NULL;
static double sumShots   = 0.0;
static double sumForcasters = 0.0;
static double sumIncome = 0.0;
static bool sResultTableNamesInited = false;

std::list<std::string> DBFilter::mResultTableNames;
std::list<double> DBFilter::mFilterTurnOvers;

std::string DBFilter::mTmpResultTableName      = "MiddleWareTable";
std::string DBFilter::mDiffBigBuySaleTableName = "";

static void initResultTableNames() {
    //XXX: Make sure the right order here
    DBFilter::mResultTableNames.push_back("FilterResult20W");
    DBFilter::mFilterTurnOvers.push_back(200000.0);
/*
    DBFilter::mResultTableNames.push_back("FilterResult100W");
    DBFilter::mResultTableNames.push_back("FilterResult10W");
    DBFilter::mResultTableNames.push_back("FilterResult20W");
    DBFilter::mResultTableNames.push_back("FilterResult30W");
    DBFilter::mResultTableNames.push_back("FilterResult40W");
    DBFilter::mResultTableNames.push_back("FilterResult50W");
    DBFilter::mResultTableNames.push_back("FilterResult60W");
    DBFilter::mResultTableNames.push_back("FilterResult70W");
    DBFilter::mResultTableNames.push_back("FilterResult80W");
    DBFilter::mResultTableNames.push_back("FilterResult90W");

    DBFilter::mFilterTurnOvers.push_back(1000000.0);
    DBFilter::mFilterTurnOvers.push_back(100000.0);
    DBFilter::mFilterTurnOvers.push_back(200000.0);
    DBFilter::mFilterTurnOvers.push_back(300000.0);
    DBFilter::mFilterTurnOvers.push_back(400000.0);
    DBFilter::mFilterTurnOvers.push_back(500000.0);
    DBFilter::mFilterTurnOvers.push_back(600000.0);
    DBFilter::mFilterTurnOvers.push_back(700000.0);
    DBFilter::mFilterTurnOvers.push_back(800000.0);
    DBFilter::mFilterTurnOvers.push_back(900000.0);
*/

    sResultTableNamesInited = true;
}

DBFilter::DBFilter(const std::string& aDBName)
        : mDBName(aDBName) {
    init();
}

bool DBFilter::init() {
    bool boolRet = false;

    boolRet = openOriginDB();

    if (!sResultTableNamesInited) {
        initResultTableNames();
    }

    DBWrapper::getAllTablesOfDB(mDBName, mOriginTableNames);
    //FIXME: Hack here, the name of origin-table starts from 'O'.
    //       result-tables start from 'F' and middleware result-table
    //       starts from 'M'. 
    std::list<std::string> existingResultTables;
    while (true) {
        if (mOriginTableNames.front() <= mTmpResultTableName) {
            existingResultTables.push_back(mOriginTableNames.front());
            mOriginTableNames.pop_front();
        } else {
            break;
        }
    }

    //Make sure all the reuslt-tables are created already.
    std::list<std::string>::iterator itrResultTable;
    for (itrResultTable = DBFilter::mResultTableNames.begin();
         itrResultTable != DBFilter::mResultTableNames.end();
         itrResultTable++) {
         if (!openTable(DBWrapper::FILTER_RESULT_TABLE, *itrResultTable)) {
             LOGE(LOGTAG, "Fail to open table: %s in database:%s", (*itrResultTable).c_str(), mDBName.c_str());
             return false;
         }
    }

    //Make sure the middle-ware reuslt-tables is created already too.
    if (!openTable(DBWrapper::FILTER_TRUNOVER_TABLE, mTmpResultTableName)) {
        LOGE(LOGTAG, "Fail to open table: %s in database:%s", mTmpResultTableName.c_str(), mDBName.c_str());
        return false;
    }
}

DBFilter::~DBFilter() {
    mBaseResultDatas.clear();
    mNewAddedTables.clear();
    mOriginTableNames.clear();
    closeOriginDB(mDBName);
}

bool DBFilter::removeTableFromOriginDB(const std::string& aTableName) {
    if (!removeTable(aTableName)) {
        LOGE(LOGTAG, "Fail to clear table:%s", aTableName.c_str());
        return false;
    }
    return true;
}

bool DBFilter::clearTableFromOriginDB(const std::string& aTableName) {
    LOGD(LOGTAG, "clear Table: %s from Database: %s", aTableName.c_str(), mDBName.c_str());

    if (!openOriginDB()) {
        LOGE(LOGTAG, "Fail to open db:%s", mDBName.c_str());
        return false;
    }

    if (!clearTable(aTableName)) {
        LOGE(LOGTAG, "Fail to clear table:%s", aTableName.c_str());
        return false;
    }

    return true;
}

bool DBFilter::filterOriginDBByTurnOver() {
    std::list<std::string>::iterator iterResultTable;
    std::list<double>::iterator iterFilterTurnOver;
    for (iterResultTable = mResultTableNames.begin(), iterFilterTurnOver = mFilterTurnOvers.begin();
         iterResultTable != mResultTableNames.end() && iterFilterTurnOver != mFilterTurnOvers.end();
         iterResultTable++, iterFilterTurnOver++) {
        if (!filterTablesByTurnOver(*iterResultTable, *iterFilterTurnOver, mOriginTableNames)) {
            LOGE(LOGTAG, "Fail to filter table:%s of :%s", (*iterResultTable).c_str(), mDBName.c_str());
            return false;
        }
    }

    return true;
}

bool DBFilter::updateFilterResultByTurnOver(const std::string& aResultTableName, const int aMinTurnover, const int aMaxTurnOver) {
    LOGD(LOGTAG, "DBName:%s", mDBName.c_str());
    if (!openOriginDB()) {
        LOGE(LOGTAG, "Fail to open db:%s", mDBName.c_str());
        return false;
    }

    //Step 1: get the filtered origin tables
    std::list<std::string> filteredOriginTables;
    std::string sql, targetColumns;
    sqlite3_stmt* stmt = NULL;
    int intRet = -1;

    targetColumns += STRING_DATE;
    sql = SELECT_COLUMNS(aResultTableName, targetColumns);

    LOGD(LOGTAG, "sql:%s", sql.c_str());
    intRet = sqlite3_prepare_v2(mOriginDB,
                          sql.c_str(),
                          -1,
                          &stmt,
                          NULL);
    LOGD(LOGTAG, "prepare stmt to Table:%s, intRet:%d, mOriginDB:%p", aResultTableName.c_str(), intRet, mOriginDB);

    if (intRet != SQLITE_OK) {
        LOGE(LOGTAG, "Fail to prepare stmt to Table:%s, intRet:%d, errorMessage:%s", aResultTableName.c_str(), intRet, sqlite3_errmsg(mOriginDB));
        return false;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string dateOfOriginTable = (char*)sqlite3_column_text(stmt, 0);
        filteredOriginTables.push_back(dateOfOriginTable);
    }

    if (SQLITE_OK != sqlite3_finalize(stmt)) {
        LOGE(LOGTAG, "Fail to finalize stmt in updateFilterResultByTurnOver aDBName:%s", mDBName.c_str());
        return false;
    }

    //Step 2: get the new added origin tables
    std::list<std::string>::iterator itrAll = mOriginTableNames.begin();
    std::list<std::string>::iterator itrFiltered = filteredOriginTables.begin();
    while (itrFiltered != filteredOriginTables.end()
           && itrAll != mOriginTableNames.end()) {
           if ((*itrAll) == "FilterResult"
               || (*itrAll) == "MiddleWareTable") {
               itrAll++;
               continue;
           }

           if ((*itrAll) == (*itrFiltered)) {
               itrAll++;
               itrFiltered++;
               LOGD(LOGTAG, "Existing Table:%s, DB:%s", (*itrAll).c_str(), mDBName.c_str());
               continue;
           } else if ((*itrAll) > (*itrFiltered)) {
               mNewAddedTables.push_back(*itrAll);
               LOGD(LOGTAG, "Find a new added Table:%s, DB:%s", (*itrAll).c_str(), mDBName.c_str());
               itrAll++;
               continue;
           } else {
               // we should not get here
               itrFiltered++;
               continue;
           }
    }

    while (itrAll != mOriginTableNames.end()) {
        mNewAddedTables.push_back(*itrAll);
        LOGD(LOGTAG, "New added Table:%s, DB:%s", (*itrAll).c_str(), mDBName.c_str());
        itrAll++;
    }

    if (mNewAddedTables.size() < 1) {
        LOGE(LOGTAG, "No any new added OriginTables, dbName:%s", mDBName.c_str());
        return true;
    }

    //Step 3: get the new added origin tables
    if (!filterTablesByTurnOver(aResultTableName, aMinTurnover, mNewAddedTables)) {
        LOGE(LOGTAG, "Fail to filter tables of :%s", mDBName.c_str());
        return false;
    }
    mNewAddedTables.clear();

    return true;
}

//=======private
bool DBFilter::openOriginDB() {
    //XXX: If fail to open database, there must be something critical wrong and
    //     we should stop to check what happed immediatly.
    if (!DBWrapper::openDB(mDBName.c_str(), &mOriginDB)) {
        LOGE(LOGTAG, "CRITICAL ERROR: Fail to open database:%s", mDBName.c_str());
        exit(1);
    }

    return true;
}

bool DBFilter::closeOriginDB(const std::string& aDBName) {
    int ret = DEFAULT_VALUE_FOR_INT;
    ret = DBWrapper::closeDB(aDBName);
    return (ret == SQLITE_OK) ? true : false;
}

bool DBFilter::openTable(int aType, const std::string& aTableName) {
    //XXX: If fail to open database or table, there must be something critical wrong and
    //     we should stop to check what happed immediatly.

    if (!openOriginDB()) {
        LOGE(LOGTAG, "CRITICAL ERROR: Fail to open database:%s", mDBName.c_str());
        exit(1);
    }

    //If it already existing one FilterResult, that's ok
    if (DBWrapper::FAIL_OPEN_TABLE != DBWrapper::openTable(aType, mDBName, aTableName)) {
        //LOGE(LOGTAG, "CRITICAL ERROR: Fail to open table:%s in database:%s", aTableName.c_str(), mDBName.c_str());
        //LOGE(LOGTAG, "CRITICAL ERROR: Msg:%s", sqlite3_errmsg(mOriginDB));
        //exit(1);
    }

    return true;
}

sqlite3* DBFilter::getDBByName(const std::string& aDBName) {
    //JUST FOR NOW
    return NULL;
}

bool DBFilter::isTableExist(const std::string& DBName, const std::string& tableName) {
    return true;
}

bool DBFilter::getExistingFilterResults(const std::string& aResultTableName, std::list<BaseResultData>& outFilterResults) {
    int ret = 0;
    sqlite3_stmt* stmt = NULL;
    std::string sql;
    std::string columns("");

    columns += STRING_DATE;
    columns += ",";
    columns += STRING_TURNOVER_SALE;
    columns += ",";
    columns += STRING_TURNOVER_BUY;
    columns += ",";
    columns += STRING_TURNOVER_FLOWIN_ONE_DAY;
    columns += ",";
    columns += STRING_VOLUME_FLOWIN_ONE_DAY;

    sql = SELECT_COLUMNS_IN_ORDER(aResultTableName, columns, STRING_DATE, false);
    ret = sqlite3_prepare(mOriginDB,
                          sql.c_str(),
                          -1,
                          &stmt,
                          NULL);
    
    if (ret != SQLITE_OK) {
        LOGE(LOGTAG, "Fail to prepare stmt for getExistingFilterResults from table:%s, :%s", aResultTableName.c_str(), sqlite3_errmsg(mOriginDB));
        return false;
    }

    int i = 0;
    BaseResultData tempBaseResultData;
     
    double buyTurnOver = 0;
    double saleTurnOver = 0;
    // get as many filter results as we can, because we need the filter results of DAYS_BEFORE_LEV3
    // days before to compute filter result of the current day.
    while (sqlite3_step(stmt) == SQLITE_ROW && i < DAYS_BEFORE_LEV3) {
        std::string date = (char*)sqlite3_column_text(stmt, 0);
        saleTurnOver = sqlite3_column_double(stmt, 1);
        buyTurnOver  = sqlite3_column_double(stmt, 2);
        //XXX: No matter if there is any valide result in the result-table, every day
        //     is counted in. Anyway, one day is one day.
        tempBaseResultData.mDate = date;
        tempBaseResultData.mSaleTurnOver = saleTurnOver;
        tempBaseResultData.mBuyTurnOver = buyTurnOver;
        tempBaseResultData.mTurnOverFlowInOneDay = sqlite3_column_double(stmt, 3);
        tempBaseResultData.mVolumeFlowInOneDay = sqlite3_column_int(stmt, 4);
        LOGD(LOGTAG, "tempBaseResultData.mVolumeFlowInOneDay:%d", tempBaseResultData.mVolumeFlowInOneDay);
        outFilterResults.push_back(tempBaseResultData);
        i++;
    }

    ret = sqlite3_finalize(stmt);
    if (ret != SQLITE_OK) {
        LOGE(LOGTAG, "Fail to finalize the stmt to getExistingFilterRsults from Table:%s",  aResultTableName.c_str());
        return false;
    }

    return true;
}

bool DBFilter::computeFilterResultForLev(int aDaysBefore, std::list<BaseResultData>& aExistingBaseResults, BaseResultData& inoutBaseResult) {
    //TODO: Find a better way to work around, for the sake of the std::list<T>::end() includes nothing.
    int i = 0;
    double sumTurnOverFlowin = 0.0;
    int sumVolumeFlowin = 0;
    std::list<DBFilter::BaseResultData>::iterator itr;

    // Step 1: Count in the newest filter result;
    if (!mBaseResultDatas.empty()) {
        itr = --(mBaseResultDatas.end());
        for (i = 0; i < aDaysBefore; itr--) {
            LOGD(LOGTAG, "Current TurnOver sale:%f, buy:%f, diff:%f", (*itr).mSaleTurnOver, (*itr).mBuyTurnOver, (*itr).mTurnOverFlowInOneDay);
            sumTurnOverFlowin += (*itr).mTurnOverFlowInOneDay;
            sumVolumeFlowin   += (*itr).mVolumeFlowInOneDay;
            LOGD(LOGTAG, "sumTurnOverFlowin:%f, sumVolumeFlowin:%d",  sumTurnOverFlowin, sumVolumeFlowin);
            LOGD(LOGTAG, "aDaysBefore:%d, i:%d", aDaysBefore, i);

            i++;
            if (i >=aDaysBefore 
                || itr == mBaseResultDatas.begin()) {
                break;
            }
        }
    }

    // Step 2: Count in the exsiting filter results 
    if (!aExistingBaseResults.empty()) {
        //FIXME: Because the aExistingBaseResults is in desc order, so count from begin to end.
        itr = aExistingBaseResults.begin();
        LOGD(LOGTAG, "existingBaseResults size:%d, i:%d", aExistingBaseResults.size(), i);
        while (i < aDaysBefore && itr != aExistingBaseResults.end()) {
            LOGD(LOGTAG, "sumTurnOverFlowin:%f, sumVolumeFlowin:%d",  sumTurnOverFlowin, sumVolumeFlowin);
            LOGD(LOGTAG, "OneDay TurnOverFlowin:%lf, OneDayVolumeFlowin:%d",  (*itr).mTurnOverFlowInOneDay, (*itr).mVolumeFlowInOneDay);
            sumTurnOverFlowin += (*itr).mTurnOverFlowInOneDay;
            sumVolumeFlowin   += (*itr).mVolumeFlowInOneDay;
            LOGD(LOGTAG, "Existing TurnOver sale:%f, buy:%f, diff:%f, date:%s", (*itr).mSaleTurnOver, (*itr).mBuyTurnOver, (*itr)mTurnOverFlowInOneDay, (*itr).mDate.c_str())
            LOGD(LOGTAG, "sumTurnOverFlowin:%lf, sumVolumeFlowin:%d",  sumTurnOverFlowin, sumVolumeFlowin);
            LOGD(LOGTAG, "aDaysBefore:%d, i:%d", aDaysBefore, i);
            i++;
            itr++;
        }
    }

    LOGD(LOGTAG, "sumTurnOverFlowin:%f, sumVolumeFlowin:%f",  sumTurnOverFlowin, sumVolumeFlowin);
    switch (aDaysBefore) {
      case DAYS_BEFORE_LEV1:
          inoutBaseResult.mTurnOverFlowInFiveDays += sumTurnOverFlowin;
          inoutBaseResult.mVolumeFlowInFiveDays += sumVolumeFlowin;
          break;
      case DAYS_BEFORE_LEV2:
          inoutBaseResult.mTurnOverFlowInTenDays += sumTurnOverFlowin;
          inoutBaseResult.mVolumeFlowInTenDays += sumVolumeFlowin;
          break;
      case DAYS_BEFORE_LEV3:
          inoutBaseResult.mTurnOverFlowInMonDays += sumTurnOverFlowin;
          inoutBaseResult.mVolumeFlowInMonDays += sumVolumeFlowin;
          break;
      default:
          LOGE(LOGTAG, "ComputeFilterResultFor unkown Level:%d", aDaysBefore);
          return false;
    }

    //TODO: Optimization of Figurout flowin in 10 days
    //if (mBaseResultDatas.size() > DAYS_BEFORE_LEV1) {
    //    for (i = 0, itr = mBaseResultDatas.end(); i < DAYS_BEFORE_LEV1 && itr != mBaseResultDatas.begin(); itr--) {
    //        if ((*itr).mBuyTurnOver > MIN_TURNOVER && (*itr).mSaleTurnOver > MIN_TURNOVER) {
    //            i++;
    //        }
    //    }
    //        LOGD(LOGTAG, "end mTurnOverFlowInTenDays:%f, DAYS_BEFORE_LEV1 pre mTurnOverFlowInTenDays:%f", (*mBaseResultDatas.end()).mTurnOverFlowInTenDays,  (*itr).mTurnOverFlowInTenDays);
    //    tempBaseResultData.mTurnOverFlowInTenDays = (*mBaseResultDatas.end()).mTurnOverFlowInTenDays + ((*mBaseResultDatas.end()).mTurnOverFlowInTenDays - (*itr).mTurnOverFlowInTenDays);
    //} else {
    //    for (i = 0, itr = mBaseResultDatas.end(); i < DAYS_BEFORE_LEV1 && itr != mBaseResultDatas.begin(); itr--) {
    //        if ((*itr).mBuyTurnOver > MIN_TURNOVER && (*itr).mSaleTurnOver > MIN_TURNOVER) {
    //            tempBaseResultData.mTurnOverFlowInTenDays += (*itr)mTurnOverFlowInOneDay;
    //            i++;
    //        }
    //    }
    //}

    return true;
}

bool DBFilter::computeResultFromTable(const std::string& aMiddleWareTableName,
                                      const std::string& aResultTableName,
                                      const std::string& aOriginTableName,
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
    columns += STRING_SALE_BUY;

    keyColumn += STRING_SALE_BUY;

    sql = SELECT_COLUMNS_IN_GROUP(aMiddleWareTableName, columns, keyColumn);

    ret = sqlite3_prepare(mOriginDB,
                          sql.c_str(),
                          -1,
                          &stmt,
                          NULL);
    if (ret != SQLITE_OK) {
        LOGE(LOGTAG, "Fail to getExistingBaseResults, err:%s", sqlite3_errmsg(mOriginDB));
        return false;
    }

    std::list<BaseResultData> existingBaseResults;
    if (!getExistingFilterResults(aResultTableName, existingBaseResults)) {
        LOGE(LOGTAG, "Fail to getExistingBaseResults, err:%s", sqlite3_errmsg(mOriginDB));
        return false;
    }

    {
        //FIXME: The first raw is Buy, second raw is Sale
        BaseResultData tempBaseResultData;
        while(sqlite3_step(stmt) == SQLITE_ROW) {
            std::string isBuy("");
            isBuy= (char*)sqlite3_column_text(stmt, 2);
            LOGD(LOGTAG, "isBuy:%s, db:%s", isBuy.c_str(), aDBName.c_str());

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
                LOGD(LOGTAG, "isBuy:%s", isBuy.c_str());
                return false;
            }
        }
        tempBaseResultData.mDate = aOriginTableName;
        tempBaseResultData.mTurnOverFlowInOneDay = tempBaseResultData.mBuyTurnOver - tempBaseResultData.mSaleTurnOver;
        LOGD(LOGTAG, "turnover: sale:%f, buy:%f, diff:%f", tempBaseResultData.mSaleTurnOver, tempBaseResultData.mBuyTurnOver, tempBaseResultData.mBuyTurnOver - tempBaseResultData.mSaleTurnOver);
        tempBaseResultData.mVolumeFlowInOneDay = tempBaseResultData.mBuyVolume - tempBaseResultData.mSaleVolume;
        LOGD(LOGTAG, "turnover: sale:%f, buy:%f, diff:%f", tempBaseResultData.mSaleVolume, tempBaseResultData.mBuyVolume, tempBaseResultData.mBuyVolume - tempBaseResultData.mSaleVolume);

        tempBaseResultData.mBeginPrice = beginningPrice;
        tempBaseResultData.mEndPrice   = endingPrice;
        //
        mBaseResultDatas.push_back(tempBaseResultData);

        LOGD(LOGTAG, " mBaseResultDatas size:%d, i:%d", mBaseResultDatas.size(), i);

        computeFilterResultForLev(DAYS_BEFORE_LEV1, existingBaseResults, tempBaseResultData);
        computeFilterResultForLev(DAYS_BEFORE_LEV2, existingBaseResults, tempBaseResultData);
        computeFilterResultForLev(DAYS_BEFORE_LEV3, existingBaseResults, tempBaseResultData);
        mBaseResultDatas.pop_back();
        mBaseResultDatas.push_back(tempBaseResultData);
    }

    ret = sqlite3_finalize(stmt);
    if (ret != SQLITE_OK) {
        LOGE(LOGTAG, "Fail to finalize the stmt to finalize tmpTable:%s for originTable:%s", aMiddleWareTableName.c_str(), aOriginTableName.c_str());
        LOGE(LOGTAG, "Error message:%s", sqlite3_errmsg(mOriginDB));
        return true;
    }

    return true;
}

bool DBFilter::saveBaseResultInBatch(const std::string& aResultTableName) {
    int ret = 0;

    //Format DES
    std::list<std::string> descriptions;
    std::string singleDes;
    singleDes = TABLE_FORMAT_FILTER_RESULT;
    singleDes += D_STMT_FORMAT_FILTER_RESULT;
    descriptions.push_back(singleDes);

///    //Result data DES
///    std::list<DBFilter::BaseResultData>::iterator iterOfFilterResult;
///
///    for (iterOfFilterResult = mBaseResultDatas.begin(); iterOfFilterResult != mBaseResultDatas.end(); iterOfFilterResult++) {
///        std::string values("");
///        values += "(";
///        values += (*iterOfFilterResult).mSaleVolume;
///        values += ", ";
///        values += (*iterOfFilterResult).mBuyVolume;
///        values += ", ";
///        values += (*iterOfFilterResult).mSaleTurnOver;
///        values += ", ";
///        values += (*iterOfFilterResult).mBuyTurnOver;
///        values += ", ";
///        values += (*iterOfFilterResult).mSalePrice;
///        values += ", ";
///        values += (*iterOfFilterResult).mBuyPrice;
///        values += ", ";
///        values += (*iterOfFilterResult).mTurnOverFlowInOneDay;
////*
///        values += ", ";
///        values += (*iterOfFilterResult).mTurnOverFlowInTenDays;
///*/
///        values += ")";
///    }
    

    DBWrapper::insertFilterResultsInBatch(mDBName, aResultTableName, descriptions, mBaseResultDatas, NULL);
    mBaseResultDatas.clear();

    return true;
}

bool DBFilter::removeTable(const std::string& tableName) {
    int ret = 0;
    std::string sql("");
    sqlite3_stmt* stmt = NULL;

    sql = REMOVE_TABLE(tableName);

    ret = sqlite3_prepare(mOriginDB,
                          sql.c_str(),
                          -1,
                          &stmt,
                          NULL);

    if (ret != SQLITE_OK) {
        LOGE(LOGTAG, "Fail to prepare stmt to clearTable:%s", tableName.c_str());
        return false;
    }

    ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE) {
        return false;
    }

    ret = sqlite3_finalize(stmt);
    if (ret != SQLITE_OK) {
        LOGE(LOGTAG, "Fail to finalize the stmt to finalize table:%s", tableName.c_str());
        return false;
    }

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
        LOGE(LOGTAG, "Fail to prepare stmt to clearTable:%s", tableName.c_str());
        return false;
    }

    ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE) {
        return false;
    }

    ret = sqlite3_finalize(stmt);
    if (ret != SQLITE_OK) {
        LOGE(LOGTAG, "Fail to finalize the stmt to finalize table:%s", tableName.c_str());
        return false;
    }

    return true;
}

bool DBFilter::getBeginAndEndPrice(const std::string& aOriginTableName, double& beginningPrice, double& endingPrice) {
    int ret = -1;
    std::string sql;
    std::string targetColumns = " Price ";
    sqlite3_stmt* stmt = NULL;

    sql = SELECT_COLUMNS(aOriginTableName, targetColumns);
    LOGD(LOGTAG, "getBeginAndEndPrice, sql:%s", sql.c_str());
    ret = sqlite3_prepare(mOriginDB,
                          sql.c_str(),
                          -1,
                          &stmt,
                          NULL);

    if (ret != SQLITE_OK) {
        LOGD(LOGTAG, "Fail to prepare stmt to select end Price for table:%s", aOriginTableName.c_str());
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
        LOGD(LOGTAG, "Fail to finalize the stmt to finalize table:%s", aOriginTableName.c_str());
        return false;
    }

    return true;
}

bool DBFilter::filterTableByTurnOver(const std::string& aOriginTableName, const int aMinTurnOver) {
    LOGD(LOGTAG, "filterTableByTurnOver for table:%s", aOriginTableName.c_str());
    char strTurnOver[8] = {0};
    sprintf(strTurnOver, "%d", aMinTurnOver);

    sqlite3_stmt* stmt = NULL;
    int ret = -1;
    std::string targetColumns = "";
    targetColumns += STRING_VOLUME;
    targetColumns += ",";
    targetColumns += STRING_TURNOVER;
    targetColumns += ",";
    targetColumns += STRING_SALE_BUY;

    std::string sql = SELECT_TURNOVER_INTO(aOriginTableName, mTmpResultTableName, targetColumns, strTurnOver);
    LOGD(LOGTAG, "sql:%s", sql.c_str());

    ret = sqlite3_prepare(mOriginDB,
                          sql.c_str(),
                          -1,
                          &stmt,
                          NULL);

    if (ret != SQLITE_OK) {
        LOGE(LOGTAG, "Fail to prepare stmt to filterTable:%s", aOriginTableName.c_str());
        return false;
    }

    ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE) {
        return false;
    }

    ret = sqlite3_finalize(stmt);
    if (ret != SQLITE_OK) {
        LOGE(LOGTAG, "Fail to finalize the stmt to finalize table:%s", aOriginTableName.c_str());
        return false;
    }

    return true;
}

static bool isOriginTable(const std::string& aTableName) {
    //XXX: All the non-origin-tables have been removed in the step 2 of
    //     DBFilter::filterOriginDBByTurnOver, so return true direcly
    return true;
}

bool DBFilter::filterTablesByTurnOver(const std::string& aResultTableName, const int aMinTurnover, std::list<std::string>& aOriginTableNames) {
    //FIXME: Assuming that process all the tables in one DB and then the tables of the other DB.
    if (aOriginTableNames.size() < 1) {
        return true;
    }

    std::list<std::string>::iterator iterOfTableName;
    double beginningPrice, endingPrice;
    for (iterOfTableName = aOriginTableNames.begin(); iterOfTableName != aOriginTableNames.end(); iterOfTableName++) {
         if (isOriginTable(*iterOfTableName)) {
             //Step 1: get Beginning & ending Price
             beginningPrice = endingPrice = 0.0;
             if (!getBeginAndEndPrice(*iterOfTableName, beginningPrice, endingPrice)) {
                 LOGE(LOGTAG, "Fail to getBeginAndEndPrice table:%s in DB:%s", (*iterOfTableName).c_str(), mDBName.c_str());
                 return false;
             }

             //Step 2: filter
             if (!filterTableByTurnOver(*iterOfTableName, aMinTurnover)) {
                 LOGE(LOGTAG, "Fail to filterTableByTurnOver table:%s in DB:%s", (*iterOfTableName).c_str(), mDBName.c_str());
                 return false;
             }

             //Step 3: compute && save result 
             if (!computeResultFromTable(mTmpResultTableName, aResultTableName, *iterOfTableName, beginningPrice, endingPrice)) {
                 LOGE(LOGTAG, "Fail to computeResultFromTable table:%s in DB:%s", (*iterOfTableName).c_str(), mDBName.c_str());
                 return false;
             }

             if (!clearTable(mTmpResultTableName)) {
                 return false;
             }
         }
    }

    ////Step 4: save to final result 
    if (!saveBaseResultInBatch(aResultTableName)) {
        LOGE(LOGTAG, "Fail to saveBaseResultInBatch table:%s in DB:%s", (*iterOfTableName).c_str(), mDBName.c_str());
        return false;
    }

    return true;
}

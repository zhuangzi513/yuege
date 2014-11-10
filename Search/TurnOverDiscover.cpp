
#include "TurnOverDiscover.h"
#include "DBFilter.h"
#include "DBOperations.h"
#include "DBWrapper.h"
#include "ErrorDefines.h"

#include <stdio.h>

#define DATE                " Date "
#define ALL_COLUMNS         " * "
#define DAYS_BEFORE_LEV3    20
#define DAYS_BEFORE_LEV2    10
#define DAYS_BEFORE_LEV1    5

#define LOGTAG "TurnOverDiscover"

const float LIMIT_RATIO = 1.1;
const std::string TurnOverDiscover::mBankerResultTable = "BankerResultTable";

static std::string SELECT_COLUMNS(const std::string& tableName, const std::string& targetColumns) {
    std::string command("");
    command += " SELECT ";
    command += targetColumns;
    command += " FROM ";
    command += tableName;
    return command;
}

static std::string SELECT_COLUMNS_BY_ORDER(const std::string& tableName, const std::string& targetColumns, const std::string& key, bool desOrder) {
    std::string command("");
    command += " SELECT ";
    command += targetColumns;
    command += " FROM ";
    command += tableName;
    command += " ORDER BY ";
    command += key;
    if (desOrder) {
        command += " DESC ";
    }
    return command;
}

static std::string SELECT_COLUMNS_FOR_DAY(const std::string& tableName, const std::string& targetColumns, const std::string& dateNO, bool desOrder) {
    std::string command("");
    command += " SELECT ";
    command += targetColumns;
    command += " FROM ";
    command += tableName;
    command += " WHERE ";
    command += DATE;
    command += " == ";
    command += dateNO;

    return command;
}


TurnOverDiscover::TurnOverDiscover(const std::string& aDBName, const std::string& aTableName)
        : mDBName(aDBName)
        , mTargetResultTableName(aTableName) {
    init();
    //updateBankerResultTable();
}

TurnOverDiscover::~TurnOverDiscover() {
    DBWrapper::closeDB(mDBName);
}

bool TurnOverDiscover::isDBBankedInDays(int aCount) {
    bool ret = false;
    ret = DBWrapper::openTable(DBWrapper::FILTER_BANDKE_TABLE, mDBName, mBankerResultTable);
    if (!ret) {
        LOGE(LOGTAG, "Cannot open table :%s in DB:%s", mBankerResultTable.c_str(), mDBName.c_str());
        return false;
    }

    std::string sql, targetColumns;
    sqlite3_stmt* stmt = NULL;
    int intRet = -1;

    targetColumns += ALL_COLUMNS;
    sql = SELECT_COLUMNS_BY_ORDER(mBankerResultTable, targetColumns, DATE, true);

    LOGD(LOGTAG, "sql:%s", sql.c_str());
    intRet = sqlite3_prepare_v2(mOriginDB,
                          sql.c_str(),
                          -1,
                          &stmt,
                          NULL);
    if (intRet != SQLITE_OK) {
        LOGE(LOGTAG, "Fails to prepare stmt for DB:%s, mOriginDB:%p", mDBName.c_str(), mOriginDB);
        LOGE(LOGTAG, "Error message:%s", sqlite3_errmsg(mOriginDB));
        return false;
    }

    int i = 0, bankerInchargecount = 0, positiveCount = 0;
    while(sqlite3_step(stmt) == SQLITE_ROW && i < aCount) {
        std::string isBankerInchage = (char*)sqlite3_column_text(stmt, 1);
        std::string isPositive = (char*)sqlite3_column_text(stmt, 2);
        if (isBankerInchage == "True") {
            ++bankerInchargecount;
        }

        if (isPositive == "True") {
            ++positiveCount;
        }
        ++i;
    }

    if (SQLITE_OK != sqlite3_finalize(stmt)) {
        LOGE(LOGTAG, "Fail to finalize stmt in isDBBankedInDays mDBName:%s", mDBName.c_str());
        DBWrapper::closeDB(mDBName);
        return false;
    }

    LOGD(LOGTAG, "count:%d , DB:%s", bankerInchargecount, mDBName.c_str());
    if (((bankerInchargecount * 2) < aCount)) {
        //DBWrapper::closeDB(mDBName);
        return false;
    }

    return ((positiveCount * 3) > (2 * bankerInchargecount));
}

void TurnOverDiscover::updateBankerResultTable() {
    bool ret = checkNewAddedOriginTables();
    if (!ret) {
        return;
    }

    std::list<std::string>::iterator itr = mNewAddedOriginTables.begin();
    std::list<BankerResultInfo> bankerResultInfos;
    while (itr != mNewAddedOriginTables.end()) {
        BankerResultInfo tmpBankerResultInfo;
        LOGD(LOGTAG, "new added origin tables:%s, DB:%s", itr->c_str(), mDBName.c_str());
        mBankerTurnOvers[0] = mBankerTurnOvers[1] = 0;
        mSumTurnOvers[0] = mSumTurnOvers[1] = 0;
        getBankerInChargeInfoFromOriginTable(*itr);

        if (isTodayBankerInCharge(*itr)) {
            tmpBankerResultInfo.mIsBankerIncharge = "True";
        } else {
            tmpBankerResultInfo.mIsBankerIncharge = "False";
        }

        if (mBankerTurnOvers[0] > 0 && mBankerTurnOvers[1] > 0) {
            if (isTodayPositiveBankerInCharge(*itr)) {
                tmpBankerResultInfo.mIsPositive = "True";
            } else {
                tmpBankerResultInfo.mIsPositive = "False";
            }
            tmpBankerResultInfo.mBuyToSale = mBankerTurnOvers[0]/mBankerTurnOvers[1];
        }
        tmpBankerResultInfo.mDate = *itr;
        bankerResultInfos.push_back(tmpBankerResultInfo);
        LOGD(LOGTAG, "tmpBankerResultInfo:mDate:%s, mIsBankerIncharge:%s, mIsPositive:%s, mBuyToSale:%f ", itr->c_str(), tmpBankerResultInfo.mIsBankerIncharge.c_str(), tmpBankerResultInfo.mIsPositive.c_str(), tmpBankerResultInfo.mBuyToSale);
        ++itr;
    }

    std::list<std::string> descriptions;
    std::string singleDes;
    singleDes = TABLE_FORMAT_BANKER;
    singleDes += D_STMT_FORMAT_BANKER;
    descriptions.push_back(singleDes);
    DBWrapper::insertBankerResultsInBatch(mDBName, mBankerResultTable, descriptions, bankerResultInfos, NULL);
}

bool TurnOverDiscover::checkNewAddedOriginTables() {
    bool ret = false;

    if (mDBName.empty()
        || mTargetResultTableName.empty()
        || mBankerResultTable.empty()) {
        return false;
    }

    ret = DBWrapper::openTable(DBWrapper::FILTER_BANDKE_TABLE, mDBName, mBankerResultTable);
    if (ret == DBWrapper::FAIL_OPEN_TABLE) {
        DBWrapper::closeDB(mDBName);
        return false;
    }

   
    //Step 1: get the filtered origin tables
    std::list<std::string> filteredOriginTables;
    std::string sql, targetColumns;
    sqlite3_stmt* stmt = NULL;
    int intRet = -1;

    targetColumns += DATE;
    sql = SELECT_COLUMNS(mBankerResultTable, targetColumns);

    LOGD(LOGTAG, "sql:%s", sql.c_str());
    intRet = sqlite3_prepare_v2(mOriginDB,
                          sql.c_str(),
                          -1,
                          &stmt,
                          NULL);
    LOGD(LOGTAG, "prepare stmt to Table:%s, intRet:%d, mOriginDB:%p", mBankerResultTable.c_str(), intRet, mOriginDB);

    if (intRet != SQLITE_OK) {
        LOGE(LOGTAG, "Fail to prepare stmt to Table:%s, intRet:%d, errorMessage:%s", mBankerResultTable.c_str(), intRet, sqlite3_errmsg(mOriginDB));
        DBWrapper::closeDB(mDBName);
        return false;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string dateOfOriginTable = (const char*)sqlite3_column_text(stmt, 0);
        filteredOriginTables.push_back(dateOfOriginTable);
    }

    if (SQLITE_OK != sqlite3_finalize(stmt)) {
        LOGE(LOGTAG, "Fail to finalize stmt in updateFilterResultByTurnOver aDBName:%s", mDBName.c_str());
        DBWrapper::closeDB(mDBName);
        return false;
    }

    //Step 2: get the new added origin tables
    std::list<std::string> originTables;
    ret = DBWrapper::getAllTablesOfDB(mDBName, originTables);
    if (!ret) {
        DBWrapper::closeDB(mDBName);
        return false;
    }

    //FIXME: Hack here, the name of origin-table starts from 'O'.
    //       result-tables start from 'F' and middleware result-table
    //       starts from 'M'. 
    std::list<std::string> existingResultTables;
    while (true) {
        if (originTables.front() <= DBFilter::mTmpResultTableName
            && originTables.size() > 0) {
            existingResultTables.push_back(originTables.front());
            originTables.pop_front();
        } else {
            break;
        }
    }

    std::list<std::string>::iterator itrAll = originTables.begin();
    std::list<std::string>::iterator itrFiltered = filteredOriginTables.begin();
    while (itrFiltered != filteredOriginTables.end()
           && itrAll != existingResultTables.end()) {
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
               mNewAddedOriginTables.push_back(*itrAll);
               LOGD(LOGTAG, "Find a new added Table:%s, DB:%s", (*itrAll).c_str(), mDBName.c_str());
               itrAll++;
               continue;
           } else {
               // we should not get here
               itrFiltered++;
               continue;
           }
    }

    while (itrAll != originTables.end()) {
        mNewAddedOriginTables.push_back(*itrAll);
        LOGD(LOGTAG, "New added Table:%s, DB:%s", (*itrAll).c_str(), mDBName.c_str());
        itrAll++;
    }

    if (mNewAddedOriginTables.size() < 1) {
        LOGD(LOGTAG, "No any new added OriginTables, dbName:%s", mDBName.c_str());
        DBWrapper::closeDB(mDBName);
        return false;
    }

    return true;
}

bool TurnOverDiscover::isDBBuyMoreThanSale(const std::string& aDBName) {
    return true;
}

bool TurnOverDiscover::isDBFlowIn(const std::string& aDBName) {
    if (!isDBFlowInFiveDays()) {
        return false;
    }

    if (!isDBFlowInTenDays()) {
        return false;
    }

    if (!isDBFlowInMonDays()) {
        return false;
    }

    return true;
}

bool TurnOverDiscover::isDBFlowInFiveDays() {
    std::list<std::string>::iterator itrFilterResultTable;
    itrFilterResultTable = DBFilter::mResultTableNames.begin();

    while (itrFilterResultTable != DBFilter::mResultTableNames.end()) {
        //if (!isTableFlowInFiveDays(*itrFilterResultTable)) {
        //    return false;
        //}
        itrFilterResultTable++;
    }

    return true;
}

bool TurnOverDiscover::isDBFlowInTenDays() {
    return true;
}

bool TurnOverDiscover::isDBFlowInMonDays() {
    return true;
}

void TurnOverDiscover::getBankerInChargeInfoFromOriginTable(const std::string& aOriginTableName) {
    bool ret = false;

    ret = DBWrapper::getSumTurnOverOfTable(mDBName, aOriginTableName, mSumTurnOvers);
    if (!ret) {
        LOGD(LOGTAG, "Get in isDBBankerInCharging:%s", mDBName.c_str());
        DBWrapper::closeDB(mDBName);
        return;
    }
    LOGD(LOGTAG, "sum buy turnover:%lf, sale turnover:%lf", mSumTurnOvers[0], mSumTurnOvers[1]);

    ret = DBWrapper::getBankerTurnOverOfTable(mDBName, aOriginTableName, mBankerTurnOvers);
    if (!ret) {
        LOGD(LOGTAG, "Get in isDBBankerInCharging:%s", mDBName.c_str());
        DBWrapper::closeDB(mDBName);
        return;
    }
    LOGD(LOGTAG, "banker buy turnover:%lf, sale turnover:%lf", mBankerTurnOvers[0], mBankerTurnOvers[1]);
}

bool TurnOverDiscover::getBankerInChargeInfoFromBankerResultTable(const std::string& aOriginTableName, BankerResultInfo& outBankerResultInfo) {
    bool ret = false;
    ret = DBWrapper::openTable(DBWrapper::FILTER_BANDKE_TABLE, mDBName, mBankerResultTable);
    if (!ret) {
        LOGE(LOGTAG, "Cannot open table :%s in DB:%s", mBankerResultTable.c_str(), mDBName.c_str());
        return false;
    }
 
    std::string sql, targetColumns;
    sqlite3_stmt* stmt = NULL;
    int intRet = -1;
 
    targetColumns += ALL_COLUMNS;
    sql = SELECT_COLUMNS_FOR_DAY(mBankerResultTable, targetColumns, aOriginTableName, true);
 
    LOGD(LOGTAG, "sql:%s", sql.c_str());
    intRet = sqlite3_prepare_v2(mOriginDB,
                          sql.c_str(),
                          -1,
                          &stmt,
                          NULL);
    if (intRet != SQLITE_OK) {
        LOGE(LOGTAG, "Fails to prepare stmt for DB:%s, mOriginDB:%p", mDBName.c_str(), mOriginDB);
        LOGE(LOGTAG, "Error message:%s", sqlite3_errmsg(mOriginDB));
        return false;
    }
 
    std::string isBankerInchage, isPositive;
    int i = 0;
    while(sqlite3_step(stmt) == SQLITE_ROW) {
        outBankerResultInfo.mDate = (char*)sqlite3_column_text(stmt, 0);
        outBankerResultInfo.mIsBankerIncharge = (char*)sqlite3_column_text(stmt, 1);
        outBankerResultInfo.mIsPositive = (char*)sqlite3_column_text(stmt, 2);
        outBankerResultInfo.mBuyToSale = (double)sqlite3_column_double(stmt, 3);
        ++i;
    }

    if (i > 1) {
        LOGE(LOGTAG, "We have more than one recoder in table :%s for Date:%s", mBankerResultTable.c_str(), aOriginTableName.c_str());
        return false;
    }

#if DEBUG
    LOGD(LOGTAG, "Dump the BankerResultInfo returned by getBankerInChargeInfoFromBankerResultTable()");
    LOGD(LOGTAG, "mDate:%s", outBankerResultInfo.mDate.c_str());
    LOGD(LOGTAG, "mIsBankerInchage:%s", outBankerResultInfo.mIsBankerInchage.c_str());
    LOGD(LOGTAG, "mIsPositive:%s", outBankerResultInfo.mIsPositive.c_str());
    LOGD(LOGTAG, "mBuyToSale:%lf", outBankerResultInfo.mBuyToSale);
#endif
 
    if (SQLITE_OK != sqlite3_finalize(stmt)) {
        LOGE(LOGTAG, "Fail to finalize stmt in isDBBankedInDays mDBName:%s", mDBName.c_str());
        DBWrapper::closeDB(mDBName);
        return false;
    }
    return true;
}

bool TurnOverDiscover::isTodayBankerInCharge(const std::string& aOriginTableName) {
    //BankerTurnover > 50% of SumTurnOver
    double sumBankerTurnOver = mBankerTurnOvers[0] + mBankerTurnOvers[1];
    double sumTurnOver = mSumTurnOvers[0] + mSumTurnOvers[1];
    if ((sumBankerTurnOver * 2) < sumTurnOver) {
        return false;
    }

    return true;
}

bool TurnOverDiscover::isTodayPositiveBankerInCharge(const std::string& aOriginTableName) {
    if (mBankerTurnOvers.size() < 2
        || mSumTurnOvers.size() < 2) {
        return false;
    }

    double sumBankerTurnOver = mBankerTurnOvers[0] + mBankerTurnOvers[1];
    double sumTurnOver = mSumTurnOvers[0] + mSumTurnOvers[1];
    LOGD(LOGTAG, "banker buy turnover:%lf, sale turnover:%lf", mBankerTurnOvers[0], mBankerTurnOvers[1]);

    // BankerBuy > 1.1 * BankerSale
    if (mBankerTurnOvers[0] > (mBankerTurnOvers[1] * LIMIT_RATIO)) {
        return true;
    }

    return false;
}

bool TurnOverDiscover::isTodayNagtiveBankerInCharge(const std::string& aOriginTableName) {
    if (mBankerTurnOvers.size() < 2
        || mSumTurnOvers.size() < 2) {
        return false;
    }

    double sumBankerTurnOver = mBankerTurnOvers[0] + mBankerTurnOvers[1];
    double sumTurnOver = mSumTurnOvers[0] + mSumTurnOvers[1];
    LOGD(LOGTAG, "banker buy turnover:%lf, sale turnover:%lf", mBankerTurnOvers[0], mBankerTurnOvers[1]);

    if ((2 * sumBankerTurnOver) < sumTurnOver) {
        return false;
    }

    if (mBankerTurnOvers[1] > (mBankerTurnOvers[0] * LIMIT_RATIO)) {
        return true;
    }

    return false;
}

bool TurnOverDiscover::isTodayNeutralBankerInCharge(const std::string& aOriginTableName) {
    if (mBankerTurnOvers.size() < 2
        || mSumTurnOvers.size() < 2) {
        return false;
    }

    double sumBankerTurnOver = mBankerTurnOvers[0] + mBankerTurnOvers[1];
    double sumTurnOver = mSumTurnOvers[0] + mSumTurnOvers[1];
    LOGD(LOGTAG, "banker buy turnover:%lf, sale turnover:%lf", mBankerTurnOvers[0], mBankerTurnOvers[1]);

    if ((2 * sumBankerTurnOver) < sumTurnOver) {
        return false;
    }

    if (mBankerTurnOvers[0] < (mBankerTurnOvers[1] * LIMIT_RATIO)
        || mBankerTurnOvers[1] < (mBankerTurnOvers[0] * LIMIT_RATIO)) {
        return true;
    }

    return false;
}

void TurnOverDiscover::getBankerTurnOvers(std::vector<double>& outBankerTurnOvers) const {
    outBankerTurnOvers = mBankerTurnOvers;
}

bool TurnOverDiscover::isTodaySuckIn(const std::string& aOriginTableName) {
    if (mBankerTurnOvers.size() < 2
        || mSumTurnOvers.size() < 2) {
        return false;
    }

    BankerResultInfo resultInfo;
    getBankerInChargeInfoFromBankerResultTable(aOriginTableName, resultInfo);
    if (resultInfo.mIsBankerIncharge == "False") {
        if (resultInfo.mIsPositive == "True") {
            return true;
        }
    }
    
    return false;
}

bool TurnOverDiscover::isPreviousDaysSuckIn(const int aCount) {
    bool ret = false;
    ret = DBWrapper::openTable(DBWrapper::FILTER_BANDKE_TABLE, mDBName, mBankerResultTable);
    if (!ret) {
        LOGE(LOGTAG, "Cannot open table :%s in DB:%s", mBankerResultTable.c_str(), mDBName.c_str());
        return false;
    }

    std::string sql, targetColumns;
    sqlite3_stmt* stmt = NULL;
    int intRet = -1;

    targetColumns += ALL_COLUMNS;
    sql = SELECT_COLUMNS_BY_ORDER(mBankerResultTable, targetColumns, DATE, true);

    LOGD(LOGTAG, "sql:%s", sql.c_str());
    intRet = sqlite3_prepare_v2(mOriginDB,
                          sql.c_str(),
                          -1,
                          &stmt,
                          NULL);
    if (intRet != SQLITE_OK) {
        LOGE(LOGTAG, "Fails to prepare stmt for DB:%s, mOriginDB:%p", mDBName.c_str(), mOriginDB);
        LOGE(LOGTAG, "Error message:%s", sqlite3_errmsg(mOriginDB));
        return false;
    }

    int i = 0, isSuckInCount = 0;
    while(sqlite3_step(stmt) == SQLITE_ROW && i < aCount) {
        std::string isBankerInchage = (char*)sqlite3_column_text(stmt, 1);
        std::string isPositive = (char*)sqlite3_column_text(stmt, 2);
        LOGD(LOGTAG, "isBankerInchage:%s", isBankerInchage.c_str());
        LOGD(LOGTAG, "isPositive:%s", isPositive.c_str());
        if (isBankerInchage == "False") {
            if (isPositive == "True") {
                ++isSuckInCount;
            }
        }
        ++i;
    }

    if (SQLITE_OK != sqlite3_finalize(stmt)) {
        LOGE(LOGTAG, "Fail to finalize stmt in isDBBankedInDays mDBName:%s", mDBName.c_str());
        //DBWrapper::closeDB(mDBName);
        return false;
    }

    LOGD(LOGTAG, "aCount:%d isSuckInCount:%d , DB:%s", aCount, isSuckInCount, mDBName.c_str());
    if (((isSuckInCount * 1.7) < aCount)) {
        DBWrapper::closeDB(mDBName);
        return false;
    }

    return true;
}

//private
void TurnOverDiscover::init() {
    mSumTurnOvers.push_back(0);
    mSumTurnOvers.push_back(0);
    mBankerTurnOvers.push_back(0);
    mBankerTurnOvers.push_back(0);
    DBWrapper::openDB(mDBName.c_str(), &mOriginDB);
}

bool TurnOverDiscover::isDBFlowInFive(const std::string& aTableName) {
    return true;
}

bool TurnOverDiscover::isDBFlowInTen(const std::string& aTableName) {
    return true;
}

bool TurnOverDiscover::isDBFlowInMon(const std::string& aTableName) {
    return true;
}


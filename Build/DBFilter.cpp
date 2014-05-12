#include "DBFilter.h"

#include "DBOperations.h"
#include "DBWrapper.h"
#include "ErrorDefines.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#define DEFAULT_LASTING_LEN 3

#define LOGTAG   "DBFilter"
#define DEFAULT_VALUE_FOR_INT -1
#define MIN_TURNOVER 100
#define VALUABLE_DAYS_BEFORE 10

#define DATE                " Date "
#define VOLUME              " Volume "
#define TURNOVER            " TurnOver "
#define TURNOVER_SALE       " TurnOverSale "
#define TURNOVER_BUY        " TurnOverBuy "
#define SALE_BUY            " SaleBuy "
#define BEGIN_PRICE         " BeginPrice "
#define END_PRICE           " EndPrice "
#define SUM_FLOWIN_TEN_DAYS " SumFlowInTenDay "
#define FLOWIN_ONE_DAY      " FlowInOneDay "

static char* sqlERR = NULL;
static double sumShots   = 0.0;
static double sumForcasters = 0.0;
static double sumIncome = 0.0;

static std::list<double> sPre15Flowins;

std::list<std::string> DBFilter::mTableNames;
std::list<std::string> DBFilter::mNewAddedTables;
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

static std::string SELECT_COLUMNS_IN_ORDER(const std::string& srcTable,
                                           const std::string& columnNames,
                                           const std::string& key,
                                           const bool positiveOrder) {
    std::string command("");
    command += " SELECT ";
    command += columnNames;
    command += " FROM ";
    command += srcTable;
    command += " ORDER BY ";
    command += key;
    if (positiveOrder) {
       command += "ASC";
    } else {
       command += "DESC";
    }

    return command;
}

static std::string SELECT_COLUMNS_IN_GROUP(const std::string& srcTable,
                                           const std::string& columnNames,
                                           const std::string& key) {
    std::string command("");
    command += " SELECT ";
    command += columnNames;
    command += " FROM ";
    command += srcTable;
    command += " GROUP BY ";
    command += key;

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

DBFilter::DBFilter(const std::string& aDBName)
        : mDBName(aDBName) {
    openOriginDB(aDBName);
}

DBFilter::~DBFilter() {
    mBaseResultDatas.clear();
    mNewAddedTables.clear();
    closeOriginDB(mDBName);
}

bool DBFilter::clearTableFromOriginDB(const std::string& aDBName, const std::string& aTableName) {
    LOGI(LOGTAG, "clear Table: %s from Database: %s", aTableName.c_str(), aDBName.c_str());

    if (!openOriginDB(aDBName)) {
        exit(1);
        LOGI(LOGTAG, "Fail to open db:%s", aDBName.c_str());
        return false;
    }

    if (!clearTable(aTableName)) {
        LOGI(LOGTAG, "Fail to clear table:%s", aTableName.c_str());
        return false;
    }
    closeOriginDB(aDBName);

    return true;
}

bool DBFilter::filterOriginDBByTurnOver(const std::string& aDBName, const int aMinTurnover, const int aMaxTurnOver) {
    LOGI(LOGTAG, "DBName:%s", aDBName.c_str());
    if (!openOriginDB(aDBName)) {
        closeOriginDB(aDBName);
        LOGI(LOGTAG, "Fail to open db:%s", aDBName.c_str());
        return false;
    }

    //Step 1: get all the tables in the OriginDB
    if (!DBWrapper::getAllTablesOfDB(aDBName, mTableNames)) {
        closeOriginDB(aDBName);
        LOGI(LOGTAG, "Failt to get tables from :%s", aDBName.c_str());
        return false;
    }

    //Step 2: Filter all the tables of the OriginDB and save them in a 'tmp table'
    if (!filterTablesByTurnOver(aDBName, aMinTurnover, mTableNames)) {
        closeOriginDB(aDBName);
        LOGI(LOGTAG, "Fail to filter tables of :%s", aDBName.c_str());
        return false;
    }

    closeOriginDB(aDBName);
    return true;
}

bool DBFilter::updateFilterResultByTurnOver(const std::string& aDBName, const int aMinTurnover, const int aMaxTurnOver) {
    LOGI(LOGTAG, "DBName:%s", aDBName.c_str());
    if (!openOriginDB(aDBName)) {
        closeOriginDB(aDBName);
        LOGI(LOGTAG, "Fail to open db:%s", aDBName.c_str());
        return false;
    }

    //Step 1: get all the tables in the OriginDB
    errno = 0;
    if (!DBWrapper::getAllTablesOfDB(aDBName, mTableNames)) {
        closeOriginDB(aDBName);
        LOGI(LOGTAG, "updateFilterResultByTurnOver Fail to get tables from :%s, errno:%d", aDBName.c_str(), errno);
        exit(1);
        return false;
    }

    //Step 2.1: get the filtered origin tables
    std::list<std::string> filteredOriginTables;
    std::string sql, targetColumns;
    sqlite3_stmt* stmt = NULL;
    bool boolRet = false;
    int intRet = -1;

    boolRet = openTable(DBWrapper::FILTER_RESULT_TABLE, aDBName, mResultTableName);
    if (!boolRet) {
        LOGI(LOGTAG, "Fail to open table:%s in DB:%s", mResultTableName.c_str(), aDBName.c_str());
        closeOriginDB(aDBName);
        exit(1);
        return false;
    }

    targetColumns += DATE;
    sql = SELECT_COLUMNS(mResultTableName, targetColumns);

    LOGI(LOGTAG, "sql:%s", sql.c_str());
    intRet = sqlite3_prepare_v2(mOriginDB,
                          sql.c_str(),
                          -1,
                          &stmt,
                          NULL);
    LOGI(LOGTAG, "prepare stmt to Table:%s, intRet:%d, mOriginDB:%p", mResultTableName.c_str(), intRet, mOriginDB);

    if (intRet != SQLITE_OK) {
        LOGI(LOGTAG, "Fail to prepare stmt to Table:%s, intRet:%d, errorMessage:%s", mResultTableName.c_str(), intRet, sqlite3_errmsg(mOriginDB));
        closeOriginDB(aDBName);
        exit(1);
        return false;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string dateOfOriginTable = (char*)sqlite3_column_text(stmt, 0);
        filteredOriginTables.push_back(dateOfOriginTable);
    }

    if (SQLITE_OK != sqlite3_finalize(stmt)) {
        LOGI(LOGTAG, "Fail to finalize stmt in updateFilterResultByTurnOver aDBName:%s", aDBName.c_str());
        closeOriginDB(aDBName);
        exit(1);
        return false;
    }

    //Step 2.2: get the new added origin tables
    std::list<std::string>::iterator itrAll = mTableNames.begin();
    std::list<std::string>::iterator itrFiltered = filteredOriginTables.begin();
    while (itrFiltered != filteredOriginTables.end()
           && itrAll != mTableNames.end()) {
           if ((*itrAll) == "FilterResult"
               || (*itrAll) == "MiddleWareTable") {
               itrAll++;
               continue;
           }

           if ((*itrAll) == (*itrFiltered)) {
               itrAll++;
               itrFiltered++;
               LOGI(LOGTAG, "Existing Table:%s, DB:%s", (*itrAll).c_str(), aDBName.c_str());
               continue;
           } else if ((*itrAll) > (*itrFiltered)) {
               mNewAddedTables.push_back(*itrAll);
               LOGI(LOGTAG, "Find a new added Table:%s, DB:%s", (*itrAll).c_str(), aDBName.c_str());
               itrAll++;
               continue;
           } else {
               // we should not get here
               itrFiltered++;
               continue;
           }
    }

    while (itrAll != mTableNames.end()) {
        mNewAddedTables.push_back(*itrAll);
        LOGD(LOGTAG, "New added Table:%s, DB:%s", (*itrAll).c_str(), aDBName.c_str());
        itrAll++;
    }

    if (mNewAddedTables.size() <= 1) {
        LOGI(LOGTAG, "No any new added OriginTables, dbName:%s", aDBName.c_str());
        return true;
    }

    //Step 3: get the new added origin tables
    if (!filterTablesByTurnOver(aDBName, aMinTurnover, mNewAddedTables)) {
        closeOriginDB(aDBName);
        LOGI(LOGTAG, "Fail to filter tables of :%s", aDBName.c_str());
        return false;
    }
    mNewAddedTables.clear();

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
        closeOriginDB(aDBName);
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
        closeOriginDB(aDBName);
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

    closeOriginDB(aDBName);
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
        closeOriginDB(aDBName);
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
        closeOriginDB(aDBName);
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

    closeOriginDB(aDBName);
    return true;
}

static double sumFlowinSinceBeginningDay;
static double sumFlowoutSinceBeginningDay;
static std::string lastCountedDate;
static int sumAvailableCount;

static void countInTurnOver(const std::string& date, bool reset, double flowinOneDay = 0, double flowoutOneDay = 0) {
    LOGD(LOGTAG, "BEFORE  ##################sumFlowinSinceBeginningDay:%f, sumFlowoutSinceBeginningDay:%f", sumFlowinSinceBeginningDay, sumFlowoutSinceBeginningDay);
    if (lastCountedDate == date) {
        return;
    }
    if (!reset) {
        sumFlowinSinceBeginningDay  += flowinOneDay;
        sumFlowoutSinceBeginningDay += flowoutOneDay;
        sumAvailableCount++;
    } else {
        sumFlowinSinceBeginningDay  = 0;
        sumFlowoutSinceBeginningDay = 0;
        sumAvailableCount = 0;
    }
    lastCountedDate = date;
    LOGD(LOGTAG, "AFTER  ##################sumFlowinSinceBeginningDay:%f, sumFlowoutSinceBeginningDay:%f", sumFlowinSinceBeginningDay, sumFlowoutSinceBeginningDay);
}

static bool shouldSaleOut(double flowinOneDay, double flowoutOneDay) {
    sumAvailableCount++;
    sumFlowinSinceBeginningDay  += flowinOneDay;
    sumFlowoutSinceBeginningDay += flowoutOneDay;

    LOGD(LOGTAG, "=====================================================sumFlowinSinceBeginningDay:%f, sumFlowoutSinceBeginningDay:%f", sumFlowinSinceBeginningDay, sumFlowoutSinceBeginningDay);
    if (sumFlowoutSinceBeginningDay > sumFlowinSinceBeginningDay * 0.7) {
        LOGD(LOGTAG, "##################sumFlowinSinceBeginningDay:%f, sumFlowoutSinceBeginningDay:%f", sumFlowinSinceBeginningDay, sumFlowoutSinceBeginningDay);
        sumFlowoutSinceBeginningDay = sumFlowinSinceBeginningDay = 0;
        sumAvailableCount = 0;
        return true;
    }
    return false;
}

static bool shouldBuyInNow() {
    if ((sumFlowinSinceBeginningDay > MIN_TURNOVER) &&
        (sumFlowoutSinceBeginningDay > MIN_TURNOVER)) {
        return sumFlowinSinceBeginningDay > (1.5 * sumFlowoutSinceBeginningDay);
    }
    return false;
}

bool DBFilter::getBuyDateRegionsContinueFlowinFFP(const std::string& aDBName, std::list<DBFilter::DateRegion>& recommandBuyDateRegions) {
    std::string sql, targetColumns;
    sqlite3_stmt* stmt = NULL;
    bool boolRet = false;
    int intRet = -1;

    boolRet = openTable(DBWrapper::FILTER_TRUNOVER_TABLE, aDBName, mResultTableName);
    if (!boolRet) {
        LOGI(LOGTAG, "Fail to open table:%s in DB:%s", mResultTableName.c_str(), aDBName.c_str());
        closeOriginDB(aDBName);
        return false;
    }

    targetColumns += DATE;
    targetColumns += ",";
    targetColumns += TURNOVER_SALE;
    targetColumns += ",";
    targetColumns += TURNOVER_BUY;
    targetColumns += ",";
    targetColumns += FLOWIN_ONE_DAY;
    targetColumns += ",";
    targetColumns += SUM_FLOWIN_TEN_DAYS;
    targetColumns += ",";
    targetColumns += BEGIN_PRICE;
    targetColumns += ",";
    targetColumns += END_PRICE;
    sql = SELECT_COLUMNS(mResultTableName, targetColumns);

    LOGI(LOGTAG, "sql:%s", sql.c_str());
    intRet = sqlite3_prepare(mOriginDB,
                          sql.c_str(),
                          -1,
                          &stmt,
                          NULL);

    if (intRet != SQLITE_OK) {
        LOGI(LOGTAG, "Fail to prepare stmt to Table:%s, errno:%d", mResultTableName.c_str(), errno);
        closeOriginDB(aDBName);
        exit(1);
        return false;
    }

    DBFilter::DateRegion tmpDateRegion;
    int count = 0;
    int lastingDays = 0;
    //block of continious positive flowinoneday
    bool newContiniousBlockStarted = false;
    //block of buy/sale
    bool newBuySaleBlockStarted = false;
    double startPrice = 0.0;
    std::string startDate, endDate;
            tmpDateRegion.mStartDate = startDate;
            tmpDateRegion.mEndDate   = endDate;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        
        std::string date    = (char*)sqlite3_column_text(stmt, 0);
        double turnoverSale = sqlite3_column_double(stmt, 1);
        double turnoverBuy  = sqlite3_column_double(stmt, 2);
        double flowinOneDay = sqlite3_column_double(stmt, 3);
        double flowinTenDay = sqlite3_column_double(stmt, 4);
        double beginPrice   = sqlite3_column_double(stmt, 5);
        double endPrice     = sqlite3_column_double(stmt, 6);

        LOGD(LOGTAG, "aDBName:%s, Date:%s, TurnoverSale:%f, TurnoverBuy:%f, flowinOneDay:%f, beginPrice:%f, endPrice:%f",
                      aDBName.c_str(), date.c_str(), turnoverSale, turnoverBuy, flowinOneDay, beginPrice, endPrice);
        //Step 1: find the starting day: there has been DEFAULT_LASTING_LEN days whose flowinOneDay is positive
        if (flowinOneDay > MIN_TURNOVER && !newBuySaleBlockStarted) {
            if (!newContiniousBlockStarted) {
                newContiniousBlockStarted= true;
                startPrice = beginPrice;
            }
            ++lastingDays;
            // If the length of lastingDays is less than DEFAULT_LASTING_LEN,
            // which means we still want lastingDays to graw up
            // Else it is time to check other contidions
            if (lastingDays < DEFAULT_LASTING_LEN) {
                countInTurnOver(date, false, turnoverBuy, turnoverSale);
                //compute the days whose flowinOneDay/turnoverSale > 30%
                if ((flowinOneDay/turnoverSale) > 0.5) {
                    LOGI(LOGTAG, "Meet the contition to start counting:flowinOneDay:%f, turnoverSale:%f, DBName:%s, date:%s", flowinOneDay, turnoverSale, aDBName.c_str(), date.c_str());
                    count++;
                }
                //If we decide to buy it now through SumFlowin/out, then do it.
                if (!shouldBuyInNow()) {
                    continue;
                }
            }
        } else if ((flowinOneDay < -MIN_TURNOVER) && !newBuySaleBlockStarted) {
            // reset the count & lastingDays & beginPrice
            countInTurnOver(date, true);
            count = lastingDays = 0;
            newContiniousBlockStarted = false;
            tmpDateRegion.mStartDate = tmpDateRegion.mEndDate = "";
            continue;
        }

        //Step 2: check whether the Turnover & Price meets our need
        if (newContiniousBlockStarted
            && ((shouldBuyInNow() && lastingDays >= (DEFAULT_LASTING_LEN/2))
            || flowinTenDay > 1.3 * flowinOneDay 
            || lastingDays >= DEFAULT_LASTING_LEN)) {
            //|| (shouldBuyInNow() && lastingDays >= (DEFAULT_LASTING_LEN/2)))) {
            countInTurnOver(date, false, turnoverBuy, turnoverSale);
            if ( count >= 1 || lastingDays >= DEFAULT_LASTING_LEN) {
                LOGI(LOGTAG, "Meet the contition to start the newBuySaleBlockStarted, DBName:%s, date:%s", aDBName.c_str(), date.c_str());
                newBuySaleBlockStarted = true;
                tmpDateRegion.mStartDate = date;
                count = lastingDays = 0;
            }
            continue;
        }

        //Step 3: get the dateNO on which we should sale it.
        if (newBuySaleBlockStarted && newBuySaleBlockStarted
            && ((flowinOneDay > MIN_TURNOVER) || (flowinOneDay < -MIN_TURNOVER))) {
            //FIXME:compute the sale-date, 
            //TODO: Needs more complicated Algorithm to determine when to sale it
            //if (flowinOneDay / turnoverSale < 0.1) {
            if (shouldSaleOut(turnoverBuy, turnoverSale)) {
                tmpDateRegion.mEndDate = date;
                LOGI(LOGTAG, "Meet the contition to end the newBuySaleBlockStarted, DBName:%s, date:%s", aDBName.c_str(), date.c_str());
                recommandBuyDateRegions.push_back(tmpDateRegion);
                //reset everything
                tmpDateRegion.mStartDate = tmpDateRegion.mEndDate = "";
                newContiniousBlockStarted = false;
                newBuySaleBlockStarted = false;
            }
        }
    }
    if (SQLITE_OK != sqlite3_finalize(stmt)) {
        LOGI(LOGTAG, "Fail to finalize stmt in getBuyDateRegionsContinueFlowinFFP aDBName:%s", aDBName.c_str());
        closeOriginDB(aDBName);
        return false;
    }

    closeOriginDB(aDBName);
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
      case DBFilter::CONTINUE_FLOWIN_FROM_FIRST_POSITIVE:
          getBuyDateRegionsContinueFlowinFFP(aDBName, recommandBuyDateRegions);
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

bool DBFilter::getExistingFilterResults(const std::string& aDBName, std::list<BaseResultData>& outFilterResults) {
    if (!openTable(DBWrapper::FILTER_TRUNOVER_TABLE, aDBName, mResultTableName)) {
        closeOriginDB(aDBName);
        return false;
    }

    int ret = 0;
    sqlite3_stmt* stmt = NULL;
    std::string sql;
    std::string columns("");

    columns += DATE;
    columns += ",";
    columns += TURNOVER_SALE;
    columns += ",";
    columns += TURNOVER_BUY;
    columns += ",";
    columns += FLOWIN_ONE_DAY;

    sql = SELECT_COLUMNS_IN_ORDER(mResultTableName, columns, DATE, false);
    ret = sqlite3_prepare(mOriginDB,
                          sql.c_str(),
                          -1,
                          &stmt,
                          NULL);
    
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "Fail to prepare stmt for getExistingFilterResults");
        return false;
    }

    int i = 0;
    BaseResultData tempBaseResultData;
     
    double buyTurnOver = 0;
    double saleTurnOver = 0;
    while(sqlite3_step(stmt) == SQLITE_ROW && i < VALUABLE_DAYS_BEFORE) {
              std::string date = (char*)sqlite3_column_text(stmt, 0);
              if (date < "O20140512") {
                 saleTurnOver = sqlite3_column_double(stmt, 1);
                 buyTurnOver  = sqlite3_column_double(stmt, 2);
                  if (buyTurnOver > MIN_TURNOVER
                      && saleTurnOver > MIN_TURNOVER) {
                      tempBaseResultData.mDate = date;
                      tempBaseResultData.mSaleTurnOver = saleTurnOver;
                      tempBaseResultData.mBuyTurnOver = buyTurnOver;
                      tempBaseResultData.mPureFlowInOneDay = sqlite3_column_double(stmt, 3);
                      outFilterResults.push_back(tempBaseResultData);
                      i++;
                  }
          }
    }

    ret = sqlite3_finalize(stmt);
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "Fail to finalize the stmt to getExistingFilterRsults from Table:%s",  mResultTableName.c_str());
        return false;
    }
    closeOriginDB(aDBName);

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

    sql = SELECT_COLUMNS_IN_GROUP(tmpTableName, columns, keyColumn);

    ret = sqlite3_prepare(mOriginDB,
                          sql.c_str(),
                          -1,
                          &stmt,
                          NULL);
    if (ret != SQLITE_OK) {
        LOGI(LOGTAG, "Fail to getExistingBaseResults, err:%s", sqlite3_errmsg(mOriginDB));
        return false;
    }

    std::list<BaseResultData> existingBaseResults;
    if (!getExistingFilterResults(aDBName, existingBaseResults)) {
        LOGI(LOGTAG, "Fail to getExistingBaseResults, err:%s", sqlite3_errmsg(mOriginDB));
        return false;
    }

    {
        //FIXME: The first raw is Buy, second raw is Sale
        BaseResultData tempBaseResultData;
        while(sqlite3_step(stmt) == SQLITE_ROW) {
            std::string isBuy("");
            isBuy= (char*)sqlite3_column_text(stmt, 2);
            LOGI(LOGTAG, "isBuy:%s, db:%s", isBuy.c_str(), aDBName.c_str());

            if (isBuy == std::string("true")) {
                tempBaseResultData.mBuyVolume   = sqlite3_column_int(stmt, 0);
                tempBaseResultData.mBuyTurnOver = sqlite3_column_double(stmt, 1);
                tempBaseResultData.mBuyPrice    = (tempBaseResultData.mBuyTurnOver/(100 * tempBaseResultData.mBuyVolume));
                LOGI(LOGTAG, "sale, volume:%d, turnover:%f, avg:%f", tempBaseResultData.mBuyVolume, tempBaseResultData.mBuyTurnOver, tempBaseResultData.mBuyPrice);
            } else if (isBuy == std::string("false")) {
                tempBaseResultData.mSaleVolume   = sqlite3_column_int(stmt, 0);
                tempBaseResultData.mSaleTurnOver = sqlite3_column_double(stmt, 1);
                tempBaseResultData.mSalePrice    = (tempBaseResultData.mSaleTurnOver/(100 * tempBaseResultData.mSaleVolume));
                LOGI(LOGTAG, "buy, volume:%d, turnover:%f, avg:%f", tempBaseResultData.mSaleVolume, tempBaseResultData.mSaleTurnOver, tempBaseResultData.mSalePrice);
            } else {
                // Not buy, not sale, just a normal.
                // The sale_buy should be empty
                if (isBuy.empty()) {
                    LOGD(LOGTAG, "NON-BUY-SALE");
                    continue;
                }
                LOGI(LOGTAG, "isBuy:%s", isBuy.c_str());
                return false;
            }
        }
        tempBaseResultData.mDate = originTableName;
        tempBaseResultData.mPureFlowInOneDay = tempBaseResultData.mBuyTurnOver - tempBaseResultData.mSaleTurnOver;
        LOGI(LOGTAG, "turnover: sale:%f, buy:%f, diff:%f", tempBaseResultData.mSaleTurnOver, tempBaseResultData.mBuyTurnOver, tempBaseResultData.mBuyTurnOver - tempBaseResultData.mSaleTurnOver);

        tempBaseResultData.mBeginPrice = beginningPrice;
        tempBaseResultData.mEndPrice   = endingPrice;
        mBaseResultDatas.push_back(tempBaseResultData);

        int i = 0;
        std::list<DBFilter::BaseResultData>::iterator itr;

        // Figureout flowin in 10 days
        // Step 1: Count in the new added FilterResults 
        LOGD(LOGTAG, " mBaseResultDatas size:%d, i:%d", mBaseResultDatas.size(), i);

        //TODO: Find a better way to work around, for the sake of the std::list<T>::end() includes nothing.
        itr = --(mBaseResultDatas.end());
        for (i = 0; i < VALUABLE_DAYS_BEFORE; itr--) {
            //Only valueable days are counted on
            if ((*itr).mBuyTurnOver > MIN_TURNOVER && (*itr).mSaleTurnOver > MIN_TURNOVER) {
                LOGD(LOGTAG, "Current TurnOver sale:%f, buy:%f, diff:%f", (*itr).mSaleTurnOver, (*itr).mBuyTurnOver, (*itr).mPureFlowInOneDay)
                LOGD(LOGTAG, "Current tempBaseResultData.mSumFlowInTenDays:%f, mPureFlowInOneDay:%f", tempBaseResultData.mSumFlowInTenDays,  (*itr).mPureFlowInOneDay);
                tempBaseResultData.mSumFlowInTenDays += (*itr).mPureFlowInOneDay;
                i++;
            }


            if (i >= VALUABLE_DAYS_BEFORE
                || itr == mBaseResultDatas.begin()) {
                break;
            }
        }

        // Step 2: Count in the exsiting FilterResults 
        itr = existingBaseResults.end();
        LOGD(LOGTAG, "existingBaseResults size:%d, i:%d", existingBaseResults.size(), i);
        while (i < VALUABLE_DAYS_BEFORE && itr != existingBaseResults.begin()) {
            if ((*itr).mBuyTurnOver > MIN_TURNOVER && (*itr).mSaleTurnOver > MIN_TURNOVER) {
                tempBaseResultData.mSumFlowInTenDays += (*itr).mPureFlowInOneDay;
                LOGD(LOGTAG, "Existing TurnOver sale:%f, buy:%f, diff:%f, date:%s", (*itr).mSaleTurnOver, (*itr).mBuyTurnOver, (*itr).mPureFlowInOneDay, (*itr).mDate.c_str())
                i++;
            }
            itr--;
        }
        mBaseResultDatas.pop_back();
        mBaseResultDatas.push_back(tempBaseResultData);

        //TODO: Optimization of Figurout flowin in 10 days
        //if (mBaseResultDatas.size() > VALUABLE_DAYS_BEFORE) {
        //    for (i = 0, itr = mBaseResultDatas.end(); i < VALUABLE_DAYS_BEFORE && itr != mBaseResultDatas.begin(); itr--) {
        //        if ((*itr).mBuyTurnOver > MIN_TURNOVER && (*itr).mSaleTurnOver > MIN_TURNOVER) {
        //            i++;
        //        }
        //    }
        //        LOGD(LOGTAG, "end mSumFlowInTenDays:%f, VALUABLE_DAYS_BEFORE pre mSumFlowInTenDays:%f", (*mBaseResultDatas.end()).mSumFlowInTenDays,  (*itr).mSumFlowInTenDays);
        //    tempBaseResultData.mSumFlowInTenDays = (*mBaseResultDatas.end()).mSumFlowInTenDays + ((*mBaseResultDatas.end()).mSumFlowInTenDays - (*itr).mSumFlowInTenDays);
        //} else {
        //    for (i = 0, itr = mBaseResultDatas.end(); i < VALUABLE_DAYS_BEFORE && itr != mBaseResultDatas.begin(); itr--) {
        //        if ((*itr).mBuyTurnOver > MIN_TURNOVER && (*itr).mSaleTurnOver > MIN_TURNOVER) {
        //            tempBaseResultData.mSumFlowInTenDays += (*itr).mPureFlowInOneDay;
        //            i++;
        //        }
        //    }
        //}
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
        closeOriginDB(aDBName);
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
    mBaseResultDatas.clear();

    closeOriginDB(aDBName);
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

bool DBFilter::filterTablesByTurnOver(const std::string& aDBName, const int aMinTurnover, std::list<std::string>& tableNames) {
    //FIXME: Assuming that process all the tables in one DB and then the tables of the other DB.
    if (tableNames.size() < 1) {
        return true;
    }
    if (!openTable(DBWrapper::FILTER_TRUNOVER_TABLE, aDBName, mTmpResultTableName)) {
        LOGI(LOGTAG, "Fail to open table:%s in DB:%s", mTmpResultTableName.c_str(), aDBName.c_str());
        closeOriginDB(aDBName);
        exit(1);
        return false;
    }
    std::list<std::string>::iterator iterOfTableName;
    double beginningPrice, endingPrice;
    for (iterOfTableName = tableNames.begin(); iterOfTableName != tableNames.end(); iterOfTableName++) {
         if ((*iterOfTableName) != (mResultTableName) &&
             (*iterOfTableName) != (mTmpResultTableName)) {
             //Step 1: get Beginning & ending Price
             beginningPrice = endingPrice = 0.0;
             if (!getBeginAndEndPrice(*iterOfTableName, beginningPrice, endingPrice)) {
                 LOGI(LOGTAG, "Fail to getBeginAndEndPrice table:%s in DB:%s", (*iterOfTableName).c_str(), aDBName.c_str());
                 closeOriginDB(aDBName);
                 return false;
             }

             //Step 2: filter
             if (!filterTableByTurnOver(*iterOfTableName, aMinTurnover)) {
                 LOGI(LOGTAG, "Fail to filterTableByTurnOver table:%s in DB:%s", (*iterOfTableName).c_str(), aDBName.c_str());
                 closeOriginDB(aDBName);
                 exit(1);
                 return false;
             }

             //Step 3: compute && save result 
             if (!computeResultFromTable(aDBName, mTmpResultTableName, *iterOfTableName, beginningPrice, endingPrice)) {
                 LOGI(LOGTAG, "Fail to computeResultFromTable table:%s in DB:%s", (*iterOfTableName).c_str(), aDBName.c_str());
                 closeOriginDB(aDBName);
                 exit(1);
                 return false;
             }

             if (!clearTable(mTmpResultTableName)) {
                 closeOriginDB(aDBName);
                 return false;
             }
         }
    }

    ////Step 4: save to final result 
    if (!saveBaseResultInBatch(aDBName, mResultTableName)) {
        LOGI(LOGTAG, "Fail to saveBaseResultInBatch table:%s in DB:%s", (*iterOfTableName).c_str(), aDBName.c_str());
        closeOriginDB(aDBName);
        exit(1);
        return false;
    }

    closeOriginDB(aDBName);
    return true;
}

//FIXME: the recommandBuyRegions should be of const
bool DBFilter::getHitRateOfBuying(const std::string& aDBName, std::list<DBFilter::DateRegion>& recommandBuyRegions) {
    LOGI(LOGTAG, "Enter getHitRateOfBuying, aDBName:%s", aDBName.c_str());
    if (recommandBuyRegions.size() < 1) {
        return true;
    }
    std::string sql, targetColumns;
    sqlite3_stmt* stmt = NULL;
    bool boolRet = false;
    int intRet = -1;

    boolRet = openTable(DBWrapper::FILTER_TRUNOVER_TABLE, aDBName, mResultTableName);
    if (!boolRet) {
        LOGI(LOGTAG, "Fail to open table:%s in DB:%s", mResultTableName.c_str(), aDBName.c_str());
        closeOriginDB(aDBName);
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
        closeOriginDB(aDBName);
        return false;
    }

    int index = 0;
    std::list<DBFilter::DateRegion>::iterator itr = recommandBuyRegions.begin();
    std::list<DBFilter::PriceRegion> realPriceRegions;
    DBFilter::PriceRegion tmpPriceRegion;

    std::list<DBFilter::DateRegion>::iterator debugDateRegionItr = recommandBuyRegions.begin();
    //while (debugDateRegionItr != recommandBuyRegions.end()) {
    //    LOGI(LOGTAG, "mStartDate in recommandRegions:%s, mEndDate:%s", (*debugDateRegionItr).mStartDate.c_str(), (*debugDateRegionItr).mEndDate.c_str());
    //    debugDateRegionItr++;
    //}

    //Step 1: compare the Date
    while (sqlite3_step(stmt) == SQLITE_ROW && itr != recommandBuyRegions.end()) {
        //NOTE: Assume that the recommandBuyRegions are ordered by date.
        std::string date = (char*)sqlite3_column_text(stmt, 0);
        if (date < (*itr).mStartDate) {
            // have not reach the edge, need not to compare
            continue;
        } else if (date == (*itr).mStartDate) {
            // record the beginningPrice
            // once beggingPrice is set, a new block of region start.
            // so clear the endPrice.
            LOGD(LOGTAG, "Find the the startDate in recommandRegions:%s", date.c_str());
            tmpPriceRegion.mBeginPrice = sqlite3_column_double(stmt, 2);
            continue;
        } else if (date == (*itr).mEndDate) {
            // record the endPrice
            LOGD(LOGTAG, "Find the the EndDate in recommandRegions:%s", date.c_str());
            tmpPriceRegion.mEndPrice = sqlite3_column_double(stmt, 2);
            LOGD(LOGTAG, "the EndPrice:%f , the BeginPrice:%f in tmpPriceRegion:%f", tmpPriceRegion.mEndPrice, tmpPriceRegion.mBeginPrice);
            realPriceRegions.push_back(tmpPriceRegion);
            itr++;
        } else {
            LOGD(LOGTAG, "Normal Date %s", date.c_str());
        }
    }

    intRet = sqlite3_finalize(stmt);
    if (intRet != SQLITE_OK) {
        closeOriginDB(aDBName);
        LOGI(LOGTAG, "Fail to finalize the stmt to finalize table:%s", mResultTableName.c_str());
        return false;
    }


    //Step 2: ensure UP DOWN
    if (realPriceRegions.size() != recommandBuyRegions.size()) {
        LOGI(LOGTAG, "The size of PriceRegions does not equal to that of recommandBuyRegions"); LOGI(LOGTAG, "realPriceRegions length:%d, recommandBuyRegions:%d", realPriceRegions.size(), recommandBuyRegions.size());
        closeOriginDB(aDBName);
        return false;
    }

    std::list<DBFilter::PriceRegion>::iterator debugPriceRegionItr = realPriceRegions.begin();
    debugDateRegionItr = recommandBuyRegions.begin();
    while (debugPriceRegionItr!= realPriceRegions.end()) {
        LOGD(LOGTAG, "mStartDate in PriceRegions mBeginPrice:%f, mEndPrice:%f", (*debugPriceRegionItr).mBeginPrice, (*debugPriceRegionItr).mEndPrice);
        debugPriceRegionItr++;
    }

    double count = 0.0;
    std::list<DBFilter::PriceRegion>::iterator itrOfPriceRegion = realPriceRegions.begin();
    debugDateRegionItr = recommandBuyRegions.begin();
    for (int i = 0; i < realPriceRegions.size(); i++, debugDateRegionItr++) {
        double diff = (*itrOfPriceRegion).mEndPrice - (*itrOfPriceRegion).mBeginPrice;
        if (diff >= 0.01) {
            count = count + 1.0;
        }
        sumIncome = sumIncome + (diff / (*itrOfPriceRegion).mBeginPrice);
        LOGI(LOGTAG, "dbname:%s, begindate:%s, enddate:%s", aDBName.c_str(), (*debugDateRegionItr).mStartDate.c_str(), (*debugDateRegionItr).mEndDate.c_str());
        LOGI(LOGTAG, "beginPrice:%f, endPrice:%f, diff:%f, sumIncome:%f ", (*itrOfPriceRegion).mBeginPrice, (*itrOfPriceRegion).mEndPrice, diff, sumIncome);
        itrOfPriceRegion++;
    }

    sumShots += count;
    sumForcasters += realPriceRegions.size();

    //Step 3: compute HintRate
    double hitRate = count/realPriceRegions.size();
    LOGI(LOGTAG, "HitRate for Suggesting Buy:%f, shot count:%f, sum count:%d", hitRate, count, realPriceRegions.size());
    LOGI(LOGTAG, "\n\n======sum of reallly HitShots:%f, sum of Forcasters:%f, hitRate:%f, sumIncome:%f\n\n", sumShots, sumForcasters, hitRate, sumIncome);
    closeOriginDB(aDBName);

    return true;
}

bool DBFilter::getGlobalHitRate(double& hitRate) {
    hitRate = sumShots/sumForcasters;
    double globaleIncome = sumIncome / sumForcasters;
    LOGI(LOGTAG, "\n\n======sum of reallly HitShots:%f, sum of Forcasters:%f, hitRate:%f, globaleIncome:%f\n\n", sumShots, sumForcasters, hitRate, globaleIncome);
    return true;
}


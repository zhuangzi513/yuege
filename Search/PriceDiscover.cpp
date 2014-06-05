#include "PriceDiscover.h"

#include <math.h>
#include "ErrorDefines.h"
#include "DBWrapper.h"

#define LOGTAG  "PriceDiscover"

#define MIN_PRICE 1.0
#define PRECISION 0.001
#define MAX_REAL_FLOAT_SIDE_WAY 0.08
#define MAX_AVG_ABS_FLOAT_SIDE_WAY 0.008

static std::string SELECT_COLUMNS(const std::string& tableName, const std::string& targetColumns) {
    std::string command("");
    command += " SELECT ";
    command += targetColumns;
    command += " FROM ";
    command += tableName;
    return command;
}


PriceDiscover::PriceDiscover(const std::string& aDBName)
        : mDBName(aDBName) {
    DBWrapper::openDB(aDBName, &mOriginDB);
}

PriceDiscover::~PriceDiscover() {
    DBWrapper::closeDB(mDBName);
}

bool PriceDiscover::isSteadySideWays(const std::string& aTableName) {
    if (!getPriceDatasFromResultTable(aTableName)) {
        return false;
    }

    std::list<PriceData>::iterator iterPriceData = mPriceDatas.begin();
    int vailuableDays = 0;
    double absFloat = 0.0;
    double realFloat = 0.0;
    double startPrice = iterPriceData->mBeginPrice;
    double endPrice = 0.0;

    for (iterPriceData = mPriceDatas.begin(); iterPriceData != mPriceDatas.end(); iterPriceData++) {
         if ((iterPriceData->mEndPrice > MIN_PRICE)
             && (iterPriceData->mBeginPrice > MIN_PRICE)) {
             absFloat += fabs((iterPriceData->mEndPrice - iterPriceData->mBeginPrice)/iterPriceData->mBeginPrice);
             vailuableDays++;
         }
         
    }

    endPrice = (--iterPriceData)->mEndPrice;
    realFloat = (endPrice - startPrice)/startPrice;

    if (endPrice < MIN_PRICE
        || startPrice < MIN_PRICE) {
        return false;
    }

    return isPriceSideWays(realFloat, absFloat, vailuableDays);
}

bool PriceDiscover::isInPhaseOne(const std::string& aTableName) {
    return true;
}

bool PriceDiscover::isInPhaseTwo(const std::string& aTableName) {
    return true;
}

bool PriceDiscover::isInPhaseThree(const std::string& aTableName) {
    return true;
}



bool PriceDiscover::isPriceSideWays(double aRealFloat, double aABSFloat, int aDaysCount) {
    if ((fabs(aRealFloat) - MAX_REAL_FLOAT_SIDE_WAY) > PRECISION)  {
        return false;
    }

    if (((aABSFloat/aDaysCount)-MAX_AVG_ABS_FLOAT_SIDE_WAY) > PRECISION) {
        return false;
    }

    return true;
}

bool PriceDiscover::getPriceDatasFromResultTable(const std::string& aTableName) {

    std::string sql;
    sqlite3_stmt* stmt = NULL;
    std::string columns;

    columns += " Date, ";
    columns += " BeginPrice, ";
    columns += " EndPrice ";

    sql = SELECT_COLUMNS(aTableName, columns);

    int ret = sqlite3_prepare(mOriginDB, sql.c_str(), -1, &stmt, NULL);
    if (ret != SQLITE_OK) {
         LOGE(LOGTAG, "Fail to prepare stmt to Table:%s, intRet:%d, errorMessage:%s", aTableName.c_str(), ret, sqlite3_errmsg(mOriginDB));
        return false;
    }


    while (sqlite3_step(stmt) == SQLITE_ROW) {
        PriceData tmpPriceData;

        tmpPriceData.mDate = (char*)sqlite3_column_text(stmt, 0);
        tmpPriceData.mBeginPrice = sqlite3_column_double(stmt, 1);
        tmpPriceData.mEndPrice = sqlite3_column_double(stmt, 2);

        mPriceDatas.push_back(tmpPriceData);
    }

    if (SQLITE_OK != sqlite3_finalize(stmt)) {
        LOGE(LOGTAG, "Fail to finalize stmt in PriceDiscover::isDBSteadySideWays aDBName:%s", mDBName.c_str());
        return false;
    }

    return true;
}

#include "PriceDiscover.h"

#include <math.h>
#include "ErrorDefines.h"
#include "DBWrapper.h"

#define LOGTAG  "PriceDiscover"

#define  DATE                    " Date "
#define  BEGIN_PRICE             " BeginPrice "
#define  END_PRICE               " EndPrice "

#define  VOLUME_SALE             " VolumeSale "
#define  VOLUME_BUY              " VolumeBuy "
#define  VOLUME_FLOWIN_ONE_DAY   " VolumeFlowInOneDay "

#define  TURNOVER_SALE           " TurnOverSale "
#define  TURNOVER_BUY            " TurnOverBuy "
#define  TURNOVER_FLOWIN_ONE_DAY " TurnOverFlowInOneDay "

#define LEVEL3  20
#define LEVEL2  10
#define LEVEL1  5

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



PriceDiscover::PriceDiscover(const std::string& aDBName, const std::string& aResultTableName)
        : mDBName(aDBName)
        , mTargetResultTableName(aResultTableName) {
    DBWrapper::openDB(aDBName, &mOriginDB);
    //FIXME: we should know the result of mTargetResultTableName and decide exit or not.
    getNeededInfoFromTable(mTargetResultTableName);
}

PriceDiscover::~PriceDiscover() {
    mValuableInfos.clear();
    mPriceDatas.clear();
    DBWrapper::closeDB(mDBName);
}

int PriceDiscover::howLongSteadySideWays() {
   if (isSteadySideWays(LEVEL3)) {
       return LEVEL3;
   } else if (isSteadySideWays(LEVEL2)) {
       return LEVEL2;
   } else if (isSteadySideWays(LEVEL1)) {
       return LEVEL1;
   }

   return 0;
}

/*
 * Phase One: eating in as many shares as he can, and make sure that the
 *            price doesn't goes up.
 *
 * Characteristics : 
 *            1) Volume of Big B is much larger than Big S;
 *            2) Price of Big B is a little higher than Big S;
 *            3) Price of Big S is always lower than avg price;
 *            4) Price of Big B should be higher than avg price;
 *            5) And the most important, Price keeps floating at a small region.
*/
bool PriceDiscover::isInPhaseOne(const std::string& aTableName) {
    // Step 1: Make sure it is side ways now;
    int lengthOfItSideWays = howLongSteadySideWays();
    //FIXME: For now, we think it as SideWays when it keeps longer than LEVEL1
    if (lengthOfItSideWays < LEVEL2) {
        return false;
    }

    int sumBuyVolume = 0;
    int sumSaleVolume = 0;
    double sumTurnOverSale = 0.0;
    double sumTurnOverBuy = 0.0;
    double buyPrice = 0.0;
    double salePrice = 0.0;
    int i = 0;
    std::list<ValueAbleInfo>::iterator itrOfValuableInfos;

    for (i = 0, itrOfValuableInfos = mValuableInfos.begin();
         (i < lengthOfItSideWays) && (itrOfValuableInfos != mValuableInfos.end());
         i++, itrOfValuableInfos++) {
         sumBuyVolume += itrOfValuableInfos->mVolumeBuyOneDay;
         sumTurnOverBuy += itrOfValuableInfos->mTurnOverBuyOneDay;

         sumSaleVolume += itrOfValuableInfos->mVolumeSaleOneDay;
         sumTurnOverSale += itrOfValuableInfos->mTurnOverSaleOneDay;
    }

    buyPrice  = sumTurnOverBuy  / sumBuyVolume;
    salePrice = sumTurnOverSale / sumSaleVolume;

    // Step 2: Is the volume of Big B larger than Big S?
    if (sumSaleVolume > sumBuyVolume) {
        return false;
    }

    // Step 3: Is the price of Big S larger than Big S?
    if (salePrice > buyPrice) {
        return false;
    }

    return true;
}

/*
 * Phase Two: Most of time, the bastard has enough shares and step over the
 *            phase one now. What he need to do is mainly two things:
 *            1) Fly and Fly;
 *            2) Crash and Fly;
 *
 * Characteristics :
 *            1) There are a lot of residue shares filtered by Big Deal;
 *            2) The cost of the remaining shares is equals to or a little higher
 *               than the current price.
 *            3) The price has been raised up a little, but just a little.
 *            4) Comparing with days before, the Volume of Big Deal declines somehow.
 *            5) And the most important, it MUST had gone through Phase One.
*/
bool PriceDiscover::isInPhaseTwo(const std::string& aTableName, const std::string& aLatestDay) {
    //Step 1: check whethe it is in the state of Phase One
    //Step 2: whether the price of today(latest day) has been raised a little

    return true;
}

/*
 * Phase Three: Fly in the Sky and enjoy yourself, drop most of your shares in this phase.
 *
 * Characteristics :
 *            1) The cost of remaining shares are much lower than the current price;
 *            2) There are still many remaining shares, which decide whether you can work
 *               out full of money;
 *            3) At the beginning of the day, turn over rate is very high and it is time to go.
*/
bool PriceDiscover::isInPhaseThree(const std::string& aTableName) {
    return true;
}

bool PriceDiscover::isSteadySideWays(int aDaysCount) {
    if (!getPriceDatasFromResultTable()) {
        return false;
    }

    std::list<PriceData>::iterator iterPriceData = mPriceDatas.begin();
    int i = 0;
    double absFloat = 0.0;
    double realFloat = 0.0;
    double startPrice = iterPriceData->mBeginPrice;
    double endPrice = 0.0;

    for (iterPriceData = mPriceDatas.begin();
         (iterPriceData != mPriceDatas.end()) && (i < aDaysCount);
         iterPriceData++) {
         if ((iterPriceData->mEndPrice > MIN_PRICE)
             && (iterPriceData->mBeginPrice > MIN_PRICE)) {
             absFloat += fabs((iterPriceData->mEndPrice - iterPriceData->mBeginPrice)/iterPriceData->mBeginPrice);
             i++;
         }

    }

    if (i < aDaysCount) {
        //TODO: if there is no enough records, what shall we return??
        return false;
    }

    endPrice = (--iterPriceData)->mEndPrice;
    realFloat = (endPrice - startPrice)/startPrice;

    if (endPrice < MIN_PRICE
        || startPrice < MIN_PRICE) {
        return false;
    }

    return isPriceSideWays(realFloat, absFloat, aDaysCount);
}

bool PriceDiscover::getNeededInfoFromTable(const std::string& aTableName) {
    std::string sql;
    sqlite3_stmt* stmt = NULL;
    std::string columns;

    columns += DATE;
    columns += ",";
    columns += VOLUME_SALE;
    columns += ",";
    columns += TURNOVER_SALE;
    columns += ",";
    columns += VOLUME_BUY;
    columns += ",";
    columns += TURNOVER_BUY;
    columns += ",";
    columns += VOLUME_FLOWIN_ONE_DAY;
    columns += ",";
    columns += TURNOVER_FLOWIN_ONE_DAY;
    columns += ",";
    columns += BEGIN_PRICE;
    columns += ",";
    columns += END_PRICE;

    sql = SELECT_COLUMNS_IN_ORDER(aTableName, columns, DATE, false);

    int ret = sqlite3_prepare(mOriginDB, sql.c_str(), -1, &stmt, NULL);
    if (ret != SQLITE_OK) {
         LOGE(LOGTAG, "Fail to prepare stmt to Table:%s, intRet:%d, errorMessage:%s", aTableName.c_str(), ret, sqlite3_errmsg(mOriginDB));
        return false;
    }


    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ValueAbleInfo tmpValueAbleInfo;

        tmpValueAbleInfo.mDate = (char*)sqlite3_column_text(stmt, 0);
        tmpValueAbleInfo.mVolumeSaleOneDay = sqlite3_column_int(stmt, 1);
        tmpValueAbleInfo.mTurnOverSaleOneDay = sqlite3_column_double(stmt, 2);
        tmpValueAbleInfo.mVolumeBuyOneDay = sqlite3_column_int(stmt, 3);
        tmpValueAbleInfo.mTurnOverBuyOneDay = sqlite3_column_double(stmt, 4);
        tmpValueAbleInfo.mTurnOverFlowInOneDay = sqlite3_column_double(stmt, 5);
        tmpValueAbleInfo.mVolumeFlowInOneDay = sqlite3_column_int(stmt, 6);
        tmpValueAbleInfo.mBeginPrice = sqlite3_column_double(stmt, 7);
        tmpValueAbleInfo.mEndPrice = sqlite3_column_double(stmt, 8);

        mValuableInfos.push_back(tmpValueAbleInfo);
    }

    if (SQLITE_OK != sqlite3_finalize(stmt)) {
        LOGE(LOGTAG, "Fail to finalize stmt in PriceDiscover::isDBSteadySideWays aDBName:%s", mDBName.c_str());
        return false;
    }

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

bool PriceDiscover::getPriceDatasFromResultTable() {

    std::list<ValueAbleInfo>::iterator itrValuAbleInfo = mValuableInfos.begin();
    while (itrValuAbleInfo != mValuableInfos.end()) {
        PriceData tmpPriceData;

        tmpPriceData.mDate = itrValuAbleInfo->mDate;
        tmpPriceData.mBeginPrice = itrValuAbleInfo->mBeginPrice;
        tmpPriceData.mEndPrice = itrValuAbleInfo->mEndPrice;
        mPriceDatas.push_back(tmpPriceData);
        itrValuAbleInfo++;
    }

    return true;
}

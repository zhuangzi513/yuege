#undef LOGTAG
#define LOGTAG "Forecaster"

#include "Forecaster.h"
#include "ErrorDefines.h"
#include "UtilsDefines.h"
#include "DBFilter.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#define DEFAULT_LASTING_LEN 3
#define MIN_TURNOVER 100

static double sumShots   = 0.0;
static double sumForcasters = 0.0;
static double sumIncome = 0.0;

static std::list<double> sPre15Flowins;

std::string Forecaster::mResultTableName = "FilterResult";

Forecaster::Forecaster() {
}

Forecaster::~Forecaster() {
}

bool Forecaster::forecasteThroughTurnOver(const std::string& aDBName) {
    std::list<Forecaster::DateRegion> recommandBuyDateRegions;
    getRecommandBuyDateRegions(Forecaster::CONTINUE_FLOWIN_PRI, aDBName, recommandBuyDateRegions);
    getHitRateOfBuying(aDBName, recommandBuyDateRegions);

    return true;
}

// Continue Flowin for MIN_LASTING days
// (FLOWIN - FLOWOUT) / FLOWIN >= 30%
// DIFF = PRICE_START_FLOWIN_DAY - PRICE_NOW
// DIFF / PRICE_START_FLOWIN_DAY < 5%
bool Forecaster::forecasteFromFirstPositiveFlowin(const std::string& aDBName) {
    std::list<Forecaster::DateRegion> recommandBuyDateRegions;
    getRecommandBuyDateRegions(Forecaster::CONTINUE_FLOWIN_FROM_FIRST_POSITIVE, aDBName, recommandBuyDateRegions);
    getHitRateOfBuying(aDBName, recommandBuyDateRegions);

    return true;
}

bool Forecaster::getBuyDateRegionsContinueFlowin(const std::string& aDBName, std::list<Forecaster::DateRegion>& recommandBuyDateRegions) {
    std::string sql, targetColumns;
    sqlite3_stmt* stmt = NULL;
    int intRet = -1;

    targetColumns += DATE;
    targetColumns += ",";
    targetColumns += TURNOVER_FLOWIN_TEN_DAYS;
    targetColumns += ",";
    targetColumns += TURNOVER_FLOWIN_ONE_DAY;
    sql = SELECT_COLUMNS(mResultTableName, targetColumns);

    LOGD(LOGTAG, "sql:%s", sql.c_str());
    intRet = sqlite3_prepare(mOriginDB,
                          sql.c_str(),
                          -1,
                          &stmt,
                          NULL);

    if (intRet != SQLITE_OK) {
        LOGE(LOGTAG, "Fail to prepare stmt to Table:%s", mResultTableName.c_str());
        return false;
    }

    Forecaster::DateRegion tmpDateRegion;
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

bool Forecaster::getBuyDateRegionsContinueFlowinPri(const std::string& aDBName, std::list<Forecaster::DateRegion>& recommandBuyDateRegions) {
    std::string sql, targetColumns;
    sqlite3_stmt* stmt = NULL;
    int intRet = -1;

    targetColumns += DATE;
    targetColumns += ",";
    targetColumns += TURNOVER_FLOWIN_TEN_DAYS;
    targetColumns += ",";
    targetColumns += TURNOVER_FLOWIN_ONE_DAY;
    sql = SELECT_COLUMNS(mResultTableName, targetColumns);

    LOGD(LOGTAG, "sql:%s", sql.c_str());
    intRet = sqlite3_prepare(mOriginDB,
                          sql.c_str(),
                          -1,
                          &stmt,
                          NULL);

    if (intRet != SQLITE_OK) {
        LOGE(LOGTAG, "Fail to prepare stmt to Table:%s", mResultTableName.c_str());
        return false;
    }

    Forecaster::DateRegion tmpDateRegion;
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

bool Forecaster::getBuyDateRegionsContinueFlowinFFP(const std::string& aDBName, std::list<Forecaster::DateRegion>& recommandBuyDateRegions) {
    std::string sql, targetColumns;
    sqlite3_stmt* stmt = NULL;
    int intRet = -1;

    targetColumns += DATE;
    targetColumns += ",";
    targetColumns += TURNOVER_SALE;
    targetColumns += ",";
    targetColumns += TURNOVER_BUY;
    targetColumns += ",";
    targetColumns += TURNOVER_FLOWIN_ONE_DAY;
    targetColumns += ",";
    targetColumns += TURNOVER_FLOWIN_TEN_DAYS;
    targetColumns += ",";
    targetColumns += BEGIN_PRICE;
    targetColumns += ",";
    targetColumns += END_PRICE;
    sql = SELECT_COLUMNS(mResultTableName, targetColumns);

    LOGD(LOGTAG, "sql:%s", sql.c_str());
    intRet = sqlite3_prepare(mOriginDB,
                          sql.c_str(),
                          -1,
                          &stmt,
                          NULL);

    if (intRet != SQLITE_OK) {
        LOGE(LOGTAG, "Fail to prepare stmt to Table:%s, errno:%d", mResultTableName.c_str(), errno);
        return false;
    }

    Forecaster::DateRegion tmpDateRegion;
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
                    LOGD(LOGTAG, "Meet the contition to start counting:flowinOneDay:%f, turnoverSale:%f, DBName:%s, date:%s", flowinOneDay, turnoverSale, aDBName.c_str(), date.c_str());
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
                LOGD(LOGTAG, "Meet the contition to start the newBuySaleBlockStarted, DBName:%s, date:%s", aDBName.c_str(), date.c_str());
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
                LOGD(LOGTAG, "Meet the contition to end the newBuySaleBlockStarted, DBName:%s, date:%s", aDBName.c_str(), date.c_str());
                recommandBuyDateRegions.push_back(tmpDateRegion);
                //reset everything
                tmpDateRegion.mStartDate = tmpDateRegion.mEndDate = "";
                newContiniousBlockStarted = false;
                newBuySaleBlockStarted = false;
            }
        }
    }
    if (SQLITE_OK != sqlite3_finalize(stmt)) {
        LOGE(LOGTAG, "Fail to finalize stmt in getBuyDateRegionsContinueFlowinFFP aDBName:%s", aDBName.c_str());
        return false;
    }

    return true;
}

bool Forecaster::getRecommandBuyDateRegions(const int throughWhat, const std::string& aDBName, std::list<Forecaster::DateRegion>& recommandBuyDateRegions) {
    LOGD(LOGTAG, "Enter getRecommandBuyDateRegions, aDBName:%s", aDBName.c_str());

    switch(throughWhat) {
      case Forecaster::CONTINUE_FLOWIN:
          getBuyDateRegionsContinueFlowin(aDBName, recommandBuyDateRegions);
          break;
      case Forecaster::CONTINUE_FLOWIN_PRI:
          getBuyDateRegionsContinueFlowinPri(aDBName, recommandBuyDateRegions);
          break;
      case Forecaster::CONTINUE_FLOWIN_FROM_FIRST_POSITIVE:
          getBuyDateRegionsContinueFlowinFFP(aDBName, recommandBuyDateRegions);
          break;
      default:
          LOGE(LOGTAG, "Unkown type to get RecommandBuyDateRegions:%d", throughWhat);
          return false;
    }
    
    return true;
}



//FIXME: the recommandBuyRegions should be of const
bool Forecaster::getHitRateOfBuying(const std::string& aDBName, std::list<Forecaster::DateRegion>& recommandBuyRegions) {
    LOGD(LOGTAG, "Enter getHitRateOfBuying, aDBName:%s", aDBName.c_str());
    if (recommandBuyRegions.size() < 1) {
        return true;
    }
    std::string sql, targetColumns;
    sqlite3_stmt* stmt = NULL;
    int intRet = -1;

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
        LOGE(LOGTAG, "Fail to prepare stmt to Table: %s, ret:%d", mResultTableName.c_str(), intRet);
        return false;
    }

    int index = 0;
    std::list<Forecaster::DateRegion>::iterator itr = recommandBuyRegions.begin();
    std::list<Forecaster::PriceRegion> realPriceRegions;
    Forecaster::PriceRegion tmpPriceRegion;

    std::list<Forecaster::DateRegion>::iterator debugDateRegionItr = recommandBuyRegions.begin();
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
        LOGE(LOGTAG, "Fail to finalize the stmt to finalize table:%s", mResultTableName.c_str());
        return false;
    }


    //Step 2: ensure UP DOWN
    if (realPriceRegions.size() != recommandBuyRegions.size()) {
        LOGE(LOGTAG, "The size of PriceRegions does not equal to that of recommandBuyRegions"); LOGI(LOGTAG, "realPriceRegions length:%d, recommandBuyRegions:%d", realPriceRegions.size(), recommandBuyRegions.size());
        return false;
    }

    std::list<Forecaster::PriceRegion>::iterator debugPriceRegionItr = realPriceRegions.begin();
    debugDateRegionItr = recommandBuyRegions.begin();
    while (debugPriceRegionItr!= realPriceRegions.end()) {
        LOGD(LOGTAG, "mStartDate in PriceRegions mBeginPrice:%f, mEndPrice:%f", (*debugPriceRegionItr).mBeginPrice, (*debugPriceRegionItr).mEndPrice);
        debugPriceRegionItr++;
    }

    double count = 0.0;
    std::list<Forecaster::PriceRegion>::iterator itrOfPriceRegion = realPriceRegions.begin();
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

    return true;
}

bool Forecaster::getGlobalHitRate(double& hitRate) {
    hitRate = sumShots/sumForcasters;
    double globaleIncome = sumIncome / sumForcasters;
    LOGI(LOGTAG, "\n\n======sum of reallly HitShots:%f, sum of Forcasters:%f, hitRate:%f, globaleIncome:%f\n\n", sumShots, sumForcasters, hitRate, globaleIncome);
    return true;
}

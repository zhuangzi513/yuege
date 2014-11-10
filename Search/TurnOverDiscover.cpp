#define LOGTAG   "TurnOverDiscover"

#include "TurnOverDiscover.h"
#include "DBFilter.h"
#include "DBWrapper.h"
#include "ErrorDefines.h"

#include <stdio.h>

const float LIMIT_RATIO = 1.1;

TurnOverDiscover::TurnOverDiscover(const std::string& aDBName, const std::string& aTableName)
        : mDBName(aDBName)
        , mTargetResultTableName(aTableName) {
    mSumTurnOvers.push_back(0);
    mSumTurnOvers.push_back(0);
    mBankerTurnOvers.push_back(0);
    mBankerTurnOvers.push_back(0);
}

TurnOverDiscover::~TurnOverDiscover() {
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

bool TurnOverDiscover::isTodayBankerInCharge(const std::string& aOriginTableName) {
    bool ret = false;

    ret = DBWrapper::getSumTurnOverOfTable(mDBName, aOriginTableName, mSumTurnOvers);
    if (!ret) {
        LOGD(LOGTAG, "Get in isDBBankerInCharging:%s", mDBName.c_str());
        return false;
    }
    LOGD(LOGTAG, "sum buy turnover:%lf, sale turnover:%lf", mSumTurnOvers[0], mSumTurnOvers[1]);

    ret = DBWrapper::getBankerTurnOverOfTable(mDBName, aOriginTableName, mBankerTurnOvers);
    if (!ret) {
        LOGD(LOGTAG, "Get in isDBBankerInCharging:%s", mDBName.c_str());
        return false;
    }
    LOGD(LOGTAG, "banker buy turnover:%lf, sale turnover:%lf", mBankerTurnOvers[0], mBankerTurnOvers[1]);

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

    if ((2 * sumBankerTurnOver) < sumTurnOver) {
        return false;
    }

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


//private
bool TurnOverDiscover::isDBFlowInFive(const std::string& aTableName) {
    return true;
}

bool TurnOverDiscover::isDBFlowInTen(const std::string& aTableName) {
    return true;
}

bool TurnOverDiscover::isDBFlowInMon(const std::string& aTableName) {
    return true;
}


#include "TurnOverDiscover.h"
#include "DBFilter.h"

TurnOverDiscover::TurnOverDiscover(const std::string& aDBName, DBFilter* pDBFilter)
        : mDatabaseName(aDBName)
        , mpDBFilter(pDBFilter) {
}

TurnOverDiscover::~TurnOverDiscover() {
    if (mpDBFilter) {
        delete mpDBFilter;
        mpDBFilter = NULL;
    }

    mDBSFlowIn.clear();
}

bool TurnOverDiscover::isDBBuyMoreThanSale(const std::string& aDBName)
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


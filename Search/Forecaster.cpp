#include "Forecaster.h"
#include "DBFilter.h"

Forecaster::Forecaster() {
}

Forecaster::~Forecaster() {
}

bool Forecaster::forecasteThroughTurnOver(const std::string& aDBName) {
    DBFilter *pDBFilter = new DBFilter(aDBName);
    pDBFilter->filterOriginDBByTurnOver();

    //std::list<DBFilter::DateRegion> recommandBuyDateRegions;
    //pDBFilter->getRecommandBuyDateRegions(DBFilter::CONTINUE_FLOWIN_PRI, aDBName, recommandBuyDateRegions);
    //pDBFilter->getHitRateOfBuying(aDBName, recommandBuyDateRegions);
    return true;
}

// Continue Flowin for MIN_LASTING days
// (FLOWIN - FLOWOUT) / FLOWIN >= 30%
// DIFF = PRICE_START_FLOWIN_DAY - PRICE_NOW
// DIFF / PRICE_START_FLOWIN_DAY < 5%
bool Forecaster::forecasteFromFirstPositiveFlowin(const std::string& aDBName) {
    DBFilter *pDBFilter = new DBFilter(aDBName);
    //pDBFilter->clearTableFromOriginDB(aDBName, DBFilter::mResultTableName);
    //pDBFilter->filterOriginDBByTurnOver(aDBName, 50000, 100000);

    std::list<DBFilter::DateRegion> recommandBuyDateRegions;
    pDBFilter->getRecommandBuyDateRegions(DBFilter::CONTINUE_FLOWIN_FROM_FIRST_POSITIVE, aDBName, recommandBuyDateRegions);
    pDBFilter->getHitRateOfBuying(aDBName, recommandBuyDateRegions);
    delete pDBFilter;

    return true;
}

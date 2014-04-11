#include "Forecaster.h"
#include "DBFilter.h"

Forecaster::Forecaster() {
}

Forecaster::~Forecaster() {
}

bool Forecaster::forecasteThroughTurnOver(const std::string& aDBName) {
    DBFilter *pDBFilter = new DBFilter(aDBName);
    pDBFilter->filterOriginDBByTurnOver(aDBName, 50000, 100000);

    std::list<DBFilter::DateRegion> recommandBuyDateRegions;
    pDBFilter->getRecommandBuyDateRegions(DBFilter::CONTINUE_FLOWIN_PRI, aDBName, recommandBuyDateRegions);
    pDBFilter->getHitRateOfBuying(aDBName, recommandBuyDateRegions);
    return true;
}

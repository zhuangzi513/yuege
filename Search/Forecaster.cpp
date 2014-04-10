#include "Forecaster.h"
#include "DBFilter.h"

Forecaster::Forecaster() {
}

Forecaster::~Forecaster() {
}

bool Forecaster::forecasteThroughTurnOver(const std::string& aDBName) {
    DBFilter *pDBFilter = new DBFilter();
    pDBFilter->filterOriginDBByTurnOver(aDBName, 100, 1000);

    std::list<DBFilter::DateRegion> recommandBuyDateRegions;
    pDBFilter->getRecommandBuyDateRegions(4, aDBName, recommandBuyDateRegions);
    pDBFilter->getHitRateOfBuying(aDBName, recommandBuyDateRegions);
    return true;
}

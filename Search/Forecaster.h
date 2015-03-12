#ifndef FORECASTER_H
#define FORECASTER_H

#include "sqlite3.h"

#include <list>
#include <string>


class Forecaster {
  public:
    Forecaster();
    ~Forecaster();

    class DateRegion;
    class PriceRegion;

    // Filter the details through TurnOver and compute the HintRate
    bool forecasteThroughTurnOver(const std::string& aDBName);

    bool forecasteFromFirstPositiveFlowin(const std::string& aDBName);

    bool getHitRateOfBuying(const std::string& tableName, std::list<DateRegion>& recommandBuyRegions);

    static bool getGlobalHitRate(double& hitRate);

    bool getRecommandBuyDateRegions(const int throughWhat, const std::string& aDBName, std::list<DateRegion>& recommandBuyDateRegions);

    class DateRegion {
      public:
        DateRegion()
            : mStartDate("")
            , mEndDate("") {
        }

        DateRegion(const DateRegion& dateRegion) {
            mStartDate = dateRegion.mStartDate;
            mEndDate   = dateRegion.mEndDate;
        }

      public:
        std::string mStartDate;
        std::string mEndDate;
    };

    class PriceRegion {
      public:
        PriceRegion()
            : mBeginPrice(0.0)
            , mEndPrice(0.0) {
        }

        PriceRegion(const PriceRegion& priceRegion) {
            mBeginPrice = priceRegion.mBeginPrice;
            mEndPrice   = priceRegion.mEndPrice;
        }
      
      public:
        double mBeginPrice;
        double mEndPrice;
    };

    /*
     * XXX:Remove it later
    */
    static std::string mResultTableName;

  private:

    enum {
      CONTINUE_FLOWIN,
      CONTINUE_FLOWIN_PRI,
      CONTINUE_FLOWIN_FROM_FIRST_POSITIVE
    };

    bool getBuyDateRegionsContinueFlowin(const std::string& aDBName, std::list<DateRegion>& recommandBuyDateRegions);
    bool getBuyDateRegionsContinueFlowinPri(const std::string& aDBName, std::list<DateRegion>& recommandBuyDateRegions);
    bool getBuyDateRegionsContinueFlowinFFP(const std::string& aDBName, std::list<DateRegion>& recommandBuyDateRegions);

    /*
     * The origin-databse being work at. And it will be resetted at the beginning of any filtering operations
     * on a origin-databse, usually in constructor of DBFilter. And it will be closed/released at the end of
     * filtering operations on a origin-database, usually in the destructor or where error occurs.
    */
    sqlite3* mOriginDB;
};


#endif

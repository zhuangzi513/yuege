#ifndef FORECASTER_H
#define FORECASTER_H

#include "sqlite3.h"

#include <list>
#include <string>


class Forecaster {
  class DateRegion {
    friend class Forecaster;
    public:
      DateRegion()
          : mStartDate("")
          , mEndDate("") {
      }

      DateRegion(const DateRegion& dateRegion) {
          mStartDate = dateRegion.mStartDate;
          mEndDate   = dateRegion.mEndDate;
      }

    private:
      std::string mStartDate;
      std::string mEndDate;
  };

  class PriceRegion {
    friend class Forecaster;
    public:
      PriceRegion()
          : mBeginPrice(0.0)
          , mEndPrice(0.0) {
      }

      PriceRegion(const PriceRegion& priceRegion) {
          mBeginPrice = priceRegion.mBeginPrice;
          mEndPrice   = priceRegion.mEndPrice;
      }
    
    private:
      double mBeginPrice;
      double mEndPrice;
  };

  enum {
    CONTINUE_FLOWIN,
    CONTINUE_FLOWIN_PRI,
    CONTINUE_FLOWIN_FROM_FIRST_POSITIVE
  };

  public:
    Forecaster(const std::string& aDBName);
    ~Forecaster();

    /*
     * Check whether the given DataBase flies up since the day.
     * Two key points:
     *    1) Days being counted in: how many days later from the day;
     *       For now, consider 20 days as default.
     *    2) What does the fly  means: 10%? 20%? 50%?
     *       For now, consider 20 as default.
     *
     * aDBName: The name of the target DataBase;
     * theDay : The special day
     *
     * return True if yes
    */
    bool isDBFlySinceTheDay(const std::string& aDBName, const std::string& theDay);

    /*
     * Compute how much it flies up during the days in the 2nd param.
     * 
     * aDBName: The name of the target DataBase;
     * theDays: The days should be counted in
     *
     * return how much it flies in percentage
    */
    double howMuchDBFlyDuringTheDays(const std::string& aDBName, const DateRegion& theDays);

    // Filter the details through TurnOver and compute the HintRate
    bool forecasteThroughTurnOver(const std::string& aDBName);

    bool forecasteFromFirstPositiveFlowin(const std::string& aDBName);

    static bool getGlobalHitRate(double& hitRate);

    bool getHitRateOfBuying(const std::string& tableName, std::list<DateRegion>& recommandBuyRegions);

    bool getRecommandBuyDateRegions(const int throughWhat, const std::string& aDBName, std::list<DateRegion>& recommandBuyDateRegions);

    /*
     * XXX:Remove it later
    */
    static std::string mResultTableName;

  private:
    bool getBuyDateRegionsContinueFlowin(const std::string& aDBName, std::list<DateRegion>& recommandBuyDateRegions);
    bool getBuyDateRegionsContinueFlowinPri(const std::string& aDBName, std::list<DateRegion>& recommandBuyDateRegions);
    bool getBuyDateRegionsContinueFlowinFFP(const std::string& aDBName, std::list<DateRegion>& recommandBuyDateRegions);

    /*
     * The origin-databse being work at. And it will be resetted at the beginning of any filtering operations
     * on a origin-databse, usually in constructor of DBFilter. And it will be closed/released at the end of
     * filtering operations on a origin-database, usually in the destructor or where error occurs.
    */
    sqlite3* mOriginDB;

    std::string mDBName;

    /*
     * The name of filter result table,
     * Default is the 'FilterResult20W'
    */
    std::string mFilterResultTableName;
};


#endif

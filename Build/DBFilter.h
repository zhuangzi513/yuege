#ifndef DB_FILTER_H
#define DB_FILTER_H

#include <stdlib.h>
#include <string>
#include <list>
#include "sqlite3.h"

#define MAX_COMPUTE_LEN 10

/*
 * This class is used to filter the OriginDatabase and save the result
 * into the result-tables in the OriginDatabase.
 * For now, five types Filtered Result:
 * Sale_Big : filtered by Turnover
 * Buy_Big  : filtered by Turnover
 *
 * Price of Sale_Big : filtered by Turnover
 * Price of Buy_Big  : filtered by Turnover
 *
 * D-value of Sale_Big & Buy_Big : filtered by Turnover
 *
 * Table of Filter Result:
 *    ID   | 20131217     | 20131218     | 20131219     |
 * 600000  | FilterResult | FilterResult | FilterResult |
 *
**/

class DBFilter {
  public:
    class BaseResultData;
    class DateRegion;
    class PriceRegion;
    friend class DBSearcher;

    enum {
      CONTINUE_FLOWIN,
      CONTINUE_FLOWIN_PRI,
      CONTINUE_FLOWIN_FROM_FIRST_POSITIVE
    };

    DBFilter(const std::string& aDBName);
    ~DBFilter();
    /*
     * Remove the table named "aTableName" in the origin-database
     *
     * aTableName: The name of the table to be cleared
     *
     * return true if successfully remove the content of the target table
    */
    bool removeTableFromOriginDB(const std::string& aTableName);

    /*
     * Clear the content of the table named "aTableName" in the origin-database
     *
     * aTableName: The name of the table to be cleared
     *
     * return true if successfully cleared the content of the target table
    */
    bool clearTableFromOriginDB(const std::string& aTableName);

    /*
     * Filter all the origin-table(s) in the origin database with the specific turnovers in mFilterTurnOvers
     * and save the results into the result-tables.
     *
     * return true if successfully filter the origin-database
    */
    bool filterOriginDBByTurnOver();

    /*
     * Filter some origin-table(s) in a database and update the filter result to the target result-table.
     * Diffing from the mem-func of "filterOriginDBByTurnOver", only the new added origin-table(s) being
     * filtered here.
     *
     * aResultTableName: The name of target result-table;
     * aMinTurnOver:     The bottom edge of the specific turnover region;
     * aMinTurnOver:     The top edge of the specific turnover region;
     *
     * return true if successfully update the result-table in the origin-database
    */
    bool updateFilterResultByTurnOver(const std::string& aResultTableName, const int aMinTurnover, const int aMaxTurnOver);

    bool getHitRateOfBuying(const std::string& tableName, std::list<DateRegion>& recommandBuyRegions);

    static bool getGlobalHitRate(double& hitRate);
    bool getRecommandBuyDateRegions(const int throughWhat, const std::string& aDBName, std::list<DBFilter::DateRegion>& recommandBuyDateRegions);

  private:
    /*
     *
    */
    bool init();

    /*
     * Open the origin-database with the mDBName, it is called only one time for a origin-databse filtering
     * when initializing the DBFilter in mem-func "init()";
     *
     * return true if open the origin-database successfully
    */
    bool openOriginDB();

    /*
     * Close the origin-databse when finish filtering, it is called only one time for a origin-database filtering
     * in the destructor of the DBFilter.
     *
     * aDBName: The origin-database to be filtered;
     *
     * return true if close the origin-database successfully
    */
    bool closeOriginDB(const std::string& aDBName);

    /*
     * 
    */
    sqlite3* getDBByName(const std::string& DBName);

    /*
     * Create the target table named "aDBName" of type "index" in the origin-database.
     * In order to avoid calling "openTable(*)" everywhere during the life cycle of the
     * DBFilter instance, we must guarantee that all the result-tables have been created
     * already in the right format.
     * So, it is called only when initialization of the DBFilter instanc in the "init()".
     *
     * aType:       The type of the table, which decide the format of the table;
     * aDBName:     The name of the origin-database the table located in;
     * aTableName:  The name of the table to be created;
     *
     * return true if successfully created
    */
    bool openTable(int aType, const std::string& aTableName);

    /*
     * Check whether or not the origin-database contains the target table named "aTableName"
     *
     * return true if the target table exist in the sepecific origin-database
    */
    bool isTableExist(const std::string& aDBName, const std::string& aTableName);

    bool getBuyDateRegionsContinueFlowin(const std::string& aDBName, std::list<DBFilter::DateRegion>& recommandBuyDateRegions);
    bool getBuyDateRegionsContinueFlowinPri(const std::string& aDBName, std::list<DBFilter::DateRegion>& recommandBuyDateRegions);
    bool getBuyDateRegionsContinueFlowinFFP(const std::string& aDBName, std::list<DBFilter::DateRegion>& recommandBuyDateRegions);

    /*
     * Get all the previous filter results in the specific result-table named "aResultTableName", and save them
     * in the out arg "outFilterResults"
     *
     * aResultTableName: The name of target result-table;
     * outFilterResults: The out list accepting the filtered results in the target result-table
     *
     * return true if successfully get the filtered result in the target result-table
    */
    bool getExistingFilterResults(const std::string& aResultTableName, std::list<BaseResultData>& outFilterResults);

    /*
     * After filter/compute origin-table, save the BaseResults into the result-table named "aResultTableName"
     *
     * aResultTableName:     The name of target result-table;
     * 
     * return true if successfully save the filter result to the result-table
    */
    bool saveBaseResultInBatch(const std::string& aResultTableName);

    /*
     * Compute the filtered result for 5/10/20 days before, and save the result into the inout arg "inoutBaseResult"
     *
     * aDaysBefore:           The days before we should cover
     * aExistingBaseResults : The previous filtered results in the result-tables
     * inoutBaseResult:       The BaseResult contains the base info for current day and we should save the computing result
     *                        to it.
     *
     * return true if successfully compute the filtered result
    */
    bool computeFilterResultForLev(int aDaysBefore, std::list<BaseResultData>& aExistingBaseResults, BaseResultData& inoutBaseResult);

    /*
     * Mainly two things here:
     *     1) Compute the sum of TurnOver and Volume in the table of name "mTmpResultTableName", which is the middle-ware
     *        result-table and container the filtered deal details.
     *     2) Since the middle-ware result-table of mTmpResultTableName contains the filtered details for one day, so get
     *        the BaseResultData from the mTmpResultTableName and save it to mBaseResultDatas, which is used to compute the
     *        till-now results, such as 5/10/20 days flowin till now.
     *
     * aMiddleWareTableName: The name of middle-ware result-table;
     * aResultTableName:     The name of target result-table;
     * aOriginTbaleName:     The name of origin-table to be operated on;
     * aBeginningPrice:      The beginning-price of one day;
     * aEndingPrice:         The ending-price of one day;
     * 
     * return true if successfully compute the filter result and save it to target result-table 
    */
    bool computeResultFromTable(const std::string& aMiddleWareTableName, const std::string& aResultTableName,
                                const std::string& aOriginTableName, const double aBeginningPrice, const double aEndingPrice);

    /*
     * Remove the table of name  "aTableName" from the origin-database
     *
     * aTableName : The target table to be cleared.
     *
     * return true if successfully remove the table
    */
    bool removeTable(const std::string& aTableName);

    /*
     * Clear everything in the table of name  "aTableName"
     *
     * aTableName : The target table to be cleared.
     *
     * return true if successfully clear the table
    */
    bool clearTable(const std::string& aTableName);

    /*
     * Get the beginning/ending price of in the origin-table. And since the origin-table is the deal details of one day,
     * so this mem-func is used to get the beginning/ending price for one day.
     *
     * aOriginTableName:  The name of the origin-table for the specific day;
     * outBeginningPrice: The beginning price gotten from the target origin-table;
     * outEndingPrice:    The ending price gotten from the target origin-table;
     *
     * return true if get the beginning/ending price successfully from the origin-table
    */
    bool getBeginAndEndPrice(const std::string& aOriginTableName, double& outBeginningPrice, double& outEndingPrice);

    /*
     * Filter an origin-table and save the result into the middle-ware result-table
     *
     * aOriginTableName: The name of the origin-table;
     * aMinTurnOver:     The bottom edge of turnover.
     *
     * return true if filter the origin-table successfully with the specific turnover
    */
    bool filterTableByTurnOver(const std::string& aOriginTableName, const int aMinTurnover);

    /*
     * Filter some origin-table(s) in a origin-database and save the filter-results into a result-table
     *
     * aResultTableName:  The name of the result-table;
     * aMinTurnOver:      The bottom edge of turnover;
     * aOriginTableNames: The origin-table(s) to be filtered.
     *
     * return true if all the specific origin-tables are successfully filtered with the specific turnover
    */
    bool filterTablesByTurnOver(const std::string& aResultTableName, const int aMinTurnover, std::list<std::string>& aOriginTableNames);

  public:
    class BaseResultData {
      public:
        BaseResultData()
            : mSaleVolume(0)
            , mBuyVolume(0)
            , mSaleTurnOver(0.0)
            , mBuyTurnOver(0.0)
            , mSalePrice(0.0)
            , mBuyPrice(0.0)
            , mEndPrice(0.0)
            , mBeginPrice(0.0)
            , mTurnOverFlowInOneDay(0.0)
            , mVolumeFlowInOneDay(0)
            , mTurnOverFlowInFiveDays(0.0)
            , mVolumeFlowInFiveDays(0)
            , mTurnOverFlowInTenDays(0.0)
            , mVolumeFlowInTenDays(0)
            , mTurnOverFlowInMonDays(0.0)
            , mVolumeFlowInMonDays(0)
            , mDate("") {
        }

        BaseResultData(const BaseResultData& baseResultData) {
            mSaleVolume       = baseResultData.mSaleVolume;
            mBuyVolume        = baseResultData.mBuyVolume;
            mSaleTurnOver     = baseResultData.mSaleTurnOver;
            mBuyTurnOver      = baseResultData.mBuyTurnOver;
            mSalePrice        = baseResultData.mSalePrice;
            mBuyPrice         = baseResultData.mBuyPrice;
            mBeginPrice       = baseResultData.mBeginPrice;
            mEndPrice         = baseResultData.mEndPrice;
            mTurnOverFlowInOneDay  = baseResultData.mTurnOverFlowInOneDay;
            mVolumeFlowInOneDay    = baseResultData.mVolumeFlowInOneDay;
            mTurnOverFlowInFiveDays = baseResultData.mTurnOverFlowInFiveDays;
            mVolumeFlowInFiveDays   = baseResultData.mVolumeFlowInFiveDays;
            mTurnOverFlowInTenDays  = baseResultData.mTurnOverFlowInTenDays;
            mVolumeFlowInTenDays    = baseResultData.mVolumeFlowInTenDays;
            mTurnOverFlowInMonDays  = baseResultData.mTurnOverFlowInMonDays;
            mVolumeFlowInMonDays    = baseResultData.mVolumeFlowInMonDays;
            mDate             = baseResultData.mDate;
        }

      public:
        //Info of current day
        int mSaleVolume;
        int mBuyVolume;
        double mSaleTurnOver;
        double mBuyTurnOver;
        double mSalePrice;
        double mBuyPrice;
        double mBeginPrice;
        double mEndPrice;
        double mTurnOverFlowInOneDay;
        int mVolumeFlowInOneDay;

        //Info of previous days
        double mTurnOverFlowInFiveDays;
        int mVolumeFlowInFiveDays;

        double mTurnOverFlowInTenDays;
        int mVolumeFlowInTenDays;

        double mTurnOverFlowInMonDays;
        int mVolumeFlowInMonDays;
        std::string mDate;
    };

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
     * The filtered results used to fill the result-tables.  we should reset it before any filtering for a
     * result table. For example, before computing FilterResult20W, we should clear content stored in the
     * the mBaseResultDatas, which is used to fill FilterResult10W. 
    */
    std::list<BaseResultData> mBaseResultDatas;

    /*
     * The origin-databse being work at. And it will be resetted at the beginning of any filtering operations
     * on a origin-databse, usually in constructor of DBFilter. And it will be closed/released at the end of
     * filtering operations on a origin-database, usually in the destructor or where error occurs.
    */
    sqlite3* mOriginDB;

    /*
     * The name of the origin-database.
    */
    std::string mDBName;

    /*
     * All the origin-tables in the origin-database.
    */
    std::list<std::string> mOriginTableNames;

    /*
     * The names of new added origin-table(s) of the origin-databse. In the case of database updation, only
     * the new added origin-table(s) are computed/filtered and then saved into the result-tables.
    */
    std::list<std::string> mNewAddedTables;

    /*
     * XXX:Remove it later
    */
    static std::string mResultTableName;

    /*
     * The names of result-tables, for now only 5 result tables for one origin-databse:
     *     1) FilterResult20W:  result-table being filtered through TurnOver of 20W
     *     1) FilterResult40W:  result-table being filtered through TurnOver of 40W
     *     1) FilterResult60W:  result-table being filtered through TurnOver of 60W
     *     1) FilterResult80W:  result-table being filtered through TurnOver of 80W
     *     1) FilterResult100W: result-table being filtered through TurnOver of 100W
    */
    static std::list<std::string> mResultTableNames;
    static std::list<double> mFilterTurnOvers;

    /*
     * The name of middle-ware result-table, it contains the origin-details filtered through TurnOver.
     * It is used to compute the BaseResultData for one day.
    */
    static std::string mTmpResultTableName;
    static std::string mDiffBigBuySaleTableName;
};

#endif

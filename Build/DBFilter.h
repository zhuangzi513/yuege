#ifndef DB_FILTER_H
#define DB_FILTER_H

#include <stdlib.h>
#include <string>
#include <list>
#include "sqlite3.h"

#define MAX_COMPUTE_LEN 10

/*
 * This class is used to filter the OriginDatbase and save the result
 * to another Database.
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
    bool clearTableFromOriginDB(const std::string& aDBName, const std::string& aTableName);

    /*
     * Filter all the origin-table(s) in the origin database with the specific turnovers in mFilterTurnOvers
     * and save the results into the result-tables.
     *
     * aDBName:      The origin-data to be filtered;
    */
    bool filterOriginDBByTurnOver(const std::string& aDBName);

    /*
     * Filter some origin-table(s) in a database and update the filter result to the target result-table.
     * Diffing from the mem-func of "filterOriginDBByTurnOver", only the new added origin-table(s) being
     * filtered here.
     *
     * aDBName:          The origin-data to be filtered;
     * aResultTableName: The name of target result-table;
     * aMinTurnOver:     The bottom edge of the specific turnover region;
     * aMinTurnOver:     The top edge of the specific turnover region;
    */
    bool updateFilterResultByTurnOver(const std::string& aDBName, const std::string& aResultTableName, const int aMinTurnover, const int aMaxTurnOver);

    bool getHitRateOfBuying(const std::string& tableName, std::list<DateRegion>& recommandBuyRegions);

    static bool getGlobalHitRate(double& hitRate);
    bool getRecommandBuyDateRegions(const int throughWhat, const std::string& aDBName, std::list<DBFilter::DateRegion>& recommandBuyDateRegions);

  private:
    bool getBuyDateRegionsContinueFlowin(const std::string& aDBName, std::list<DBFilter::DateRegion>& recommandBuyDateRegions);
    bool getBuyDateRegionsContinueFlowinPri(const std::string& aDBName, std::list<DBFilter::DateRegion>& recommandBuyDateRegions);
    bool getBuyDateRegionsContinueFlowinFFP(const std::string& aDBName, std::list<DBFilter::DateRegion>& recommandBuyDateRegions);
    bool openOriginDB(const std::string& name);
    bool closeOriginDB(const std::string& name);
    bool openTable(int index, const std::string& aDBName, const std::string& aTableName);
    sqlite3* getDBByName(const std::string& DBName);
    bool isTableExist(const std::string& DBName, const std::string& tableName);

    bool getExistingFilterResults(const std::string& aDBName, const std::string& aResultTableName, std::list<BaseResultData>& outFilterResults);

    /*
     * After filter/compute origin-table, save the BaseResults into a result-table.
     *
     * aDBName:              The origin-data to be filtered;
     * aResultTableName:     The name of target result-table;
     * 
    */
    bool saveBaseResultInBatch(const std::string& aDBName, const std::string& aResultTableName);

    /*
     * Mainly two things here:
     *     1) Compute the sum of TurnOver and Volume in the table of name "mTmpResultTableName", which is the middle-ware
     *        result-table and container the filtered deal details.
     *     2) Since the middle-ware result-table of mTmpResultTableName contains the filtered details for one day, so get
     *        the BaseResultData from the mTmpResultTableName and save it to mBaseResultDatas, which is used to compute the
     *        till-now results, such as 5/10/20 days flowin till now.
     *
     * aDBName:              The origin-data to be filtered;
     * aMiddleWareTableName: The name of middle-ware result-table;
     * aResultTableName:     The name of target result-table;
     * aOriginTbaleName:     The name of origin-table to be operated on;
     * aBeginningPrice:      The beginning-price of one day;
     * aEndingPrice:         The ending-price of one day;
     * 
    */
    bool computeResultFromTable(const std::string& aDBName, const std::string& aMiddleWareTableName, const std::string& aResultTableName,
                                const std::string& aOriginTableName, const double aBeginningPrice, const double aEndingPrice);

    /*
     * Clear everything in the table of name  "aTableName"
     *
     * aTableName : The target table to be cleared.
    */
    bool clearTable(const std::string& aTableName);

    /*
     * Get the beginning/ending price of in the origin-table. And since the origin-table is the deal details of one day,
     * so this mem-func is used to get the beginning/ending price for one day.
     *
     * aOriginTableName:  The name of the origin-table for the specific day;
     * outBeginningPrice: The beginning price gotten from the target origin-table;
     * outEndingPrice:    The ending price gotten from the target origin-table;
    */
    bool getBeginAndEndPrice(const std::string& aOriginTableName, double& outBeginningPrice, double& outEndingPrice);

    /*
     * Filter an origin-table and save the result into the middle-ware result-table
     *
     * aDBName:          The name of the origin-database;
     * aOriginTableName: The name of the origin-table;
     * aMinTurnOver:     The bottom edge of turnover.
    */
    bool filterTableByTurnOver(const std::string& aDBName, const std::string& aOriginTableName, const int aMinTurnover);

    /*
     * Filter some origin-table(s) in a origin-database and save the filter-results into a result-table
     *
     * aDBName:           The name of the origin-database;
     * aResultTableName:  The name of the result-table;
     * aMinTurnOver:      The bottom edge of turnover;
     * aOriginTableNames: The origin-table(s) to be filtered.
    */
    bool filterTablesByTurnOver(const std::string& aDBName, const std::string& aResultTableName, const int aMinTurnover, std::list<std::string>& aOriginTableNames);

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
            , mPureFlowInOneDay(0.0)
            , mSumFlowInTenDays(0.0) {
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
            mPureFlowInOneDay = baseResultData.mPureFlowInOneDay;
            mSumFlowInTenDays = baseResultData.mSumFlowInTenDays;
            mDate             = baseResultData.mDate;
        }

      public:
        int mSaleVolume;
        int mBuyVolume;
        double mSaleTurnOver;
        double mBuyTurnOver;
        double mSalePrice;
        double mBuyPrice;
        double mBeginPrice;
        double mEndPrice;
        double mPureFlowInOneDay;
        double mSumFlowInTenDays;
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

    std::list<BaseResultData> mBaseResultDatas;

    /*
     * The origin-databse being work at. And it will be resetted at the beginning of any filtering operations
     * on a origin-databse, usually in constructor of DBFilter. And it will be closed/released at the end of
     * filtering operations on a origin-database, usually in the destructor or where error occurs.
    */
    sqlite3* mOriginDB;
    std::string mDBName;

    /*
     * 
    */
    static std::list<std::string> mTableNames;

    /*
     * The names of new added origin-table(s). In the case of database updation, only the new added origin-table(s)
     * is computed/filtered/saved.
    */
    static std::list<std::string> mNewAddedTables;
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

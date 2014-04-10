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

    DBFilter();
    ~DBFilter();

    /*
     * Filter the origin database with the aTurnover and save the result
     * into another database.
     */
    bool filterOriginDBByTurnOver(const std::string& aDBName, int aMinTurnover, int aMaxTurnover);
    bool getHitRateOfBuying(const std::string& tableName, std::list<DateRegion>& recommandBuyRegions);
    bool getRecommandBuyDateRegions(const int lastingLen, const std::string& aDBName, std::list<DBFilter::DateRegion>& recommandBuyDateRegions);

  private:
    bool openOriginDB(const std::string& name);
    bool closeOriginDB(const std::string& name);
    sqlite3* getDBByName(const std::string& DBName);
    bool isTableExist(const std::string& DBName, const std::string& tableName);

    bool getAllTablesOfDB(const std::string& tableName);

    bool saveBaseResultInBatch(const std::string& aDBName, const std::string& tableName);
    bool computeResultFromTable(const std::string& aDBName, const std::string& tmpTableName, const std::string& originTableName, const double beginningPrice, const double endingPrice);
    bool clearTable(const std::string& tableName);
    bool getBeginAndEndPrice(const std::string& tableName, double& beginningPrice, double& endingPrice);
    bool filterTableByTurnOver(const std::string& tableName, const int aMinTurnover);
    bool filterAllTablesByTurnOver(const std::string& tableName, const int aMinTurnover);

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
    static sqlite3* mOriginDB;
    static std::list<std::string> mTableNames;
    static std::string mResultTableName;
    static std::string mTmpResultTableName;
    static std::string mDiffBigBuySaleTableName;
};

#endif

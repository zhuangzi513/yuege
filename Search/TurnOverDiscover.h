#ifndef DISCOVER_BY_TURNOVER_H
#define DISCOVER_BY_TURNOVER_H

#include <list> 
#include <string> 
#include <vector> 

class DBFilter;
class sqlite3;

class TurnOverDiscover {
  public:

    class BankerResultInfo {
      public:
        BankerResultInfo() {};
        virtual ~BankerResultInfo() {};
        std::string mIsBankerIncharge;
        std::string mIsPositive;
        double mBuyToSale;
        std::string mDate;
    };

    TurnOverDiscover(const std::string& aDBName, const std::string& aTableName);
    ~TurnOverDiscover();

    // BuyTurnOver/SaleTurnOver > 1.2
    bool isDBBuyMoreThanSale(const std::string& aDBName);
    bool isDBFlowIn(const std::string& aDBName);
    bool isDBFlowInFiveDays();
    bool isDBFlowInTenDays();
    bool isDBFlowInMonDays();

    /*
     * Decide whethe or not the DB is in charged by Bankers in the previous days.
     *
     * aCount: The count of days before.
     *
     * return true if yes.
    */
    bool isDBBankedInDays(int aCount);

    /*
     * Compute the details of aOriginTableName and save the Banker info into the mBankerResultTable
    */
    void updateBankerResultTable();

    /*
     * Get the new added OriginTables
    */
    bool checkNewAddedOriginTables();

    /*
     * Compute the BankerInChargeInfo for a specified originTable
    */
    void getBankerInChargeInfoFromOriginTable(const std::string& aOriginTableName);

    /*
     * Get the BankerInChargeInfo for a specified originTable from the BankerResultTable
     *
     * aOriginTableName:    The name of target origin-table
     * outBankerResultInfo: The BankerResultInfo instance to save the info
     *
     * return true if successfully get the BandkerResultInfo
    */
    bool getBankerInChargeInfoFromBankerResultTable(const std::string& aOriginTableName, BankerResultInfo& outBankerResultInfo);

    /*
     * Compute the details of aOriginTableName and decide whether or not the DB is totally controlled by the Banker today.
     *
     * aOriginTableName: The name of target origin-table;
     *
     * return true if yes
    */
    bool isTodayBankerInCharge(const std::string& aOriginTableName);

    /*
     * Decide whether or not the current DB is controlled by a PositiveBanker.
     *
     * aOriginTableName: The name of target origin-table;
     *
     * return true if yes
    */
    bool isTodayPositiveBankerInCharge(const std::string& aOriginTableName);

    /*
     * Decide whether or not the current DB is controlled by a NagtiveBanker.
     *
     * aOriginTableName: The name of target origin-table;
     *
     * return true if yes
    */
    bool isTodayNagtiveBankerInCharge(const std::string& aOriginTableName);

    /*
     * Decide whether or not the current DB is controlled by a NeutralBanker.
     *
     * aOriginTableName: The name of target origin-table;
     *
     * return true if yes
    */
    bool isTodayNeutralBankerInCharge(const std::string& aOriginTableName);

    /*
     * Retrieve the BankerTurnOvers of this.
     *
     * outTurnOvers: The container of mBankerTurnOvers
    */
    void getBankerTurnOvers(std::vector<double>& outTurnOvers) const;

    /*
     * Decide whether the target OriginTable should be marked as SUNCK_IN
     * 
     * return true if yes
    */
    bool isTodaySuckIn(const std::string& aOriginTableName);

    /*
     * Decide whether the DB should be marked as SUNCK_IN according to the origin details
     * of previous days.
     *
     * count : The number of days should be counted in
     *
     * return true if yes
    */
    bool isPreviousDaysSuckIn(const int aCount);


  private:
    void init();
    bool isDBFlowInFive(const std::string& aTableName);
    bool isDBFlowInTen(const std::string& aTableName);
    bool isDBFlowInMon(const std::string& aTableName);

  private:
    sqlite3* mOriginDB;
    std::string mDBName;
    const static std::string mBankerResultTable;
    std::string mTargetResultTableName;
    std::vector<double> mSumTurnOvers;
    std::vector<double> mBankerTurnOvers;
    std::list<std::string> mNewAddedOriginTables;
    //std::list<ValueAbleInfo>::iterator mStartItr;
};

#endif

#ifndef DISCOVER_BY_TURNOVER_H
#define DISCOVER_BY_TURNOVER_H

#include <list> 
#include <string> 
#include <vector> 

class DBFilter;
class sqlite3;

class TurnOverDiscover {
  public:
    TurnOverDiscover(const std::string& aDBName, const std::string& aTableName);
    ~TurnOverDiscover();

    // BuyTurnOver/SaleTurnOver > 1.2
    bool isDBBuyMoreThanSale(const std::string& aDBName);
    bool isDBFlowIn(const std::string& aDBName);
    bool isDBFlowInFiveDays();
    bool isDBFlowInTenDays();
    bool isDBFlowInMonDays();

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


  private:
    bool isDBFlowInFive(const std::string& aTableName);
    bool isDBFlowInTen(const std::string& aTableName);
    bool isDBFlowInMon(const std::string& aTableName);

  private:
    sqlite3* mOriginDB;
    std::string mDBName;
    std::string mTargetResultTableName;
    std::vector<double> mSumTurnOvers;
    std::vector<double> mBankerTurnOvers;
    //std::list<ValueAbleInfo>::iterator mStartItr;
};

#endif

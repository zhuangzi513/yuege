#ifndef DISCOVER_BY_TURNOVER_H
#define DISCOVER_BY_TURNOVER_H

#include <list> 
#include <string> 

class DBFilter;

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

  private:
    bool isDBFlowInFive(const std::string& aTableName);
    bool isDBFlowInTen(const std::string& aTableName);
    bool isDBFlowInMon(const std::string& aTableName);

  private:
    sqlite3* mOriginDB;
    std::string mDBName;
    std::string mTargetResultTableName;
    std::list<ValueAbleInfo>::iterator mStartItr;
};

#endif

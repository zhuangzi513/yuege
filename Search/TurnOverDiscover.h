#ifndef DISCOVER_BY_TURNOVER_H
#define DISCOVER_BY_TURNOVER_H

#include <list> 
#include <string> 

class DBFilter;

class TurnOverDiscover {
  public:
    TurnOverDiscover(const std::string& aDBName, DBFilter* pDBFilter);
    ~TurnOverDiscover();

    bool isDBFlowIn(const std::string& aDBName);
    bool isDBFlowInFiveDays();
    bool isDBFlowInTenDays();
    bool isDBFlowInMonDays();

  private:
    bool isDBFlowInFive(const std::string& aTableName);
    bool isDBFlowInTen(const std::string& aTableName);
    bool isDBFlowInMon(const std::string& aTableName);

  private:
    std::string mDatabaseName;
    DBFilter* mpDBFilter;
    std::list<std::string> mDBSFlowIn;
};

#endif

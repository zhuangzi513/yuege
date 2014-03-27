#ifndef DB_FILTER_H
#define DB_FILTER_H

#include <stdlib.h>
#include <string>
#include <list>
#include "sqlite3.h"

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
    friend class DBSearcher;

    DBFilter();
    ~DBFilter();

    /*
     * Filter the origin database with the aTurnover and save the result
     * into another database.
     */
    bool filterOriginDBByTurnOver(const std::string& aDBName, int aMinTurnover, int aMaxTurnover);

  private:
    bool openOriginDB(const std::string& name);
    bool closeOriginDB(const std::string& name);
    sqlite3* getDBByName(const std::string& DBName);
    bool isTableExist(const std::string& DBName, const std::string& tableName);

    bool getAllTablesOfDB(const std::string& tableName);

    bool saveResultToResultTable(const std::string& aDBName, const std::string& tableName, const int turnoverSale, const float avgPriceSale, const int turnoverBuy, const float avgPriceBuy);
    bool computeResultFromTable(const std::string& aDBName, const std::string& tableName);
    bool clearTable(const std::string& tableName);
    bool filterTableByTurnOver(const std::string& tableName, const int aMinTurnover);
    bool filterAllTablesByTurnOver(const std::string& tableName, const int aMinTurnover);

  private:
    static sqlite3* mOriginDB;
    static std::list<std::string> mTableNames;
    static std::string mResultTableName;
    static std::string mTmpResultTableName;
    static std::string mDiffBigBuySaleTableName;
};

#endif

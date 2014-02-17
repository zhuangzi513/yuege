#ifndef ORIGIN_DB_HELPER_H
#define ORIGIN_DB_HELPER_H

#include "sqlite3.h"
#include "XLSReader.h"

#include <string>
#include <list>

/*
 * A OriginDatabase is used to save the details for a Stock.
 * It contains many tables, each of them is used to save the
 * deal-details for the Stock on the specified Date.
 * Take the Stock, whose ID is 600200, For example:
 * The name of the OriginDatabase for the Stock is "600200".
 * The Databse contains many tables, such as:
 *     1) table 20131205: the deal-detail for the stock(600200) on
 * 2013-12-05.
 *     2) table 20131206: the deal-detail for the stock(600200) on
 * 2013-12-06.
 *     ...
**/

class OriginDBHelper {
  public:
    OriginDBHelper();
    ~OriginDBHelper();

    bool createOriginDBFromFile(const std::string& name);
    sqlite3* getOriginDBForDate(const std::string& date);
    bool applyFilter(const std::string& filterComment, const std::string& tableName);

  private:
    bool getSpecsFromFileName(const std::string& fileName,
                              std::string& date, std::string& stockID);
    bool insertElement(const XLSReader::XLSElement* xlsElement);
    bool initOriginDBWithDetailInfo(std::list<XLSReader::XLSElement*>& detailInfoList);

  private:
    std::string mDBName;
    std::string mTableName;
    //std::Map<std::string, std::string> mOriginDBMaps
};
#endif

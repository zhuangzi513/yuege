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

    bool travelDir(const std::string& dirName, const std::string& fileName="");
    bool createOriginDBForDir(const std::string& dirName);
    bool updateOriginDBForStock(const std::string& fullPathofDetails, const std::string& fullPathofOriginDB);
    sqlite3* getOriginDBForDate(const std::string& date);
    bool applyFilter(const std::string& filterComment, const std::string& tableName);

  private:
    bool createOriginTableFromFile(const std::string& name, const std::string& originDBName="");
    bool addMoreTableToOriginDB(const std::string& aDBName, const std::list<std::string> tableNames);
    bool getSpecsFromFileName(const std::string& fileName,
                              std::string& date, std::string& stockID);
    bool insertElement(const XLSReader::XLSElement* xlsElement);
    bool initOriginDBWithDetailInfo(std::list<XLSReader::XLSElement*>& detailInfoList);

  private:
    static std::string mDetailPrefix;
    std::string mPreDBName;
    std::string mCurDBName;
    std::string mTableName;
    std::list<std::string> mOriginFiles;
};
#endif

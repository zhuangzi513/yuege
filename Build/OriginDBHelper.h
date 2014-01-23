#ifndef ORIGIN_DB_HELPER_H
#define ORIGIN_DB_HELPER_H

#include "sqlite3.h"
#include "XLSReader.h"

#include <string>
#include <list>

class OriginDBHelper {
  public:
    OriginDBHelper();
    ~OriginDBHelper();

    bool createOriginDBFromFile(const std::string& name);
    sqlite3* getOriginDBForDate(const std::string& date);
    bool applyFilter(const std::string& filterComment);

  private:
    bool getSpecsFromFileName(const std::string& fileName,
                              std::string& date, std::string& stockID);
    bool insertElement(const XLSReader::XLSElement* xlsElement);
    bool FillDBWithDetailInfo(std::list<XLSReader::XLSElement*>& detailInfoList);

  private:
    std::string mDBName;
    std::string mTableName;
    //std::Map<std::string, std::string> mOriginDBMaps
};
#endif

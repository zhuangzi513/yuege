#include "DBOperations.h"
#include "OriginDBHelper.h"

#include "TextXLSReader.h"
#include "DBWrapper.h"

#include <list>

#define LOGTAG   "OriginDBHelper"
#define SUFFIX_OF_DB_TYPE ".db"
#define PREFIX_OF_TABLE   "Table"

OriginDBHelper::OriginDBHelper() {
}

OriginDBHelper::~OriginDBHelper() {
}


bool OriginDBHelper::createOriginDBFromFile(const std::string& fileName) {
    std::string date;
    std::string stockID;
    // Need to figure out how to generate the DBName and TableName from
    // DBName = StockID && TableName = date ???

    if (!getSpecsFromFileName(fileName, date, stockID)) {
        return false;
    }

    std::list<XLSReader::XLSElement*> detailInfoList;
    if (!TextXLSReader::getElementsFrom(fileName, detailInfoList)) {
        printf("Fail to parse :%s\n", fileName.c_str());
        return false;
    }

    if (!initOriginDBWithDetailInfo(detailInfoList)) {
        printf("Fail to Fill Database:%s with the file:%s\n", mDBName.c_str(), fileName.c_str());
        return false;
    }

    return true;
}

sqlite3* OriginDBHelper::getOriginDBForDate(const std::string& date) {
    return NULL;
}

bool OriginDBHelper::applyFilter(const std::string& filterComment, const std::string& tableName) {
    return true;
}

bool OriginDBHelper::getSpecsFromFileName(const std::string& fileName,
                                          std::string& date, std::string& stockID) {
    // Retrieve the info from fileName
    // Asume that: the format of fileName is "6000001/2013/01/01.xls"

    int32_t start = fileName.find_first_of('/') + 1;
    int32_t end = fileName.find_first_of('.');
    stockID = fileName.substr(0, fileName.find_first_of('/'));
    date    = fileName.substr(start, end - start);
    char c = '0';
    for (int32_t i = 0; i < date.length(); i++ ) {
         if (date[i] == '/')
             date[i] = '0';
    }

    mDBName    = stockID + SUFFIX_OF_DB_TYPE;
    mTableName = PREFIX_OF_TABLE + date;
    return true;
}

bool OriginDBHelper::insertElement(const XLSReader::XLSElement* detailInfo) {
    std::string insertDescription;
    //TODO: generate the insertDescription for OriginDatabase

    std::string members("");
    std::string values("");

    members += "(";
    members += TABLE_FORMAT_ORIGIN;
    members += ")";

    values += "(";
    values += detailInfo->mTime;
    values += ",";
    values += detailInfo->mPrice;
    values += ",";
    values += detailInfo->mFloat;
    values += ",";
    values += detailInfo->mVolume;
    values += ",";
    values += detailInfo->mTurnOver;
    values += ",";
    values += detailInfo->mSB;
    values += ")";

    insertDescription = std::string(members) + std::string(values);
    return DBWrapper::insertElement(mDBName, mTableName, insertDescription, NULL);
}

bool OriginDBHelper::initOriginDBWithDetailInfo(std::list<XLSReader::XLSElement*>& detailInfoList) {
    // FIXME:Take care of the instance being released by other
    // thread. We may add a lock in the DBWrapper to handle the
    if (!DBWrapper::openTable(DBWrapper::ORIGIN_TABLE, mDBName, mTableName)) {
        return false;
    }

    std::string singleDes;
    std::list<XLSReader::XLSElement*>::iterator iterOfXLSElement;
    std::list<std::string> descriptions;

    // First of all, define the format of values to insert
    // (Time, Price, Float, Volume, TurnOver, SaleBuy)
    // VALUES (?, ?, ?, ?, ?, ?) or
    // VALUES (:Time, :Price, :Float, :Volume, :TurnOver, :SaleBuy)
    singleDes = TABLE_FORMAT_ORIGIN;
    singleDes += D_STMT_FORMAT_ORGIN;
    descriptions.push_back(singleDes);

    // And then, push the make the values into descriptions to be stepped.
    // FIXME: maybe we should define all the members of XLSReader::XLSElement
    // of std::string
    for (iterOfXLSElement = detailInfoList.begin(); iterOfXLSElement != detailInfoList.end(); iterOfXLSElement++) {
         std::string values("");
         values += "(";
         values += (*iterOfXLSElement)->mTime;
         values += ", ";
         values += (*iterOfXLSElement)->mPrice;
         values += ", ";
         values += (*iterOfXLSElement)->mFloat;
         values += ", ";
         values += (*iterOfXLSElement)->mVolume;
         values += ", ";
         values += (*iterOfXLSElement)->mTurnOver;
         values += ", ";
         values += (*iterOfXLSElement)->mSB;
         values += ")";
         descriptions.push_back(values);
    }
    DBWrapper::insertElementsInBatch(mDBName, mTableName, descriptions, detailInfoList, NULL);

    DBWrapper::closeDB(mDBName);

    return true;
}

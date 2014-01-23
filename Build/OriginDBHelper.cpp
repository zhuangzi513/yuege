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

bool OriginDBHelper::applyFilter(const std::string& filterComment) {
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
    std::string time("Time");
    std::string price("Price");
    std::string floatV("Float");
    std::string volume("Volume");
    std::string turnover("TurnOver");
    std::string saleBuy("SaleBuy");

    char members[100] = {0};
    char values[150]  = {0};
    sprintf(members, "(%s, %s, %s, %s, %s, %s) ", time.c_str(), price.c_str(), floatV.c_str(), volume.c_str(), turnover.c_str(), saleBuy.c_str());
    sprintf(values, "VALUES (%d, %f, %f, %d, %f, %d) ", ((int)(detailInfo->mTime)), ((double)(detailInfo->mPrice)), ((double)(detailInfo->mFloat)), ((int)(detailInfo->mVolume)), ((double)(detailInfo->mTurnOver)), ((int)(detailInfo->mSB)));

    insertDescription = std::string(members) + std::string(values);
    return DBWrapper::insertElement(mDBName, mTableName, insertDescription, NULL);
}

bool OriginDBHelper::initOriginDBWithDetailInfo(std::list<XLSReader::XLSElement*>& detailInfoList) {
    // FIXME:Take care of the instance being released by other
    // thread. We may add a lock in the DBWrapper to handle the
    if (!DBWrapper::openTable(DBWrapper::ORIGIN_TABLE, mDBName, mTableName)) {
        return false;
    }

    int32_t i = 0;
    std::list<XLSReader::XLSElement*>::iterator iterOfXLSElement;
    for (iterOfXLSElement = detailInfoList.end(); iterOfXLSElement != detailInfoList.begin(); iterOfXLSElement--) {
         //FIXME: Maybe we should check whether the Inserting is success or not
         printf("1count:%d\n", ++i);
         if (!insertElement(*iterOfXLSElement)) {
             printf("2count:%d\n", i);
             return false;
         }
    }

    DBWrapper::closeDB(mDBName);

    return true;
}

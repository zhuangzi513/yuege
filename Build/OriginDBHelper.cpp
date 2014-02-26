#include "ErrorDefines.h"
#include "DBOperations.h"
#include "OriginDBHelper.h"

#include "TextXLSReader.h"
#include "DBWrapper.h"

#include <list>
#include <algorithm> 
//LINUX
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
//====

#define LOGTAG   "OriginDBHelper"
#define SUFFIX_OF_DB_TYPE ".db"
#define PREFIX_OF_TABLE   "Table"

//FIXME: not here
std::string OriginDBHelper::mDetailPrefix = "details";

OriginDBHelper::OriginDBHelper() {
}

OriginDBHelper::~OriginDBHelper() {
}

bool OriginDBHelper::travelDir(const std::string& dirName, const std::string& fileName) {
    std::string fullName = dirName + fileName;
    struct dirent* pDirent = NULL;
    struct stat s;
    DIR* pDir = NULL;
    std::string childName;

    LOGI(LOGTAG, "opendir for:%s", fullName.c_str());
    pDir = opendir(fullName.c_str());
    if (pDir == NULL) {
        LOGI(LOGTAG, "Fail to opendir:%s %s", dirName.c_str(), fileName.c_str());
        return false;
    }

    while (true) {
        errno = 0;
        pDirent = readdir(pDir);
        if (pDirent != NULL) {
            if ((strcmp((const char*)pDirent->d_name, ".") == 0) ||
                (strcmp((const char*)pDirent->d_name, "..") == 0) ||
                (strcmp((const char*)pDirent->d_name, ".git") == 0)) {
                errno = 0;
                continue;
            }

            childName = fullName + "/" + pDirent->d_name;
            if (lstat(childName.c_str(), &s) == -1) {
                LOGI(LOGTAG, "lstat errno:%d", errno);
                errno = 0;
                continue;
            }

            if (S_ISDIR(s.st_mode)) {
                travelDir(childName.c_str());
            } else if (S_ISREG(s.st_mode)) {
                mOriginFiles.push_back(childName);
            } else {
                LOGI(LOGTAG, "Error OTHER");
                break;
            }
        } else {
            LOGI(LOGTAG, "Error Fail to readdir from:%s", fullName.c_str());
            break;
        }
    }
    closedir(pDir);
    return true;
}

bool OriginDBHelper::createOriginDBForDir(const std::string& dirName) {
    std::list<std::string>::iterator singleOriginFileName;
    size_t i = 0;

    travelDir(dirName);
    mOriginFiles.sort();

    LOGI(LOGTAG, "Size of files to open:%d", mOriginFiles.size());
    for (singleOriginFileName = mOriginFiles.begin(), i = 0; i < mOriginFiles.size(); singleOriginFileName++, i++) {
         LOGI(LOGTAG, "create OriginDB from file:%s\n", (*singleOriginFileName).c_str());
         if (!createOriginTableFromFile(*singleOriginFileName)) {
             perror("Reason:");
             LOGI(LOGTAG, "Fail to open %dth file", i);
             return false;
         }
    }

    return true;
}

bool OriginDBHelper::createOriginTableFromFile(const std::string& fileName) {
    std::string dbName;
    std::string tableName;
    // Need to figure out how to generate the DBName and TableName from
    // DBName = StockID && TableName = date ???

    if (!getSpecsFromFileName(fileName, tableName, dbName)) {
        return false;
    }

    if (dbName != mCurDBName) {
        if (mCurDBName.length() > 0) {
            //FIXME: the init value of mCurDBName is ""
            LOGI(LOGTAG, "close DB:%s", mCurDBName.c_str());
            DBWrapper::closeDB(mCurDBName);
        }
        mCurDBName = dbName;
    }
    mTableName = tableName;

    LOGI(LOGTAG, "fileName:%s", fileName.c_str());
    std::list<XLSReader::XLSElement*> detailInfoList;
    if (!TextXLSReader::getElementsFrom(fileName, detailInfoList)) {
        printf("Fail to parse :%s\n", fileName.c_str());
        return false;
    }

    if (!initOriginDBWithDetailInfo(detailInfoList)) {
        printf("Fail to Fill Database:%s with the file:%s, errno:%d\n", mCurDBName.c_str(), fileName.c_str(), errno);
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
                                          std::string& tableName, std::string& dbName) {
    // Retrieve the info from fileName
    // Asume that: the format of fileName is "DETAILS/6000001/2013/01/01.xls"

    std::string fileNoPrefix = fileName.substr(mDetailPrefix.length() + 1);

    int32_t start = fileNoPrefix.find_first_of('/') + 1;
    int32_t end   = fileNoPrefix.find_first_of('.');
    dbName    = fileNoPrefix.substr(0, fileNoPrefix.find_first_of('/'));
    tableName = fileNoPrefix.substr(start, end - start);
    char c = '0';
    for (int32_t i = 0; i < tableName.length(); i++ ) {
         if (tableName[i] == '/')
             tableName[i] = '0';
    }

    dbName = dbName + SUFFIX_OF_DB_TYPE;
    tableName = PREFIX_OF_TABLE + tableName;
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
    return DBWrapper::insertElement(mCurDBName, mTableName, insertDescription, NULL);
}

bool OriginDBHelper::initOriginDBWithDetailInfo(std::list<XLSReader::XLSElement*>& detailInfoList) {
    // FIXME:Take care of the instance being released by other
    // thread. We may add a lock in the DBWrapper to handle the
    LOGI(LOGTAG, "openTable, mCurDBName:%s, mTableName:%s", mCurDBName.c_str(), mTableName.c_str());
    if (!DBWrapper::openTable(DBWrapper::ORIGIN_TABLE, mCurDBName, mTableName)) {
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
    DBWrapper::insertElementsInBatch(mCurDBName, mTableName, descriptions, detailInfoList, NULL);

    return true;
}

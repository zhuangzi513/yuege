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
#define PREFIX_OF_TABLE   "O"

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

    LOGD(LOGTAG, "opendir for:%s", fullName.c_str());
    pDir = opendir(fullName.c_str());
    if (pDir == NULL) {
        LOGD(LOGTAG, "Fail to opendir:%s %s", dirName.c_str(), fileName.c_str());
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
                LOGD(LOGTAG, "lstat errno:%d", errno);
                errno = 0;
                continue;
            }

            if (S_ISDIR(s.st_mode)) {
                travelDir(childName.c_str());
            } else if (S_ISREG(s.st_mode)) {
                mOriginFiles.push_back(childName);
            } else {
                LOGD(LOGTAG, "Error OTHER");
                break;
            }
        } else {
            LOGD(LOGTAG, "Error Fail to readdir from:%s, errno:%d", fullName.c_str(), errno);
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

    LOGD(LOGTAG, "Size of files to open:%d", mOriginFiles.size());
    for (singleOriginFileName = mOriginFiles.begin(), i = 0; i < mOriginFiles.size(); singleOriginFileName++, i++) {
         LOGD(LOGTAG, "create OriginDB from file:%s\n", (*singleOriginFileName).c_str());
         if (!createOriginTableFromFile(*singleOriginFileName)) {
             perror("Reason:");
             LOGD(LOGTAG, "Fail to open %dth file", i);
             return false;
         }
    }

    return true;
}

static std::string getFileNO(std::string& originFile) {
    //PATH/StockID/2014/03/02.xls
    //length of "2014/03/02" is 10
    std::string fileNO;
    int dateEndPos, dateBeginPos;
    dateEndPos   = originFile.find_first_of('.');
    dateBeginPos = dateEndPos - 10;
    fileNO = originFile.substr(dateBeginPos, 10);
    fileNO = fileNO.substr(0, 4) + fileNO.substr(5, 2) + fileNO.substr(8, 2);

    return fileNO;
}

static bool getFilesNeedToUpdate(std::list<std::string>& existingTables,
                                 std::list<std::string>& originFiles,
                                 std::list<std::string>& outFilesNeedToUpdate) {
    std::list<std::string>::iterator itrTable  = existingTables.begin();
    std::list<std::string>::iterator itrDetail = originFiles.begin();
    std::string tableNO, fileNO;
    //FIXME: assume that there are some tables in the OriginDB already
    while (itrTable != existingTables.end()
        && itrDetail != originFiles.end()) {
        if ((*itrTable) == "FilterResult"
            || (*itrTable) == "FilterResult20W"
            || (*itrTable) == "BankerResultTable"
            || ((*itrTable) == "MiddleWareTable")) {
            itrTable++;
            continue;
        }

        tableNO = (*itrTable).substr(1);
        fileNO = getFileNO(*itrDetail); 

        if (tableNO == fileNO) {
            //The originFile has been added
            itrTable++;
            itrDetail++;
            continue;    
        } else if (tableNO != fileNO) {
            //The originFile has not been added
            //AddTheFileToDB
            outFilesNeedToUpdate.push_back(*itrDetail);
            itrDetail++;
        }
        /* else {
            //For the sake of removing empty origin files
            LOGI(LOGTAG, "tableNO: %s, fileNO:%s", tableNO.c_str(), fileNO.c_str());
            itrTable++;
        }
        */
    }

    // the remaining origin files to update
    while (itrDetail != originFiles.end()) {
        fileNO = getFileNO(*itrDetail);
        if (tableNO < fileNO) {
            LOGI(LOGTAG, "File need to update: tableNO: %s, fileNO:%s, originFile:%s", tableNO.c_str(), fileNO.c_str(), (*itrDetail).c_str());
            outFilesNeedToUpdate.push_back(*itrDetail);
        }
        itrDetail++;
    }


    return true;
}

bool OriginDBHelper::updateOriginDBForStock(const std::string& fullPathofDetails, const std::string& fullPathofOriginDB) {
    std::list<std::string> existingTables;
    if (!DBWrapper::getAllTablesOfDB(fullPathofOriginDB, existingTables)) {
        LOGI(LOGTAG, "Fail to get tables from originDB: %s", fullPathofOriginDB.c_str());
        DBWrapper::closeDB(fullPathofOriginDB);
        return false;
    }
    existingTables.sort();

    if (!travelDir(fullPathofDetails)) {
        LOGI(LOGTAG, "Fail to get detail files from dir: %s", fullPathofDetails.c_str());
        return false;
    }

    std::list<std::string> newDetailFiles;
    mOriginFiles.sort();

    if (!getFilesNeedToUpdate(existingTables, mOriginFiles, newDetailFiles)) {
        LOGE(LOGTAG, "Fail to get files need to update for DB dir: %s", fullPathofOriginDB.c_str());
        return false;
    }

//#if DEBUG
    std::list<std::string>::iterator itrDebug = newDetailFiles.begin();
    while(itrDebug != newDetailFiles.end()) {
        if (!createOriginTableFromFile(*itrDebug, fullPathofOriginDB)) {
            LOGE(LOGTAG, "Cannot create OriginTbale for Origin file:%s", (*itrDebug).c_str());
            break;
        }
        itrDebug++;
    }
//#endif

    mOriginFiles.clear();
    DBWrapper::closeDB(fullPathofOriginDB);
    return true;
}

template <typename T>
inline void releaseList(std::list<T*>& list) {
    typename std::list<T*>::iterator itr;
    for (itr = list.begin(); itr != list.end(); ++itr) {
         delete *itr;
    }

    list.clear();
}

bool OriginDBHelper::createOriginTableFromFile(const std::string& fileName, const std::string& originDBName) {
    std::string dbName = originDBName;
    std::string tableName;
    // Need to figure out how to generate the DBName and TableName from
    // DBName = StockID && TableName = date ???

    if (!getSpecsFromFileName(fileName, tableName, dbName)) {
        return false;
    }

    if (dbName != mCurDBName) {
        if (mCurDBName.length() > 0) {
            //FIXME: the init value of mCurDBName is ""
            LOGD(LOGTAG, "close DB:%s", mCurDBName.c_str());
            DBWrapper::closeDB(mCurDBName);
        }
        mCurDBName = dbName;
    }
    mTableName = tableName;

    LOGD(LOGTAG, "fileName:%s", fileName.c_str());
    LOGD(LOGTAG, "mCurDBName:%s", mCurDBName.c_str());
    LOGD(LOGTAG, "mTableName:%s", mTableName.c_str());
    std::list<XLSReader::XLSElement*> detailInfoList;
    std::list<XLSReader::XLSElement*>::iterator itr;
    if (!TextXLSReader::getElementsFrom(fileName, detailInfoList)) {
        LOGI(LOGTAG, "Fail to parse :%s\n", fileName.c_str());
        return false;
    }

    if (!initOriginDBWithDetailInfo(detailInfoList)) {
        LOGI(LOGTAG, "Fail to Fill Database:%s with the file:%s, errno:%d\n", mCurDBName.c_str(), fileName.c_str(), errno);
        releaseList(detailInfoList);
        return false;
    }

    releaseList(detailInfoList);
/*
    for (itr = detailInfoList.begin(); itr != detailInfoList.end(); itr++) {
        delete *itr;
        //detailInfoList.pop_front();
    }
    detailInfoList.clear();
*/

    return true;
}

bool OriginDBHelper::addMoreTableToOriginDB(const std::string& aDBName, const std::list<std::string> tableNames) {
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
    tableName = fileNoPrefix.substr(start, end - start);
    std::string year("");
    std::string month("");
    std::string day("");

    // XXX: Year/Month/Day
    year = tableName.substr(0, tableName.find_first_of('/'));
    month = tableName.substr(tableName.find_first_of('/') + 1, tableName.find_last_of('/') - tableName.find_first_of('/') - 1);
    day = tableName.substr(tableName.find_last_of('/') + 1);

    // XXX: table name
    if (month.size() < 2) {
        month = "0" + month;
    }

    if (day.size() < 2) {
        day = "0" + day;
    }

    tableName = PREFIX_OF_TABLE + year + month + day;
    LOGD(LOGTAG, "tableName:%s", tableName.c_str());

    // XXX: database name
    if (dbName.size() == 0) {
        dbName    = fileNoPrefix.substr(0, fileNoPrefix.find_first_of('/'));
        dbName = dbName + SUFFIX_OF_DB_TYPE;
    }
    LOGD(LOGTAG, "dbName:%s", dbName.c_str());
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
    LOGD(LOGTAG, "openTable, mCurDBName:%s, mTableName:%s", mCurDBName.c_str(), mTableName.c_str());
    const std::string curDBName(mCurDBName);
    const std::string curTableName(mTableName);
    // If there is already an OrginTable with the same to currTableName, Abort!
    if (DBWrapper::SUCC_OPEN_TABLE != DBWrapper::openTable(DBWrapper::ORIGIN_TABLE, curDBName, curTableName)) {
        LOGD(LOGTAG, "Fail to open table:%s or it has been there already", curTableName.c_str());
        return true;
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

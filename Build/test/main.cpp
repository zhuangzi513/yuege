
#include "Forecaster.h"
#include "PriceDiscover.h"
#include "TurnOverDiscover.h"
#include "OriginDBHelper.h"
#include "DBFilter.h"
#include "DBWrapper.h"
#include "sqlite3.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include <errno.h>

#define EXAMPLE_XLS_NAME "600001/2013/0101.xls"
#define DAY_OF_TARGET    "O20141107"
#define NUM_THREAD 20

int searchCallback(void *arg1, int arg2, char **arg3, char **arg4) {
    static int count = 0;
    printf("searchCallback:%p\n", arg1);
    printf("searchCallback:%d\n", arg2);
    printf("searchCallback:%p\n", arg3);
    printf("searchCallback:%p\n", *arg3);
    printf("searchCallback:%s\n", *arg3);
    printf("searchCallback:%s\n", *(arg3 + 1));
    printf("searchCallback:%s\n", *(arg3 + 2));
    printf("searchCallback:%s\n", *(arg3 + 3));
    printf("searchCallback:%p\n", arg4);
    printf("searchCallback:%p\n", *arg4);
    printf("searchCallback:%s\n", *arg4);
    printf("searchCallback:%s\n", *(arg4 + 1));
    printf("searchCallback:%s\n", *(arg4 + 2));
    printf("searchCallback:%s\n", *(arg4 + 3));
    printf("==============count:%d\n", count++);
    return 0;
}

//bool createOriginDB(const std::string& fileName)
//{
//    OriginDBHelper* originDBHelper = new OriginDBHelper();
//    if (originDBHelper) {
//        originDBHelper->createOriginTableFromFile(fileName);
//    }
//    return true;
//}

void getAllDatabase(std::list<std::string>& fileNames, const std::string targetDir) {
    std::string currentDir(targetDir);
    std::string fullName;
    struct dirent* pDirent = NULL;
    struct stat s;
    DIR* pDir = NULL;
    std::string childName;

    printf( "opendir for:%s\n", currentDir.c_str());
    pDir = opendir(currentDir.c_str());
    if (pDir == NULL) {
        perror("Fail to open dir"); 
        return;
    }

    while (true) {
        fullName = currentDir;
        pDirent = readdir(pDir);
        if (pDirent != NULL) {
            if ((strcmp((const char*)pDirent->d_name, ".") == 0) ||
                (strcmp((const char*)pDirent->d_name, ".git") == 0) ||
                (strcmp((const char*)pDirent->d_name, "..") == 0) ){
                continue;
            }

            childName = pDirent->d_name;

            fullName += "/";
            fullName += childName;
            
            if (lstat(fullName.c_str(), &s) == -1) {
                continue;
            }

            if (S_ISREG(s.st_mode)) {
                if (childName.find_first_of(".db") == 6) {
                    //printf( "find file named:%s, first '.db':%d \n", childName.c_str(), childName.find_first_of(".db"));
                    fileNames.push_back(fullName);
                }
            } else if (S_ISDIR(s.st_mode)) {
                    fileNames.push_back(fullName);
            } else if (S_ISLNK(s.st_mode)) {
                printf( "find file named:%s, LINK\n", childName.c_str());
            } else if (S_ISBLK(s.st_mode)) {
                printf( "find file named:%s, BLK\n", childName.c_str());
            } else if (S_ISFIFO(s.st_mode)) {
                printf( "find file named:%s, FIFO\n", childName.c_str());
            } else if (S_ISSOCK(s.st_mode)) {
                printf( "find file named:%s, SOCKET\n", childName.c_str());
            } else {
                printf( "find file named:%s, unkown\n", childName.c_str());
                continue;
            }
        } else {
            printf( "Error Fail to readdir from:%s\n", currentDir.c_str());
            perror("Reason is:");
            break;
        }
    }
    closedir(pDir);

    
    return;
}

bool filterOriginDB() {
    std::list<std::string> fileNames;
    getAllDatabase(fileNames, "dbs");
    Forecaster* pForecaster = new Forecaster();
    std::list<std::string>::iterator itrOfDBNames = fileNames.begin();
    for (int i = 0; i < fileNames.size(); i++) {
      DBFilter* filter = new DBFilter(*itrOfDBNames);
      //XXX: Make sure the right order here
      filter->removeTableFromOriginDB("FilterResult100W");
      filter->removeTableFromOriginDB("FilterResult10W");
      filter->removeTableFromOriginDB("FilterResult30W");
      filter->removeTableFromOriginDB("FilterResult40W");
      filter->removeTableFromOriginDB("FilterResult50W");
      filter->removeTableFromOriginDB("FilterResult60W");
      filter->removeTableFromOriginDB("FilterResult70W");
      filter->removeTableFromOriginDB("FilterResult80W");
      filter->removeTableFromOriginDB("FilterResult90W");
      filter->removeTableFromOriginDB("T50000");
      filter->removeTableFromOriginDB("F50000");
      //filter->filterOriginDBByTurnOver();
      itrOfDBNames++;
      delete filter;
    }
    return true;
}

bool getTP() {
    std::list<std::string> fileNames;
    getAllDatabase(fileNames, "dbs");
    std::list<std::string> listOfDBs;
    std::list<std::string>::iterator itrOfDBNames = fileNames.begin();
    int countStep1 = 0, countStep2 = 0;
    for (int i = 0; i < fileNames.size(); i++) {
      PriceDiscover* priceDiscover = new PriceDiscover(*itrOfDBNames, "FilterResult20W");
      int ret = priceDiscover->isDBBuyMoreThanSale("FilterResult20W", 30, 10, 1.8);
      if (ret >= 0) {
          ++countStep1;
          printf("Step 1: SideWay DB:%s,\n", (*itrOfDBNames).c_str());
          if (ret > 0) {
              printf("Step 2: SideWay DB:%s,\n", (*itrOfDBNames).c_str());
              ++countStep2;
          }
          //listOfDBs.push_back(*itrOfDBNames);
      }
      delete priceDiscover;
      itrOfDBNames++;
    }
    printf("\n\n\ncountStep1:%d, countStep2:%d\n\n\n", countStep1, countStep2);
}

bool getSideWaysStock() {
    std::list<std::string> fileNames;
    getAllDatabase(fileNames, "dbs");
    std::list<std::string> listOfDBs;
    std::list<std::string>::iterator itrOfDBNames = fileNames.begin();
    for (int i = 0; i < fileNames.size(); i++) {
      PriceDiscover* priceDiscover = new PriceDiscover(*itrOfDBNames, "FilterResult20W");
      if (priceDiscover->isInPhaseTwo("FilterResult20W", 2)) {
          printf("SideWay DB:%s\n", (*itrOfDBNames).c_str());
          listOfDBs.push_back(*itrOfDBNames);
      }
      delete priceDiscover;
      itrOfDBNames++;
    }
}

struct PositiveDB {
  std::string Name;
  double BankerBuyingTurnOver;
  double BankerSalingTurnOver;
  double RatioBS;
};

bool LargeToSmall(const PositiveDB& db1, const PositiveDB& db2) {
    return (db1.RatioBS >= db2.RatioBS);
}

std::list<PositiveDB> sPositiveDBs;

void getBankerInChargingDB() {

    float countOfBanker = 0;
    float countOfNeutralBanker = 0;
    float countOfNagtiveBanker = 0;
    float countOfPositiveBanker = 0;

    std::list<std::string> fileNames;
    getAllDatabase(fileNames, "dbs");
    fileNames.sort();
    std::list<std::string> listOfDBs;
    std::list<std::string>::iterator itrOfDBNames = fileNames.begin();
    TurnOverDiscover* pTurnOverDiscover = NULL;
    for (int i = 0; i < fileNames.size(); i++) {
      pTurnOverDiscover = new TurnOverDiscover(*itrOfDBNames, "FilterResult30W");
      if (pTurnOverDiscover->isTodayBankerInCharge(DAY_OF_TARGET)) {
          if (pTurnOverDiscover->isTodayNeutralBankerInCharge(DAY_OF_TARGET)) {
              //printf("Is Positive:%s\n", itrOfDBNames->c_str());
              countOfNeutralBanker += 1;
          }
          if (pTurnOverDiscover->isTodayNagtiveBankerInCharge(DAY_OF_TARGET)) {
              //printf("Is Nagtive:%s\n", itrOfDBNames->c_str());
              countOfNagtiveBanker += 1;
          }
          if (pTurnOverDiscover->isTodayPositiveBankerInCharge(DAY_OF_TARGET)) {
              //printf("Is Positive:%s\n", itrOfDBNames->c_str());
              std::vector<double> bankerTurnOvers;
              pTurnOverDiscover->getBankerTurnOvers(bankerTurnOvers);
              //printf("BankerTurnOvers, Buy:%lf, Sale:%lf\n", bankerTurnOvers[0], bankerTurnOvers[1]);
              PositiveDB tmpPositiveDB;
              tmpPositiveDB.Name = *itrOfDBNames;
              tmpPositiveDB.BankerBuyingTurnOver = bankerTurnOvers[0];
              tmpPositiveDB.BankerSalingTurnOver = bankerTurnOvers[1];
              tmpPositiveDB.RatioBS = bankerTurnOvers[0]/bankerTurnOvers[1];
              sPositiveDBs.push_back(tmpPositiveDB);
              countOfPositiveBanker += 1;
          }
          countOfBanker += 1;
      }
      itrOfDBNames++;
      delete pTurnOverDiscover;
      pTurnOverDiscover = NULL;
    }

    printf("BankerInCharge size:%lf ratio:%lf\n", countOfBanker, countOfBanker / fileNames.size());
    printf("NeutralBanker  size:%lf ratio:%lf\n", countOfNeutralBanker ,countOfNeutralBanker / countOfBanker);
    printf("NagtiveBanker  size:%lf ratio:%lf\n", countOfNagtiveBanker, countOfNagtiveBanker / countOfBanker);
    printf("PositiveBanker size:%lf ratio:%lf\n", countOfPositiveBanker, countOfPositiveBanker / countOfBanker);

    sPositiveDBs.sort(LargeToSmall);
    std::list<PositiveDB>::iterator iterPositionDB = sPositiveDBs.begin();
    for (int i = 0; i < sPositiveDBs.size(); i++) {
        printf("PositiveBanker, name:%s, buying:%lf sale:%lf, ratio:%lf\n", iterPositionDB->Name.c_str(), iterPositionDB->BankerBuyingTurnOver, iterPositionDB->BankerSalingTurnOver, iterPositionDB->RatioBS);
        iterPositionDB++; 
    }
}

struct InputArg {
  int ItrLength;
  std::list<std::string>::iterator BeginDBItr;
  std::list<std::string>::iterator BeginFileItr;
};

//bool doUpdateFilterResults(std::list<std::string>::iterator& startItr, int len) {
void* doUpdateFilterResults(void *args) {
    struct InputArg* pInputArg = (struct InputArg*)args;
    int itrLength = pInputArg->ItrLength;
    std::list<std::string>::iterator beginItr = pInputArg->BeginDBItr;
    DBFilter* dbFilter = NULL;

    pthread_t self = pthread_self();

    for (int i = 0; i < itrLength; i++) {
        dbFilter = new DBFilter(*beginItr);
        dbFilter->clearTableFromOriginDB(DBFilter::mResultTableName);
        printf("doUpdateFilterResults self beginItr:%s\n", beginItr->c_str());
        bool result = dbFilter->updateFilterResultByTurnOver("FilterResult20W", 200000, 1000000);
        if (!result) {
            printf("CRITICAL ERR: doUpdateFilterResults self beginItr:%s\n", beginItr->c_str());
            //exit(1);
        }
/*
        dbFilter->updateFilterResultByTurnOver("FilterResult10W", 100000, 1000000);
        dbFilter->updateFilterResultByTurnOver("FilterResult30W", 300000, 1000000);
        dbFilter->updateFilterResultByTurnOver("FilterResult40W", 400000, 1000000);
        dbFilter->updateFilterResultByTurnOver("FilterResult50W", 500000, 1000000);
        dbFilter->updateFilterResultByTurnOver("FilterResult60W", 600000, 1000000);
        dbFilter->updateFilterResultByTurnOver("FilterResult70W", 700000, 1000000);
        dbFilter->updateFilterResultByTurnOver("FilterResult80W", 800000, 1000000);
        dbFilter->updateFilterResultByTurnOver("FilterResult90W", 900000, 1000000);
        dbFilter->updateFilterResultByTurnOver("FilterResult100W", 1000000, 1000000);
*/
        beginItr++;
        delete dbFilter;
    }
    delete pInputArg;
}

bool updateFilterResults(const std::string& dirName, int threads) {

    std::list<std::string> originDBNames;
    std::list<std::string>::iterator itrDB;
    getAllDatabase(originDBNames, "dbs");
    originDBNames.sort();
    itrDB     = originDBNames.begin();

/*
    for (itrDB = originDBNames.begin(); itrDB != originDBNames.end(); itrDB++) {
        //pForecaster->forecasteFromFirstPositiveFlowin(fileNames[i]);
        printf("fileNames:%s\n", (*itrDB).c_str());
    }
*/


    DBFilter* dbFilter = NULL;
    while (itrDB != originDBNames.end()) {
        dbFilter = new DBFilter(*itrDB);
        dbFilter->clearTableFromOriginDB(DBFilter::mResultTableName);
        dbFilter->updateFilterResultByTurnOver("FilterResult20W", 200000, 1000000);
        //dbFilter->updateFilterResultByTurnOver("FilterResult10W", 100000, 1000000);
        //dbFilter->updateFilterResultByTurnOver("FilterResult20W", 200000, 1000000);
        //dbFilter->updateFilterResultByTurnOver("FilterResult30W", 300000, 1000000);
        //dbFilter->updateFilterResultByTurnOver("FilterResult40W", 400000, 1000000);
        //dbFilter->updateFilterResultByTurnOver("FilterResult50W", 500000, 1000000);
        //dbFilter->updateFilterResultByTurnOver("FilterResult60W", 600000, 1000000);
        //dbFilter->updateFilterResultByTurnOver("FilterResult70W", 700000, 1000000);
        //dbFilter->updateFilterResultByTurnOver("FilterResult80W", 800000, 1000000);
        //dbFilter->updateFilterResultByTurnOver("FilterResult90W", 900000, 1000000);
        //dbFilter->updateFilterResultByTurnOver("FilterResult100W", 1000000, 1000000);
        itrDB++;
        delete dbFilter;
    }

/*
    InputArg* dispatchArg = NULL;
    pthread_t activeThreads[NUM_THREAD];
    int DBCountForEachThread = 4000/threads;
    int myStart = 0;
    int myLen = 0;
    for (int i = 0; i < threads; i++) {
        myLen = 0;
        dispatchArg = new InputArg;
        dispatchArg->BeginDBItr = itrDB;
        printf("create thread from :%s\n", itrDB->c_str());
        //get the start itr for every thread
        if ((myStart + DBCountForEachThread) < originDBNames.size()) {
            for (; myStart < (i + 1) * DBCountForEachThread; myStart++) {
                 ++itrDB;
                 ++myLen;
            }
        } else if (myStart < originDBNames.size()) {
            for (int j = myStart; (itrDB != originDBNames.end() && j < originDBNames.size()); j++) {
                 ++itrDB;
                 ++myLen;
            }
        } else {
            break;
        }

        dispatchArg->ItrLength = myLen;
        pthread_t tmpThread;
        pthread_create(&tmpThread, NULL, doUpdateFilterResults, (void*)dispatchArg);
        activeThreads[i] = tmpThread;
    }

    for (int i = 0; i < threads; i++) {
         pthread_join(activeThreads[i], NULL);
    }
*/

    return true;
}

static void* doUpdateOriginDBForStock(void *args) {
    struct InputArg* pInputArg = (struct InputArg*)args;
    int itrLength = pInputArg->ItrLength;
    std::list<std::string>::iterator beginDBItr = pInputArg->BeginDBItr;
    std::list<std::string>::iterator beginFileItr = pInputArg->BeginFileItr;
    OriginDBHelper* originDBHelper = new OriginDBHelper();

    pthread_t self = pthread_self();
    std::string stockIDFromDB = (*beginDBItr).substr((*beginDBItr).find_first_of('/') + 1, 6);
    std::string stockIDFromDetail = (*beginFileItr).substr((*beginFileItr).find_first_of('/') + 1);
    int i = 0;

    while (stockIDFromDB == stockIDFromDetail
           && i < itrLength) {
        stockIDFromDB     = (*beginDBItr).substr((*beginDBItr).find_first_of('/') + 1, 6);
        stockIDFromDetail = (*beginFileItr).substr((*beginFileItr).find_first_of('/') + 1);
        bool result = originDBHelper->updateOriginDBForStock(*beginFileItr, *beginDBItr);
        if (!result) {
            printf("CRITICAL ERR: self :%d, Try beginDBItr:%s, beginFileItr:%s , len:%d\n", self, beginDBItr->c_str(), beginFileItr->c_str(), itrLength);
            exit(1);
        }
        ++beginDBItr;
        ++beginFileItr;
        ++i;
    }
    printf("end self\n");
    delete pInputArg;
    delete originDBHelper;
}


bool updateOriginDBs(const std::string& dirName, int threads) {
    OriginDBHelper* originDBHelper = new OriginDBHelper();
    DBFilter* dbFilter = NULL;

    std::list<std::string> detailNames;
    std::list<std::string>::iterator itrDetail;
    getAllDatabase(detailNames, "details");
    detailNames.sort();
//    for (itrDetail = detailNames.begin(); itrDetail != detailNames.end(); itrDetail++){
//        //pForecaster->forecasteFromFirstPositiveFlowin(fileNames[i]);
//        printf("fileNames:%s\n", (*itrDetail).c_str());
//    }

    std::list<std::string> originDBNames;
    std::list<std::string>::iterator itrDB;
    getAllDatabase(originDBNames, "dbs");
    originDBNames.sort();
//    for (itrDB = originDBNames.begin(); itrDB != originDBNames.end(); itrDB++) {
//        //pForecaster->forecasteFromFirstPositiveFlowin(fileNames[i]);
//        printf("fileNames:%s\n", (*itrDB).c_str());
//    }


    std::string stockIDFromDB, stockIDFromDetail;
    itrDetail = detailNames.begin();
    itrDB     = originDBNames.begin();
    while(itrDB != originDBNames.end()
          && itrDetail != detailNames.end()) {
        stockIDFromDB     = (*itrDB).substr((*itrDB).find_first_of('/') + 1, 6);
        stockIDFromDetail = (*itrDetail).substr((*itrDetail).find_first_of('/') + 1);
        printf("stockIDFromDB:%s, stockIDFromDetail:%s\n", stockIDFromDB.c_str(), stockIDFromDetail.c_str());
        if (stockIDFromDB != stockIDFromDetail) {
            printf("stockIDFromDB:%s, stockIDFromDetail:%s\n", stockIDFromDB.c_str(), stockIDFromDetail.c_str());
            exit(1);
            break;
        }

        originDBHelper->updateOriginDBForStock(*itrDetail, *itrDB);
        itrDB++;
        itrDetail++;
    }

/*
    itrDetail = detailNames.begin();
    itrDB     = originDBNames.begin();
    InputArg* dispatchArg = NULL;
    pthread_t activeThreads[NUM_THREAD];
    int DBCountForEachThread = originDBNames.size()/threads;
    int myStart = 0;
    int myLen = 0;
    for (int i = 0; i < threads; i++) {
        myLen = 0;
        dispatchArg = new InputArg;
        dispatchArg->BeginDBItr = itrDB;
        dispatchArg->BeginFileItr = itrDetail;
        printf("create thread from DB:%s, File:%s\n", itrDB->c_str(), itrDetail->c_str());
        //get the start itr for every thread
        if ((myStart + DBCountForEachThread) < originDBNames.size()) {
            for (; myStart < (i + 1) * DBCountForEachThread; myStart++) {
                 ++itrDB;
                 ++itrDetail;
                 ++myLen;
            }
        } else if (myStart < originDBNames.size()) {
            for (int j = myStart; (itrDB != originDBNames.end() && j < originDBNames.size()); j++) {
                 ++itrDB;
                 ++itrDetail;
                 ++myLen;
            }
        } else {
            break;
        }

        dispatchArg->ItrLength = myLen;
        pthread_t tmpThread;
        pthread_create(&tmpThread, NULL, doUpdateOriginDBForStock, (void*)dispatchArg);
        activeThreads[i] = tmpThread;
    }

    for (int i = 0; i < threads; i++) {
         pthread_join(activeThreads[i], NULL);
    }
*/

    return true;
}

int deleteRows(std::list<std::string>& targetRows) {
    std::string keyColumn = "Date";
    std::string tableName = "FilterResult";
    std::list<std::string> dbNames;
    std::list<std::string>::iterator itrDB;

    getAllDatabase(dbNames, "dbs");
    dbNames.sort();

    itrDB = dbNames.begin();
    while (itrDB != dbNames.end()) {
           DBWrapper::deleteRows(*itrDB, tableName, keyColumn, targetRows);
           itrDB++;
    }
}

int main() {
   std::string fileName(EXAMPLE_XLS_NAME);
   getBankerInChargingDB();
   //updateOriginDBs("details", NUM_THREAD);
   //updateFilterResults("dbs", NUM_THREAD);
   //getSideWaysStock();
   //filterOriginDB();
   //getTP();
/*
   std::list<std::string> rowsToDelete;
   rowsToDelete.push_back("O20140508");
   rowsToDelete.push_back("O20140509");
   rowsToDelete.push_back("O20140512");
   //deleteRows(rowsToDelete);
   double hintRate = 0.0;
   DBFilter::getGlobalHitRate(hintRate);
   printf("\n\n\n=================Global Hint Rate:%f\n\n\n", hintRate);
*/
   return 1;
}

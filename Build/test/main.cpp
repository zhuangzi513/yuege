
#include "Forecaster.h"
#include "OriginDBHelper.h"
#include "DBFilter.h"
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
        printf( "Fail to opendir:%s\n", currentDir.c_str());
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
                    printf( "find file named:%s, first '.db':%d \n", childName.c_str(), childName.find_first_of(".db"));
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
            break;
        }
    }
    closedir(pDir);

    
    return;
}

bool filterOriginDB() {
//    std::vector<std::string> fileNames;
//    getAllDatabase(fileNames, "dbs");
//    Forecaster* pForecaster = new Forecaster();
//    for (int i = 0; i < fileNames.size(); i++) {
//        //pForecaster->forecasteFromFirstPositiveFlowin(fileNames[i]);
//        printf("fileNames:%s", fileNames[i].c_str());
//    }
    return true;
}

bool updateOriginDBs(const std::string& dirName) {
    OriginDBHelper* originDBHelper = new OriginDBHelper();

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
             break;
         }
         originDBHelper->updateOriginDBForStock(*itrDetail, *itrDB);
         itrDB++;
         itrDetail++;
    }

    return true;
}

int main() {
   std::string fileName(EXAMPLE_XLS_NAME);
   updateOriginDBs("details");
   //filterOriginDB();
   //double hintRate = 0.0;
   //DBFilter::getGlobalHitRate(hintRate);
   //printf("\n\n\n=================Global Hint Rate:%f\n\n\n", hintRate);
   return 1;
}

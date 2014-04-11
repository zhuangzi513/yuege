
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

bool createOriginDB(const std::string& fileName)
{
    OriginDBHelper* originDBHelper = new OriginDBHelper();
    if (originDBHelper) {
        originDBHelper->createOriginTableFromFile(fileName);
    }
    return true;
}

void getAllDatabase(std::vector<std::string>& fileNames) {
    std::string currentDir("./");
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
        pDirent = readdir(pDir);
        if (pDirent != NULL) {
            if ((strcmp((const char*)pDirent->d_name, ".") == 0) ||
                (strcmp((const char*)pDirent->d_name, "..") == 0) ){
                continue;
            }

            childName = pDirent->d_name;
            if (lstat(childName.c_str(), &s) == -1) {
                continue;
            }

            if (S_ISREG(s.st_mode)) {
                if (childName.find_first_of(".db") == 6) {
                    printf( "find file named:%s, first '.db':%d \n", childName.c_str(), childName.find_first_of(".db"));
                    fileNames.push_back(childName);
                }
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
    std::vector<std::string> fileNames;
    getAllDatabase(fileNames);
    Forecaster* pForecaster = new Forecaster();
    for (int i = 0; i < fileNames.size(); i++) {
        pForecaster->forecasteThroughTurnOver(fileNames[i]);
    }
    return true;
}

bool createOriginDBForDir(const std::string& dirName) {
    OriginDBHelper* originDBHelper = new OriginDBHelper();
    if (originDBHelper) {
        originDBHelper->createOriginDBForDir(dirName);
    }
    return true;
}

int main() {
   std::string fileName(EXAMPLE_XLS_NAME);
   //createOriginDBForDir("details");
   filterOriginDB();
   double hintRate = 0.0;
   DBFilter::getGlobalHitRate(hintRate);
   printf("\n\n\n=================Global Hint Rate:%f\n\n\n", hintRate);
   return 1;
}

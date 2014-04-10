
#include "Forecaster.h"
#include "OriginDBHelper.h"
#include "DBFilter.h"
#include "sqlite3.h"

#include <stdio.h>
#include <stdlib.h>

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

bool filterOriginDB(const std::string& fileName) {
    //DBFilter *pDBFilter = new DBFilter();
    //pDBFilter->filterOriginDBByTurnOver(fileName, 100, 1000);
    Forecaster* pForecaster = new Forecaster();
    pForecaster->forecasteThroughTurnOver(fileName);
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
   filterOriginDB("600022.db");
   return 1;
}

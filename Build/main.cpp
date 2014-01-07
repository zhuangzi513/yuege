#include <stdlib.h>
#include <stdio.h>
#include <string>

#include "DBBuilder.h"
#include "DBOperations.h"

static std::string guseless("useless");
static std::string gfilterDBName("filter.db");
static std::string gfilterTableName("test");
static DBBuilder* gBuilder = NULL;

int testCallback(void *arg1, int arg2, char **arg3, char **arg4) {
    static int count = 0;
    //printf("searchCallback:%p\n", arg1);
    //printf("searchCallback:%d\n", arg2);
    //printf("searchCallback:%p\n", arg3);
    //printf("searchCallback:%p\n", *arg3);

    if (count == 0) {
        for (int i = 0; i < arg2; i++) {
            printf("%-8s", *(arg4 + i));
        }
        printf("\n");
    }

    for (int i = 0; i < arg2; i++) {
        printf("%-8s", *(arg3 + i));
    }
    printf("\n");
    count = 1;
    return 0;
}

bool createTable(std::string& DBName, std::string& tableName, bool isDefault) {
    if (isDefault) {
        return gBuilder->createTable1(DBName, tableName);
    } else {
        return gBuilder->createTable2(DBName, tableName);
    }
}

bool insertDefaultElement(std::string& tableName, bool isDefault) {
   //4*23
   char names[100] = {0};
   //6*23 
   // 600001 2100.25
   // 60000* 3.5
   char values[150] = {0};
   bool ret = false;
   std::string sql("");
   std::string value("");

   std::string ID("StockID");
   std::string date0("date0");
   std::string date1("date1");
   std::string date2("date2");

   float v1 = 1.2345;
   float v2 = 2.2345;
   float v3 = 3.2345;

   if (!isDefault) {
       date0 = "date3";
       date1 = "date4";
       date2 = "date5";
       v1 = 3.2345;
       v2 = 4.2345;
       v3 = 5.2345;
   }

   for (int i = 6001; i < 6020; i++) {
       sprintf(names, "(%s, %s, %s, %s) ", ID.c_str(), date0.c_str(), date1.c_str(), date2.c_str());
       sprintf(values, "VALUES (%d, %d, %d, %d) ", i, ((int)(v1)), ((int)(v2)), ((int)(v3)));
       //printf("names:%s\n", names);
       //printf("values:%s\n", values);

       sql = std::string(names) + std::string(values);
       printf("sql:%s\n", sql.c_str());
       ret = gBuilder->insertElement(gfilterDBName, tableName, sql, testCallback);
       if (!ret) {
           return ret;
       }
   }
   return ret;
}

bool deleteElement() {
    bool ret = false;
    std::string sql("");
    std::string condition("");

    condition = " WHERE StockID = 6001";
    ret = gBuilder->deleteElement(gfilterDBName, gfilterTableName, condition, testCallback);
    if (!ret) {
        return false;
    }

    return true;
}

bool innerJoin(std::string& DBName, std::string& srcTableName, std::string& targetTableName) {
    bool ret = false;

    ret = gBuilder->joinTables(DBName, srcTableName, targetTableName, testCallback);
    return ret;
}

int printTable(std::string& DBName, std::string& tableName) {
   printf("printTable\n");
   std::string sql("");
   sql = "SELECT DISTINCT * FROM " + tableName;
   printf("sql:%s\n", sql.c_str());
   gBuilder->selectElements(gfilterDBName, gfilterTableName, sql, testCallback);
}



int main() {
   bool ret = false;
   gBuilder = new DBBuilder();

   ret = gBuilder->openFilterDB(gfilterDBName);
   if (!ret) {
       printf("Fail to open DB:%s\n", gfilterDBName.c_str());
       return -1;
   }

   ret = createTable(gfilterDBName, gfilterTableName, true);
   if (!ret) {
       printf("Fail to create Table:%s in DB:%s\n", gfilterTableName.c_str(), gfilterDBName.c_str());
       return -1;
   }

   ret = insertDefaultElement(gfilterTableName, true);
   if (!ret) {
       printf("Fail to insert elements to Table:%s in DB:%s\n", gfilterTableName.c_str(), gfilterDBName.c_str());
       return -1;
   }
   printTable(gfilterDBName, gfilterTableName);

   std::string second("second");
   ret = createTable(gfilterDBName, second, false);
   if (!ret) {
       printf("Fail to create Table:%s in DB:%s\n", gfilterTableName.c_str(), second.c_str());
       return -1;
   }

   ret = insertDefaultElement(second, false);
   if (!ret) {
       printf("Fail to insert elements to Table:%s in DB:%s\n", second.c_str(), gfilterDBName.c_str());
       return -1;
   }
   printTable(gfilterDBName, second);

   ret = innerJoin(gfilterDBName, gfilterTableName, second);
   if (!ret) {
       printf("Fail to join Tables Table1:%s Table2:%s\n", gfilterDBName.c_str(), second.c_str());
       return -1;
   }
   printTable(gfilterDBName, second);

   //ret = deleteElement();
   //if (!ret) {
   //    printf("Fail to delete elements from Table:%s in DB:%s\n", gfilterTableName.c_str(), gfilterDBName.c_str());
   //    return -1;
   //}


   gBuilder->closeFilterDB(gfilterDBName);
   return 1;
}

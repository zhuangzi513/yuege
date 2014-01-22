#include <stdio.h>
#include <stdlib.h>
#include "sqlite3.h"

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

int main()
{
    sqlite3* db;
    int ret;
    char *sql;
    char *zerr;

    ret = sqlite3_open("test.db",&db);
    if(ret)
    {
     fprintf(stderr,"cannot open db : %s\n",sqlite3_errmsg(db));
            sqlite3_close(db);
            return 1;
    }

//    sql = "CREATE TABLE details(id int ,     \
                                date0 int,   \
                                date1 int,   \
                                date2 int,   \
                                date3 int,   \
                                date4 int,   \
                                date5 int,   \
                                date6 int,   \
                                date7 int,   \
                                date8 int,   \
                                date9 int,   \
                                date10 int,  \
                                date11 int,  \
                                date12 int,  \
                                date13 int,  \
                                date14 int,  \
                                date15 int,  \
                                date16 int,  \
                                date17 int,  \
                                date18 int,  \
                                date19 int,  \
                                date20 int,  \
                                date21 int,  \
                                date22 int,  \
                                date23 int)";
    sql = "CREATE TABLE details(id int ,     \
                                date0 int,   \
                                date1 int,   \
                                date2 int)";
    ret = sqlite3_exec(db, sql, NULL, NULL, &zerr);
    if(ret != SQLITE_OK)
    {
     if(zerr!=NULL)
            {
             fprintf(stderr,"SQL error:%s\n",zerr);
                    sqlite3_free(zerr);
            }
    }
    sql = "INSERT INTO details (date0, date1, date2) VALUES ('40', '40', '40')";
    ret = sqlite3_exec(db, sql, NULL, NULL, &zerr);

    sql = "SELECT * FROM details";
    ret = sqlite3_exec(db, sql, searchCallback, NULL, &zerr);

    printf("select ok:%s\n", ret);
    sqlite3_close(db);
    return 0;
}

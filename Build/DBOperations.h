#define CREATE_TABLE "CREATE TABLE "
#define INSERT_OP    "INSERT INTO "
#define DELETE_OP    "DELETE FROM "
#define VALUE_OP     "VALUES "
#define TABLE_FORMAT1 "(StockID int,  \
                       date0   int,  \
                       date1   int,  \
                       date2   int) "
#define TABLE_FORMAT2 "(StockID int,  \
                       date3   int,  \
                       date4   int,  \
                       date5   int) "

#define COLUMNS(format, ...)   sprintf(format, ##__VA_ARGS__)
#define VALUE_OF(arg1, ...)  (#arg1, ##__VA_ARGS__)


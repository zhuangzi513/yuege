#define CREATE_TABLE "CREATE TABLE "
#define INSERT_OP    "INSERT INTO "
#define DELETE_OP    "DELETE FROM "
#define VALUE_OP     "VALUES "
#define TABLE_FORMAT2 "(StockID int,  \
                       date0   int,  \
                       date1   int,  \
                       date2   int) "

#define TABLE_FORMAT_ORIGIN_DEF   " (Time varchar(12), Price double, Float double, Volume int,  TurnOver double,  SaleBuy varchar(6)) "
#define TABLE_FORMAT_ORIGIN   " (Time, Price, Float, Volume,  TurnOver,  SaleBuy) "

#define TABLE_FORMAT_FILTER_TURNOVER_DEF " (Volume, TurnOver, SaleBuy) "
#define TABLE_FORMAT_FILTER_RESULT_DEF " (Date varchar(12), TurnOverSale int, PriceSale double, TurnOverBuy int, PriceBuy double) "
//default
#define D_STMT_FORMAT_ORGIN   " VALUES (?, ?, ?, ?, ?, ?) "
//spec
#define S_STMT_FORMAT_ORGIN   " VALUES (:Time, :Price, :Float, :Volume, :TurnOver, :SaleBuy) "

#define BUY_IN_CHINESE  "买盘"
#define SAL_IN_CHINESE  "卖盘"

#define COLUMNS(format, ...)   sprintf(format, ##__VA_ARGS__)
#define VALUE_OF(arg1, ...)  (#arg1, ##__VA_ARGS__)


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

#define TABLE_FORMAT_BANKER_DEF " (Date varchar(12), IsBankerIncharge varchar(6), IsPositive varchar(6), BuyToSale double) "
#define TABLE_FORMAT_BANKER " (Date, IsBankerIncharge, IsPositive, BuyToSale) "

#define TABLE_FORMAT_FILTER_TURNOVER_DEF " (Volume, TurnOver, SaleBuy) "

#define TABLE_FORMAT_FILTER_RESULT_DEF " (Date varchar(12), VolumeSale int, TurnOverSale double, PriceSale double, VolumeBuy int, TurnOverBuy double, PriceBuy double, TurnOverFlowInOneDay double, VolumeFlowInOneDay double, BeginPrice double, EndPrice double, TurnOverFlowInFiveDays double, VolumeFlowInFiveDays int, TurnOverFlowInTenDays double, VolumeFlowInTenDays int, TurnOverFlowInMonDays double, VolumeFlowInMonDays int) "
#define TABLE_FORMAT_FILTER_RESULT " (Date, VolumeSale, TurnOverSale, PriceSale, VolumeBuy, TurnOverBuy, PriceBuy, TurnOverFlowInOneDay, VolumeFlowInOneDay, BeginPrice, EndPrice, TurnOverFlowInFiveDays, VolumeFlowInFiveDays, TurnOverFlowInTenDays, VolumeFlowInTenDays, TurnOverFlowInMonDays, VolumeFlowInMonDays) "

//default
#define D_STMT_FORMAT_ORGIN   " VALUES (?, ?, ?, ?, ?, ?) "
#define D_STMT_FORMAT_BANKER  " VALUES (?, ?, ?, ?) "
#define D_STMT_FORMAT_FILTER_RESULT " VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "

//spec
#define S_STMT_FORMAT_ORGIN   " VALUES (:Time, :Price, :Float, :Volume, :TurnOver, :SaleBuy) "
#define S_STMT_FORMAT_BANKER  " VALUES (:Date, :IsBankerIncharge, :IsPositive, :BuyToSale) "
#define S_STMT_FORMAT_FILTER_RESULT " VALUES (:Time, :VolumeSale, :TurnOverSale, :PriceSale, :VolumeBuy, :TurnOverBuy, :PriceBuy, :TurnOverFlowInOneDay, :VolumeFlowInOneDay, :BeginPrice, :EndPrice, :TurnOverFlowInFiveDays, :VolumeFlowInFiveDays, :TurnOverFlowInTenDays, :VolumeFlowinTenDays, :TurnOverFlowInMonDays, :VolumeFlowInMonDays) "

#define BUY_IN_CHINESE  "买盘"
#define SAL_IN_CHINESE  "卖盘"

#define COLUMNS(format, ...)   sprintf(format, ##__VA_ARGS__)
#define VALUE_OF(arg1, ...)  (#arg1, ##__VA_ARGS__)


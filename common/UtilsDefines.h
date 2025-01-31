#ifndef UTILS_DEFINS_H
#define UTILS_DEFINS_H

#define STRING_DATE                " Date "
#define STRING_VOLUME              " Volume "
#define STRING_TURNOVER            " TurnOver "
#define STRING_TURNOVER_SALE       " TurnOverSale "
#define STRING_TURNOVER_BUY        " TurnOverBuy "
#define STRING_SALE_BUY            " SaleBuy "
#define STRING_BEGIN_PRICE         " BeginPrice "
#define STRING_END_PRICE           " EndPrice "

#define STRING_TURNOVER_FLOWIN_ONE_DAY  " TurnOverFlowInOneDay "
#define STRING_VOLUME_FLOWIN_ONE_DAY    " VolumeFlowInOneDay "

#define STRING_TURNOVER_FLOWIN_FIVE_DAY " TurnOverFlowInFiveDays "
#define STRING_VOLUME_FLOWIN_FIVE_DAY   " VolumeFlowInFiveDays "

#define STRING_TURNOVER_FLOWIN_TEN_DAYS " TurnOverFlowInTenDays "
#define STRING_VOLUME_FLOWIN_TEN_DAYS   " VolumeFlowInTenDays "

#define STRING_TURNOVER_FLOWIN_MON_DAYS " TurnOverFlowInMonDays "
#define STRING_VOLUME_FLOWIN_MON_DAYS   " VolumeFlowInMonDays "


static std::string SELECT_COLUMNS(const std::string& tableName, const std::string& targetColumns) {
    std::string command("");
    command += " SELECT ";
    command += targetColumns;
    command += " FROM ";
    command += tableName;
    return command;
}

static std::string SELECT_TURNOVER_INTO(const std::string& srcTable,
                                        const std::string& targetTable,
                                        const std::string& columnNames,
                                        const std::string& arg) {
    std::string command("");
    command += " INSERT INTO ";
    command += targetTable;
    command += " SELECT ";
    command += columnNames;
    command += " FROM ";
    command += srcTable;
    command += " WHERE ";
    command += STRING_TURNOVER;
    command += " >= ";
    command += arg;
    return command;
}

static std::string SELECT_COLUMNS_IN_ORDER(const std::string& srcTable,
                                           const std::string& columnNames,
                                           const std::string& key,
                                           const bool positiveOrder) {
    std::string command("");
    command += " SELECT ";
    command += columnNames;
    command += " FROM ";
    command += srcTable;
    command += " ORDER BY ";
    command += key;
    if (positiveOrder) {
       command += "ASC";
    } else {
       command += "DESC";
    }

    return command;
}

static std::string SELECT_COLUMNS_IN_GROUP(const std::string& srcTable,
                                           const std::string& columnNames,
                                           const std::string& key) {
    std::string command("");
    command += " SELECT ";
    command += columnNames;
    command += " FROM ";
    command += srcTable;
    command += " GROUP BY ";
    command += key;

    return command;
}


static std::string SELECT_IN(const std::string& srcTable, const std::string& targetDBName) {
    std::string command("");
    command += " SELECT * INTO ";
    command += srcTable;
    command += " IN ";
    command += targetDBName;
    return command;
}

static std::string REMOVE_TABLE(const std::string& srcTable) {
    std::string command("");
    command += " DROP TABLE ";
    command += srcTable;
    return command;
}

static std::string DELETE_FROM_TABLE(const std::string& srcTable) {
    std::string command("");
    command += " DELETE FROM ";
    command += srcTable;

    return command;
}

static std::string GET_TABLES() {
    std::string command("");
    command += " SELECT name FROM ";
    command += " sqlite_master ";
    command += " WHERE TYPE=\"table\"";
    command += " ORDER BY name ";
    return command;
}

#endif

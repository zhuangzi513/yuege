#ifndef PRICE_DISCOVER_H

#include <string>
#include <list>

class sqlite3;

class PriceDiscover {
  public:
    PriceDiscover(const std::string& aDBName, const std::string& aTableName);
    ~PriceDiscover();
// Begin TurnOverDiscover
    int isDBBuyMoreThanSale(const std::string& aDBName, int aDayCount, int pre, float ratio);
// End TurnOverDiscover


    /*
     * Return how long it keeps sideways.
     *
     * Return 0 if error happens
     *
    */
    int howLongSteadySideWays();

    /*
     * PhaseOne: Steady SideWays and keep eating in.
     *
     * aTableName: The input parameter which target at a result table in the OriginDatabase.
     *
     * return true if yes
    */
    bool isInPhaseOne(const std::string& aTableName, const int aLatestCount);

    /*
     * PhaseTwo: End of Steady SideWays and about to flying.
     *
     * aTableName: The input parameter which target at a result table in the OriginDatabase.
     *
     * return true if yes
    */

    bool isInPhaseTwo(const std::string& aTableName, const int aLatestCount);

    /*
     * PhaseThree: Flying in the sky, enjoy your self.
     *
     * aTableName: The input parameter which target at a result table in the OriginDatabase.
     *
     * return true if yes
    */
    bool isInPhaseThree(const std::string& aTableName);

  private:
    /*
     *
    */
    /*
     * Check whether the specific database with name 'aDBName' is in the state
     * of SideWays.
     *
     * aDaysCount: The length of days to check
     *
     * return true if the specific databse is in the state of sideways.
    */

    bool isSteadySideWays(int aDaysCount);


    /*
     * Get all the valuable info from the target result table;
     *
     * aTable: The input parameter which target at a result table.
     *
     * return true if successfully get the entire info.
    */
    bool getNeededInfoFromTable(const std::string& aTableName);

    /*
     * Decide whether the given real-float/abs-float meets the need of sideways
     *
     * aRealFloat: The input paramter which equals to (start-price of the first day - end-price of the last day);
     * aABSFloat:  The input paramter which equals to the sum of abs(start-price - end-price) of everyday;
     * aDaysCount: The input paramter which is the size of days counted in.
     *
     * return true if it is decide as sideways.
     *
    */
    bool isPriceSideWays(double aRealFloat, double aABSFloat, int aDaysCount);

    /*
     * Get the PriceData(s) in the result table named by input arg 'aTable'.
     *
     * return true if successfully get the PriceData(s).
    */
    bool getPriceDatasFromResultTable();


  private:
    class ValueAbleInfo {
      public:
        ValueAbleInfo()
            : mDate("") {
            mVolumeBuyOneDay = 0;
            mVolumeSaleOneDay = 0;
            mVolumeFlowInOneDay = 0;

            mBeginPrice = 0;
            mEndPrice = 0;
            mTurnOverBuyOneDay = 0;
            mTurnOverSaleOneDay = 0;
            mTurnOverFlowInOneDay = 0;
        }

        ValueAbleInfo(const ValueAbleInfo& srcValueAbleInfo) {
            mDate = srcValueAbleInfo.mDate;
            mVolumeBuyOneDay = srcValueAbleInfo.mVolumeBuyOneDay;
            mVolumeSaleOneDay = srcValueAbleInfo.mVolumeSaleOneDay;
            mVolumeFlowInOneDay = srcValueAbleInfo.mVolumeFlowInOneDay;

            mBeginPrice = srcValueAbleInfo.mBeginPrice;
            mEndPrice = srcValueAbleInfo.mEndPrice;
            mTurnOverBuyOneDay = srcValueAbleInfo.mTurnOverBuyOneDay;
            mTurnOverSaleOneDay = srcValueAbleInfo.mTurnOverSaleOneDay;
            mTurnOverFlowInOneDay = srcValueAbleInfo.mTurnOverFlowInOneDay;
        }

        int mVolumeBuyOneDay;
        int mVolumeSaleOneDay;
        int mVolumeFlowInOneDay;
        double mBeginPrice;
        double mEndPrice;
        double mTurnOverBuyOneDay;
        double mTurnOverSaleOneDay;
        double mTurnOverFlowInOneDay;
        std::string mDate;
    };
    class PriceData {
      public:
        PriceData() {
            mDate = "";
            mEndPrice = 0;
            mBeginPrice = 0;
        }

        PriceData(const PriceData& priceData) {
            mDate = priceData.mDate;
            mEndPrice = priceData.mEndPrice;
            mBeginPrice = priceData.mBeginPrice;
        }

        std::string mDate;
        double mEndPrice;
        double mBeginPrice;
    };

    sqlite3* mOriginDB;
    std::string mDBName;
    std::string mTargetResultTableName;
    std::list<ValueAbleInfo>::iterator mStartItr;

    //reversed order by Date
    std::list<PriceData>     mPriceDatas;
    std::list<ValueAbleInfo> mValuableInfos;

    std::list<std::string>   mSideWaysInMonDays;
    std::list<std::string>   mSideWaysInTenDays;
    std::list<std::string>   mSideWaysInFiverDays;
};

#define PRICE_DISCOVER_H
#endif

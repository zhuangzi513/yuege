#ifndef PRICE_DISCOVER_H

#include <string>
#include <list>

class sqlite3;

class PriceDiscover {
  public:
    PriceDiscover(const std::string& aDBName);
    ~PriceDiscover();

    /*
     * Check whether the specific database with name 'aDBName' is in the state
     * of SideWays.
     *
     * aTableName: The input parameter which target at a result table in the OriginDatabase.
     *
     * return true if the specific databse is in the state of sideways.
    */

    bool isSteadySideWays(const std::string& aTableName);

    /*
     * PhaseOne: Steady SideWays and keep eating in.
     *
     * aTableName: The input parameter which target at a result table in the OriginDatabase.
     *
     * return true if yes
    */
    bool isInPhaseOne(const std::string& aTableName);

    /*
     * PhaseTwo: End of Steady SideWays and about to flying.
     *
     * aTableName: The input parameter which target at a result table in the OriginDatabase.
     *
     * return true if yes
    */

    bool isInPhaseTwo(const std::string& aTableName);

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
     * aTable: The input parameter which target at a result table.
     *
     * return true if successfully get the PriceData(s).
    */
    bool getPriceDatasFromResultTable(const std::string& aTable);


  private:
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
    std::list<PriceData>   mPriceDatas;
    std::list<std::string> mSideWaysInMonDays;
    std::list<std::string> mSideWaysInTenDays;
    std::list<std::string> mSideWaysInFiverDays;
};

#define PRICE_DISCOVER_H
#endif

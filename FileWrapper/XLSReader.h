#ifndef XLS_READER_H
#define XLS_READER_H

#include <string>
#include <stdint.h>
#include <stdio.h>

class XLSReader {
  public:
    class XLSElement {
      public:
      enum {
        TIME = 0,
        PRICE,
        FLOAT,
        VOLUME,
        TURNOVER,
        SBFLAG
      };
      void dump() { printf("mTime:%10s, mPrice:%10f, mFloat:%10f, mVolume:%10d, mTurnOver:%10f\n", mTime, mPrice, mFloat, mVolume, mTurnOver);};
      int32_t mSB;
      int32_t mDate;
      int32_t mTime;
      int32_t mVolume;
      double mTurnOver;
      double mPrice;
      double mFloat;
    };
};

#endif

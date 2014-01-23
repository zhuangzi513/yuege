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
      void dump() { printf("mTime:%10s, mPrice:%10s, mFloat:%10s, mVolume:%10s, mTurnOver:%10s\n", mTime.c_str(), mPrice.c_str(), mFloat.c_str(), mVolume.c_str(), mTurnOver.c_str());};
      std::string mSB;
      std::string mDate;
      std::string mTime;
      std::string mVolume;
      std::string mTurnOver;
      std::string mPrice;
      std::string mFloat;
    };
};

#endif

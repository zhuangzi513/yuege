#ifndef XLS_READER_H
#define XLS_READER_H

#include <string>
#include <stdint.h>

class XLSReader {
  public:
    class XLSElement {
      public:
      void dump() { };
      int32_t SB;
      int32_t date;
      int32_t time;
      int32_t volume;
      float   price;
      int32_t turnover;
    };
};

#endif

#include "TextXLSReader.h"

#include "ConstDefines.h"
#include "ErrorDefines.h"

#include <stdlib.h>
#include <fstream>
#include <errno.h>

#define SPACE ' '
#define TAB   '	'
#define INTERNAL_MAX 900
#define LOGTAG "TextXLSReader"

//FIXME: Add this to fix the linker Error: no definition of `__dso_handle'
#ifndef TEST
extern "C"
{
void *__dso_handle = NULL;
}
#endif

static const std::string sFileFormat="text";

TextXLSReader::TextXLSReader(const std::string& fileFormat) {
}

TextXLSReader::~TextXLSReader() {
}

//static
bool TextXLSReader::getElementsFrom(const std::string& fileName, std::list<XLSReader::XLSElement*>& out) {
    LOGI(LOGTAG, "fileName:%s", fileName.c_str());
    if (fileName.empty()) {
        //LOG ERR
        return false;
    }

    errno = 0;
    std::ifstream fileStream;
    //ios::in : read the file content into the mem
    //1       : readonly
    fileStream.open(fileName.c_str(), std::ios::in);
    LOGI(LOGTAG, "errno:%d", errno);
    if (fileStream.fail()) {
        //LOG ERR
        return false;
    }

    if (!readFile(fileStream, out)) {
        //LOG ERR
        return false;
    }

    fileStream.close();
    fileStream.clear();
    return true;
}

bool TextXLSReader::readFile(std::ifstream& fileStream, std::list<XLSReader::XLSElement*>& out) {
    std::string line;
   
    while(true) {
        std::getline(fileStream, line);
        if (!fileStream.fail()) {
            XLSReader::XLSElement* pElement = new XLSReader::XLSElement();
            if (convertStringIntoData(line, *pElement)) {
                out.push_back(pElement);
            } else {
                //LOG ERR
                return false;
            }
        } else if(fileStream.eof()) {
            //LOG
            break;
        } else {
            //LOG ERR
            return false;
        }
    }

    return true;
}

static int32_t findStartOfChar(const std::string& srcString) {
    int32_t indexSpace = 0;
    int32_t indexTab   = 0;

    indexSpace = srcString.find_first_of(SPACE);
    indexSpace = ((indexSpace >= 0) ? indexSpace : INTERNAL_MAX);
    indexTab   = srcString.find_first_of(TAB);
    indexTab   = ((indexTab >= 0) ? indexTab : INTERNAL_MAX);
    return ((indexSpace < indexTab) ? indexSpace : indexTab);
}

static bool parse(int32_t index, const std::string& srcString, XLSReader::XLSElement& outData) {
    //index
    // 0         1          2            3           4           5
    // time      price      float        volume      turnover    SB
    
    switch(index) {
      case XLSReader::XLSElement::TIME: {
          outData.mTime = srcString;
          break;
      }
      case XLSReader::XLSElement::PRICE: {
          outData.mPrice = srcString;
          break;
      }
      case XLSReader::XLSElement::FLOAT: {
          outData.mFloat = srcString;
          break;
      }
      case XLSReader::XLSElement::VOLUME: {
          outData.mVolume = srcString;
          break;
      }
      case XLSReader::XLSElement::TURNOVER: {
          outData.mTurnOver = srcString;
          break;
      }
      case XLSReader::XLSElement::SBFLAG: {
          if (srcString == BUY_IN_CHINESE) {
              outData.mSB = "true";
          } else if (srcString == SAL_IN_CHINESE) {
              outData.mSB = "false";
          } else {
              //LOG ERR
              return false;
          }
          break;
      }
      default:
          return false;
    }

    return true;
}

bool TextXLSReader::convertStringIntoData(const std::string& srcString, XLSReader::XLSElement& outData) {
    //Format of the string:
    //  Time       price    float   volume    turnover 
    //  09:31:00   22.00    0.01    20        20 * 22 * 100
    std::string string(srcString);
    std::string targetString;
    int32_t startOfChar = -1;
    int32_t index       = 0;

    while(!string.empty()) {
        int32_t startOfChar = findStartOfChar(string);
        if (0 == startOfChar) {
            string = string.substr(1);
        } else if (startOfChar != INTERNAL_MAX) {
            targetString = string.substr(0, startOfChar);
            parse(index, targetString, outData);
            string = string.substr(startOfChar);
            index++;
        } else {
            //If start of both of SPACE and TAB is INTERNAL_MAX
            //It means that we are at the end of the string.
            parse(index, string, outData);
            index++;
            break;
        }
    }
    return true;
}

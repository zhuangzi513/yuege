#include "TextXLSReader.h"

//#include <iostream>
#include <stdlib.h>
#include <fstream>

#define SPACE ' '
#define TAB   '	'
#define INTERNAL_MAX 900

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
    if (fileName.empty()) {
        //LOG ERR
        return false;
    }

    std::ifstream fileStream;
    //ios::in : read the file content into the mem
    //1       : readonly
    fileStream.open(fileName.c_str(), std::ios::in);
    if (fileStream.fail()) {
        //LOG ERR
        return false;
    }

    if (!readFile(fileStream, out)) {
        //LOG ERR
        return false;
    }

    fileStream.close();
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
          break;
      }
      case XLSReader::XLSElement::PRICE: {
          double price = atof(srcString.c_str());
          outData.mPrice = price;
          break;
      }
      case XLSReader::XLSElement::FLOAT: {
          double floatV = atof(srcString.c_str());
          outData.mFloat = floatV;
          break;
      }
      case XLSReader::XLSElement::VOLUME: {
          int32_t volume = atof(srcString.c_str());
          outData.mVolume = volume;
          break;
      }
      case XLSReader::XLSElement::TURNOVER: {
          double turnover = atof(srcString.c_str());
          outData.mTurnOver = turnover;
          break;
      }
      case XLSReader::XLSElement::SBFLAG: {
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

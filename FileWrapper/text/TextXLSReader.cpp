#include "TextXLSReader.h"

//#include <iostream>
#include <fstream>

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
   
    //while(true) {
    //    std::getline(fileStream, line);
    //    if (!fileStream.fail()) {
    //        printf("read:%s\n", line.c_str());
    //        XLSReader::XLSElement* pElement = new XLSReader::XLSElement();
    //        if (convertStringIntoData(line, *pElement)) {
    //            //out.push_back(pElement);
    //        } else {
    //            //LOG ERR
    //            return false;
    //        }
    //    } else if(fileStream.eof()) {
    //        printf("end\n");
    //        break;
    //    } else {
    //        printf("fail:\n");
    //        return false;
    //    }
    //}

    XLSReader::XLSElement* pElement = new XLSReader::XLSElement();
    convertStringIntoData("", *pElement);

    return true;
}

bool TextXLSReader::convertStringIntoData(const std::string& srcString, XLSReader::XLSElement& pOutData) {
    //if (*pOutData == NULL) {
    //    //LOG ERR
    //    return false;
    //}

    //Format of the string:
    //  Time       price    float   volume    turnover 
    //  09:31:00   22.00    0.01    20        20 * 22 * 100
    //std::string string("09:31:00   22.00    0.01    20        20 * 22 * 100");
    std::string string("4买盘");//:31买盘买盘");
    //std::string string(srcString);
    std::string targetString;
    int32_t len = string.length();
    printf("string len:%d\n", string.length());
    for (int32_t i = 0; i < len; i++ ) {
         printf("string len:%c\n", string[i]);
    }
    return true;

    int32_t index = 0;
    while(!string.empty()) {
        index = string.find_first_of(' ');
        if (0 == index) {
            string = string.substr(1);
            printf("string:%s\n", string.c_str());
        } else {
            index = string.find_first_of(' ');
            targetString = string.substr(0, index);
            printf("target string:%s, index:%d\n", targetString.c_str(), index);
            string = string.substr(index);
            printf("string:%s\n", string.c_str());
        }
    }
    return true;
}
